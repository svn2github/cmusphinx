/**
 * [[[copyright]]]
 */

package tests.frontend;

import edu.cmu.sphinx.frontend.FrontEnd;
import edu.cmu.sphinx.frontend.MelFilterbank;
import edu.cmu.sphinx.frontend.SpectrumAnalyzer;
import edu.cmu.sphinx.frontend.Windower;
import edu.cmu.sphinx.frontend.Preemphasizer;


/**
 * Test program for the MelFilterbank.
 */
public class MelFilterbankTest {

    public static void main(String[] argv) {

	if (argv.length < 3) {
	    System.out.println
                ("Usage: java testClass <testName> " +
                 "<propertiesFile> <audiofilename>");
	}

        try {
            FrontEndTest fet = new FrontEndTest(argv[0], argv[1], argv[2]);

            // create the processors
            Preemphasizer preemphasizer = new Preemphasizer(argv[0]);
            Windower hammingWindow = new Windower(argv[0]);
            SpectrumAnalyzer spectrumAnalyzer = new SpectrumAnalyzer(argv[0]);
            MelFilterbank melFilterbank = new MelFilterbank(argv[0]);

            // add the processors
            FrontEnd fe = fet.getFrontEnd();
            fe.addProcessor(preemphasizer);
            fe.addProcessor(hammingWindow);
            fe.addProcessor(spectrumAnalyzer);
	    fe.addProcessor(melFilterbank);

            fet.run();

	} catch (Exception e) {
	    e.printStackTrace();
	}
    }
}
