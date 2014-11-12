/*
 * Copyright 2014 Carnegie Mellon University.  
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */

package edu.cmu.sphinx.linguist.acoustic.tiedstate.tiedmixture;

import static edu.cmu.sphinx.linguist.acoustic.tiedstate.Pool.Feature.NUM_GAUSSIANS_PER_STATE;
import static edu.cmu.sphinx.linguist.acoustic.tiedstate.Pool.Feature.NUM_SENONES;
import static edu.cmu.sphinx.linguist.acoustic.tiedstate.Pool.Feature.NUM_STREAMS;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.net.MalformedURLException;
import java.net.URISyntaxException;
import java.net.URL;
import java.util.ArrayList;

import edu.cmu.sphinx.linguist.acoustic.UnitManager;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.Pool;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.Senone;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.Sphinx3Loader;
import edu.cmu.sphinx.util.ExtendedStreamTokenizer;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4Integer;

/**
 * Loads a PTM (phonetic tied mixture) acoustic model generated by the Sphinx-3 trainer.
 */

public class Sphinx3PTMLoader extends Sphinx3Loader {
	
    /**
     * Number of top Gaussians to use in scoring
     */
    @S4Integer(defaultValue = 4)
    public final static String PROP_TOPN = "topGaussiansNum";
	
    private int[] senone2ci;
    private int numBase;
    private int topGauNum;
    
    public Sphinx3PTMLoader(URL location, String model, String dataLocation,
            UnitManager unitManager, float distFloor, float mixtureWeightFloor,
            float varianceFloor, boolean useCDUnits, int topGauNum) {

        super(location, model, dataLocation, unitManager, distFloor,
                mixtureWeightFloor, varianceFloor, useCDUnits);
        this.topGauNum = topGauNum;
    }

    public Sphinx3PTMLoader(String location, String model, String dataLocation,
            UnitManager unitManager, float distFloor, float mixtureWeightFloor,
            float varianceFloor, boolean useCDUnits, int topGauNum)
            throws MalformedURLException, ClassNotFoundException {

        super(location, model, dataLocation, unitManager, distFloor, 
        		mixtureWeightFloor, varianceFloor, useCDUnits);
        this.topGauNum = topGauNum;
    }
    
    public Sphinx3PTMLoader() {
    	
    }
    
    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
    	super.newProperties(ps);
    	this.topGauNum = ps.getInt(PROP_TOPN);
    }
    
    /**
     * Loads the AcousticModel from a directory in the file system.
     * 
     * @param modelDef
     *            the name of the acoustic modelDef; if null we just load from
     *            the default location
     * @throws java.io.IOException
     */
    @Override
    protected void loadModelFiles(String modelDef) throws IOException,
            URISyntaxException {

        logger.config("Loading Sphinx3 acoustic model: " + modelDef);
        logger.config("    modelName: " + this.model);
        logger.config("    dataLocation   : " + dataLocation);

        meansPool = loadDensityFile(dataLocation + "means", -Float.MAX_VALUE);
        variancePool = loadDensityFile(dataLocation + "variances",
                varianceFloor);
        mixtureWeightsPool = loadMixtureWeights(dataLocation
                + "mixture_weights", mixtureWeightFloor);
        transitionsPool = loadTransitionMatrices(dataLocation
                + "transition_matrices");
        transformMatrix = loadTransformMatrix(dataLocation
                + "feature_transform");

        // load the HMM modelDef file
        InputStream modelStream = getDataStream(this.model);
        if (modelStream == null) {
            throw new IOException("can't find modelDef " + this.model);
        }
        getSenoneToCIPhone(useCDUnits, modelStream, this.model);

        senonePool = createSenonePool(distFloor, varianceFloor);

        modelStream = getDataStream(this.model);
        loadHMMPool(useCDUnits, modelStream, this.model);
        
        modelProps = loadModelProps(dataLocation + "feat.params");
    }
    
    /**
     * Creates the senone pool from the rest of the pools.
     * 
     * @param distFloor
     *            the lowest allowed score
     * @param varianceFloor
     *            the lowest allowed variance
     * @return the senone pool
     */
    @Override
    protected Pool<Senone> createSenonePool(float distFloor, float varianceFloor) {
        Pool<Senone> pool = new Pool<Senone>("senones");
        int numMixtureWeights = mixtureWeightsPool.size();

        int numMeans = meansPool.size();
        int numVariances = variancePool.size();
        int numGaussiansPerState = mixtureWeightsPool.getFeature(NUM_GAUSSIANS_PER_STATE, 0);
        int numSenones = mixtureWeightsPool.getFeature(NUM_SENONES, 0);
        int numStreams = mixtureWeightsPool.getFeature(NUM_STREAMS, 0);

        logger.fine("Senones " + numSenones);
        logger.fine("Gaussians Per State " + numGaussiansPerState);
        logger.fine("MixtureWeights " + numMixtureWeights);
        logger.fine("Means " + numMeans);
        logger.fine("Variances " + numVariances);

        assert numGaussiansPerState > 0;
        assert numMixtureWeights == numSenones;
        assert numVariances == numBase * numGaussiansPerState * numStreams;
        assert numMeans == numBase * numGaussiansPerState * numStreams;

        float[][] meansTransformationMatrix = meanTransformationMatrixPool == null ? null
                : meanTransformationMatrixPool.get(0);
        float[] meansTransformationVector = meanTransformationVectorPool == null ? null
                : meanTransformationVectorPool.get(0);
        float[][] varianceTransformationMatrix = varianceTransformationMatrixPool == null ? null
                : varianceTransformationMatrixPool.get(0);
        float[] varianceTransformationVector = varianceTransformationVectorPool == null ? null
                : varianceTransformationVectorPool.get(0);
        
        MixtureComponentSet[] phoneticTiedMixtures = new MixtureComponentSet[numBase];
        for (int i = 0; i < numBase; i++) {
            ArrayList<PrunableMixtureComponent[]> mixtureComponents = new ArrayList<PrunableMixtureComponent[]>();
            for (int j = 0; j < numStreams; j++) {
            	PrunableMixtureComponent[] featMixtureComponents = new PrunableMixtureComponent[numGaussiansPerState];
                for (int k = 0; k < numGaussiansPerState; k++) {
                	int whichGaussian = i * numGaussiansPerState * numStreams + j * numGaussiansPerState + k;
                	featMixtureComponents[k] = new PrunableMixtureComponent(
                            meansPool.get(whichGaussian),
                            meansTransformationMatrix, meansTransformationVector,
                            variancePool.get(whichGaussian),
                            varianceTransformationMatrix,
                            varianceTransformationVector, distFloor, varianceFloor, k);
                }
                mixtureComponents.add(featMixtureComponents);
            }
            phoneticTiedMixtures[i] = new MixtureComponentSet(mixtureComponents, topGauNum);
        }
        
        for (int i = 0; i < numSenones; i++) {
            Senone senone = new SetBasedGaussianMixture(mixtureWeightsPool.get(i), phoneticTiedMixtures[senone2ci[i]], i);
            pool.put(i, senone);
        }
        return pool;
    }

    /**
     * Loads the sphinx3 density file, a set of density arrays are created and
     * placed in the given pool.
     * 
     * @param useCDUnits
     *            if true, loads also the context dependent units
     * @param inputStream
     *            the open input stream to use
     * @param path
     *            the path to a density file
     * @throws FileNotFoundException
     *             if a file cannot be found
     * @throws IOException
     *             if an error occurs while loading the data
     */
    protected void getSenoneToCIPhone(boolean useCDUnits,
            InputStream inputStream, String path) throws IOException {
        ExtendedStreamTokenizer est = new ExtendedStreamTokenizer(inputStream,
                '#', false);

        logger.fine("Loading HMM file from: " + path);

        est.expectString(MODEL_VERSION);

        numBase = est.getInt("numBase");
        est.expectString("n_base");

        int numTri = est.getInt("numTri");
        est.expectString("n_tri");

        int numStateMap = est.getInt("numStateMap");
        est.expectString("n_state_map");

        int numTiedState = est.getInt("numTiedState");
        est.expectString("n_tied_state");

        senone2ci = new int[numTiedState];

        est.getInt("numContextIndependentTiedState");
        est.expectString("n_tied_ci_state");

        int numTiedTransitionMatrices = est.getInt("numTiedTransitionMatrices");
        est.expectString("n_tied_tmat");

        int numStatePerHMM = numStateMap / (numTri + numBase);

        assert numTiedState == mixtureWeightsPool.getFeature(NUM_SENONES, 0);
        assert numTiedTransitionMatrices == transitionsPool.size();

        // Load the base phones
        for (int i = 0; i < numBase + numTri; i++) {
            //TODO name this magic const somehow
            for (int j = 0; j < 5; j++)
                est.getString();
            int tmat = est.getInt("tmat");

            for (int j = 0; j < numStatePerHMM - 1; j++) {
                senone2ci[est.getInt("j")] = tmat;
            }
            est.expectString("N");

            assert tmat < numTiedTransitionMatrices;
        }

        est.close();
    }
}
