/**
 * Copyright 1999-2007 Carnegie Mellon University.
 * Portions Copyright 2002 Sun Microsystems, Inc.
 * All Rights Reserved.  Use is subject to license terms.
 * <p/>
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 * <p/>
 * <p/>
 * User: Garrett Weinberg
 * Date: Jan 13, 2007
 * Time: 9:16:28 PM
 */

package edu.cmu.sphinx.tools.riddler.types;

import java.util.Calendar;

/**
 * Describes a given corpus
 */
public class CorpusDescriptor {

    String[] metadataKeys;
    String[] metadataValues;
    /**
     * java.util.Date is not currently supported (by JAX-RPC 1.1) as an over-the-wire datatype
     */
    Calendar collectDate;

    public CorpusDescriptor(Calendar collectDate, String[] metadataKeys, String[] metadataValues) {
        this.metadataKeys = metadataKeys;
        this.metadataValues = metadataValues;
        this.collectDate = collectDate;
    }

    public String[] getMetadataKeys() {
        return metadataKeys;
    }

    public void setMetadataKeys(String[] metadataKeys) {
        this.metadataKeys = metadataKeys;
    }

    public String[] getMetadataValues() {
        return metadataValues;
    }

    public void setMetadataValues(String[] metadataValues) {
        this.metadataValues = metadataValues;
    }

    public Calendar getCollectDate() {
        return collectDate;
    }

    public void setCollectDate(Calendar collectDate) {
        this.collectDate = collectDate;
    }
}
