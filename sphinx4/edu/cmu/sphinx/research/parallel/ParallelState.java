
/*
 * Copyright 1999-2002 Carnegie Mellon University.  
 * Portions Copyright 2002 Sun Microsystems, Inc.  
 * Portions Copyright 2002 Mitsubishi Electronic Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */

package edu.cmu.sphinx.research.parallel;

/**
 * A SentenceHMMState in a parallel branch.
 */
public interface ParallelState {

    /**
     * Returns the name of the acoustic model behind this 
     * ParallelHMMStateState.
     *
     * @return the name of the acoustic model
     */
    public String getModelName();

    /**
     * Returns the token stack of this ParallelUnitState.
     *
     * @return the token stack
     */
    public TokenStack getTokenStack();
}


