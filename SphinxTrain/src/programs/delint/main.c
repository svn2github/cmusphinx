/*********************************************************************
 *
 * $Header$
 *
 * Carnegie Mellon ARPA Speech Group
 *
 * Copyright (c) 1995 Carnegie Mellon University.
 * All rights reserved.
 *
 *********************************************************************
 *
 * File: main.c
 * 
 * Description: 
 *	This smooths the counts produced by bw(1).
 * 
 * Author: 
 *	Eric Thayer (eht@cs.cmu.edu)
 * 
 *********************************************************************/

#include "interp_fn.h"
#include "parse_cmd_ln.h"

/* SPHINX-III includes */
#include <s3/cmd_ln.h>
#include <s3/acmod_set.h>
#include <s3/model_def_io.h>
#include <s3/s3tmat_io.h>
#include <s3/s3mixw_io.h>
#include <s3/s3gau_io.h>
#include <s3/gauden_io.h>
#include <s3/gauden.h>
#include <s3/matrix.h>
#include <s3/prim_type.h>	/* define int32, etc. */
#include <s3/ckd_alloc.h>	/* memory allocation */
#include <s3/err.h>		/* define E_ERROR, E_INFO, etc */

#include <sys_compat/file.h>

/* system wide includes */
#include <assert.h>	/* for assert() macro */
#include <math.h>	/* for fabs(), etc */

static int rd_interp_wr(void);


int
compute_mixw_lambda(float32 **lambda,
		    float64 **lambda_acc,

		    unsigned char *converged,

		    uint32 n_cd_state,
		    uint32 n_ci_state,
		    uint32 **ci_mixw,

		    float32 ***mixw_acc_a,
		    float32 ***mixw_acc_b,
		    float64 *dnom,

		    uint32 n_mixw,
		    uint32 n_feat,
		    uint32 n_gau)
{
    float64 tt_uni;
    float64 tt_ci;
    float64 tt_cd;
    float32 **acc_a;
    float32 **acc_b;
    float32 **ci_acc_b;
    float64 ci_dnom;
    float32 uniform_prob = 1.0 / n_gau;

    float64 norm;

    uint32 i, cd_i, j, k, l;

    for (i = 0; i < n_cd_state; i++) {
	if (converged[i])
	    continue;	/* skip the ones which have converged */

	cd_i = i + n_ci_state;

	acc_a = mixw_acc_a[cd_i];
	acc_b = mixw_acc_b[cd_i];
	
	for (j = 0; ci_mixw[i][j] != TYING_NO_ID; j++) {

	    ci_acc_b = mixw_acc_b[ci_mixw[i][j]];
	    ci_dnom = dnom[ci_mixw[i][j]];
	    
	    /* for all non-zero weight counts, accumulate lambda */
	    for (k = 0; k < n_feat; k++) {
		for (l = 0; l < n_gau; l++) {
		    if (acc_a[k][l] > MIN_IEEE_NORM_POS_FLOAT32) {	/* non-vanishing codeword */
			tt_uni = lambda[i][DIST_UNIFORM] * uniform_prob;
			tt_ci  = lambda[i][DIST_CI] * (ci_acc_b[k][l] * ci_dnom); /* i.e. CI lambda * CI prob */
			tt_cd  = lambda[i][DIST_CD] * (acc_b[k][l] * dnom[cd_i]); /* i.e. CD lambda * CD prob */
			assert((tt_uni + tt_ci + tt_cd) > 0);
		    
			norm = 1.0 / (tt_uni + tt_ci + tt_cd);
			
			if (tt_uni > MIN_IEEE_NORM_POS_FLOAT32) {
			    tt_uni *= norm;
			    lambda_acc[i][DIST_UNIFORM] += tt_uni * acc_a[k][l];
			}
			if (tt_ci > MIN_IEEE_NORM_POS_FLOAT32) {
			    tt_ci *= norm;
			    lambda_acc[i][DIST_CI] += tt_ci * acc_a[k][l];
			}
			if (tt_cd > MIN_IEEE_NORM_POS_FLOAT32) {
			    tt_cd *= norm;
			    lambda_acc[i][DIST_CD] += tt_cd * acc_a[k][l];
			}
		    }
		}
	    }
	}
    }
    return S3_SUCCESS;
}

#define CON_TH			0.0001

int32
smooth_mixw(float32 ****out_mixw,
	    
	    float32 ***mixw_acc_a,
	    float32 ***mixw_acc_b,

	    uint32 n_mixw,
	    uint32 n_feat,
	    uint32 n_gau,

	    model_def_t *mdef
	    )
{
    uint32 i, j, k;
    uint32 n_base, cd_start, n_phone;
    uint32 n_ci_state;
    uint32 n_cd_state;
    uint32 n_state;
    uint32 max_state_pm;
    float64 *dnom_a;
    float64 *dnom_b;
    float64 *dnom;
    float64 sum_a;
    float64 sum_b;
    float64 sum;
    float32 **lambda;
    float64 **lambda_acc;
    uint32 **ci_mixw;
    uint32 *ci_state;
    uint32 max_iter;
    uint32 tt;
    unsigned char *conv_flag;
    uint32 converged;
    uint32 state_converged;
    acmod_id_t base;
    uint32 iter;
    float64 norm;
    float32 prior_lambda;
    model_def_entry_t *defn;
    float32 cilambda = *(float32 *)cmd_ln_access("-cilambda");
    uint32 n_conv_state;
    uint32 **n_tied;
    acmod_set_t *acmod_set;

    acmod_set = mdef->acmod_set;

    /*
     * Compute normalization factors
     */
    n_state = mdef->n_tied_state;

    dnom_a = (float64 *)ckd_calloc(n_state, sizeof(float64));
    dnom_b = (float64 *)ckd_calloc(n_state, sizeof(float64));
    dnom   = (float64 *)ckd_calloc(n_state, sizeof(float64));

    /* compute the count normalization factor for all tied states */
    for (i = 0; i < n_state; i++) {
	sum_a = sum_b = sum = 0;
	for (k = 0; k < n_gau; k++) {
	    sum_a += mixw_acc_a[i][0][k];
	    sum_b += mixw_acc_b[i][0][k];
	}

	if ((sum_a > 0) && (sum_b > 0)) {
	    dnom_a[i] = 1.0 / sum_a;
	    dnom_b[i] = 1.0 / sum_b;
	    dnom[i] = 1.0 / (sum_a + sum_b);
	}
	else {
	    E_WARN("Tied state %u never observed in the training corpus\n", i);
	    dnom_a[i] = dnom_b[i] = dnom[i] = 0.0;
	}
    }

    /*
     * Interpolate CI states
     */

    n_cd_state = mdef->n_tied_state - mdef->n_tied_ci_state;
    n_ci_state = mdef->n_tied_ci_state;

    if (n_cd_state == 0) {
	E_INFO("Only CI states.\n");
	E_INFO("    CI PDF == %.2f * CI + %.2f * UNIFORM\n", cilambda, (1.0 - cilambda));
    
	/* add together counts for context independent states */
	accum_3d(mixw_acc_a, mixw_acc_b,
		 n_ci_state,	/* run over n_ci_state states */
		 n_feat, n_gau);

	/* interpolate CI distributions with uniform distribution */
	interp_counts_3d_uniform(mixw_acc_a,
				 0,		/* start state */
				 n_ci_state,	/* run length */
				 n_feat, n_gau,
				 cilambda);

	*out_mixw = mixw_acc_a;
	
	ckd_free_3d((void ***)mixw_acc_b);
	
	return S3_SUCCESS;
    }

    /*
     * Interpolate all states
     */

    max_state_pm = mdef->max_n_state;

    E_INFO("Interpolating %u CD states\n", n_cd_state);
    E_INFO("%u states max/model\n", max_state_pm);
    
    ci_mixw = (uint32 **)ckd_calloc_2d(n_cd_state, max_state_pm+1,
				       sizeof(uint32));
    for (i = 0; i < n_cd_state; i++)
	for (j = 0; j < max_state_pm+1; j++)
	    ci_mixw[i][j] = TYING_NO_ID;

    n_tied = (uint32 **)ckd_calloc_2d(n_cd_state, max_state_pm+1,
				      sizeof(uint32));
    for (i = 0; i < n_cd_state; i++)
	for (j = 0; j < max_state_pm+1; j++)
	    n_tied[i][j] = TYING_NO_ID;
    
    cd_start = n_base = acmod_set_n_ci(acmod_set);
    n_phone = acmod_set_n_acmod(acmod_set);

    defn = mdef->defn;
    
    /* for each cd state, find the id's of the associated ci states */
    for (i = cd_start; i < n_phone; i++) {
	base = acmod_set_base_phone(acmod_set, i);
	for (j = 0; j < defn[i].n_state; j++) {
	    if (defn[i].state[j] < n_ci_state) {
		/* This is a ci state, so skip it */

		continue;
	    }

	    if (defn[i].state[j] != TYING_NON_EMITTING) {
		tt = defn[i].state[j] - n_ci_state;

		ci_state = ci_mixw[tt];
		
		for (k = 0; (k < max_state_pm) && (ci_state[k] != TYING_NO_ID); k++) {
		    if (ci_state[k] == defn[base].state[j])
			break;	/* already on list */
		}

		if (ci_state[k] != defn[base].state[j]) {
		    /* not on list, so add */
		    ci_state[k] = defn[base].state[j];
		    n_tied[tt][k] = 0;
		}

		++n_tied[tt][k];	/* # of times CD and CI occur in same state position */
	    }
	}
    }
    
    lambda = (float32 **)ckd_calloc_2d(n_cd_state, N_DIST_TYPE,
					sizeof(float32));
    /* storage is returned zeroed */
    lambda_acc = (float64 **)ckd_calloc_2d(n_cd_state, N_DIST_TYPE,
					   sizeof(float64));

    for (i = 0; i < n_cd_state; i++) {
	/* biased initialization should help generally well trained models
	 * converge faster */
	lambda[i][(int)DIST_CD] = 0.6;
	lambda[i][(int)DIST_CI] = 0.3;
	lambda[i][(int)DIST_UNIFORM] = 0.1;
    }

    max_iter = *(const int *)cmd_ln_access("-maxiter");

    conv_flag = (unsigned char *)ckd_calloc(n_cd_state, sizeof(unsigned char));

    for (iter = 0, converged = FALSE, n_conv_state = 0;
	 (iter < max_iter) && (!converged);
	 iter++) {

	compute_mixw_lambda(lambda, lambda_acc, conv_flag,
			    n_cd_state, n_ci_state,
			    ci_mixw,
			    mixw_acc_b,
			    mixw_acc_a,
			    dnom_a,
			    n_mixw, n_feat, n_gau);

	compute_mixw_lambda(lambda, lambda_acc, conv_flag,
			    n_cd_state, n_ci_state,
			    ci_mixw,
			    mixw_acc_a,
			    mixw_acc_b,
			    dnom_b,
			    n_mixw, n_feat, n_gau);

	/* update lambdas and check for convergence */
	for (i = 0; i < n_cd_state; i++) {

	    if (conv_flag[i] == TRUE)	/* don't update converged states */
		continue;

	    norm = 0.0;
	    for (j = 0; j < N_DIST_TYPE; j++) {
		norm += lambda_acc[i][j];
	    }

	    if (norm > MIN_IEEE_NORM_POS_FLOAT32) {
		norm = 1.0 / norm;

		for (j = 0, state_converged = TRUE; j < N_DIST_TYPE; j++) {
		    prior_lambda = lambda[i][j];
		    lambda[i][j] = lambda_acc[i][j] * norm;

		    lambda_acc[i][j] = 0.0;

		    if (fabs(prior_lambda - lambda[i][j]) > CON_TH) {
			state_converged = converged = FALSE;
		    }
		}
		    
	    }
	    else {
		for (j = 0; j < N_DIST_TYPE; j++) {
		    lambda[i][j] = 1.0 / N_DIST_TYPE;
		}
		state_converged = TRUE;
	    }
		
	    if (state_converged) {
		conv_flag[i] = TRUE;
		++n_conv_state;
	    }
	}

	E_INFO("%u:%u:%u\n", iter, n_conv_state, n_cd_state);
    }

    if (!converged) {
	E_WARN("%u of %u states converged after %u iterations.\n",
	       n_conv_state, n_cd_state, iter);

    }

    printf("SUMMARY\n\n");
    printf(" State  Count A   Count B    Total    CD    CI    UNI   Cnv\n");
    printf("------ --------- --------- --------- ----- ----- ------ ---\n");
    for (i = 0; i < n_cd_state; i++) {

	j = n_ci_state + i;	/* CD tied state id */

	printf("%6u %.3e %.3e %.3e %.3f %.3f %.3f %s\n",
	       j,
	       (dnom_a[j] > 0 ? 1.0 / dnom_a[j] : 0.0),	/* count of partition A tied state j */
	       (dnom_b[j] > 0 ? 1.0 / dnom_b[j] : 0.0),	/* count of partition B tied state j */
	       (dnom[j] > 0 ? 1.0 / dnom[j] : 0.0),	/* count of partition A+B tied state j */
	       lambda[i][DIST_CD],
	       lambda[i][DIST_CI],
	       lambda[i][DIST_UNIFORM],
	       (conv_flag[i] ? "y" : "n"));
    }
    
    interp_mixw(out_mixw,
		mixw_acc_a, mixw_acc_b, dnom,
		lambda, cilambda,
		ci_mixw, n_tied,
		n_cd_state, n_ci_state,
		n_mixw, n_feat, n_gau);

    return S3_SUCCESS;
}

static int
interp(float32 ****out_mixw,

       model_def_t *mdef,

       float32 ***mixw_acc_a,
       float32 ***mixw_acc_b,
       uint32 n_mixw,
       uint32 n_feat,
       uint32 n_gau)
{
    int ret = S3_SUCCESS;

    if (smooth_mixw(out_mixw,
		    mixw_acc_a,
		    mixw_acc_b,
		    n_mixw,
		    n_feat,
		    n_gau,
		    mdef) != S3_SUCCESS) {
	ret = S3_ERROR;
    }

    return ret;
}

static int
rd_param(uint32 *idx,
	 const char **accum_dirs,

	 float32 ****out_mixw_acc,
	 uint32 *out_n_mixw,
	 uint32 *out_n_feat,
	 uint32 *out_n_gau)
{
    char fn[MAXPATHLEN+1];
    const char *accum_dir;
    uint32 i;

    i = *idx;

    accum_dir = accum_dirs[i];
    
    sprintf(fn, "%s/mixw_counts", accum_dir);

    E_INFO("Reading %s\n", fn);

    if (s3mixw_read(fn,
		    out_mixw_acc,
		    out_n_mixw,
		    out_n_feat,
		    out_n_gau) != S3_SUCCESS) {
	return S3_ERROR;
    }

    ++(*idx);

    return S3_SUCCESS;
}

static void
accum_param(float32 ***mixw_acc,

	    float32 ***mixw_acc_in,
	    uint32 n_mixw,
	    uint32 n_feat,
	    uint32 n_gau)
{
    if (mixw_acc_in) {
	accum_3d(mixw_acc, mixw_acc_in,
		 n_mixw, n_feat, n_gau);
    }
}

int
wr_param(float32 ***mixw_acc,
	 uint32 n_mixw,
	 uint32 n_feat,
	 uint32 n_density)
{
    /*
     * Write the parameters to files
     */

    E_INFO("Writing %s\n", cmd_ln_access("-mixwfn"));

    if (s3mixw_write(cmd_ln_access("-mixwfn"),
		     mixw_acc,
		     n_mixw,
		     n_feat,
		     n_density) != S3_SUCCESS) {
	return S3_ERROR;
    }

    return S3_SUCCESS;
}

static int
rd_interp_wr()
{
    model_def_t *mdef;

    float32 ***mixw_acc_in;
    float32 ***mixw_acc_a;
    float32 ***mixw_acc_b;
    float32 ***mixw;
    uint32 n_mixw;
    uint32 n_feat;
    uint32 n_gau;

    const char **accum_dir;
    uint32 i;

    if (cmd_ln_access("-feat")) {
	feat_set((const char *)cmd_ln_access("-feat"));
    }
    else {
	E_FATAL("Please set the feature set using -feat\n");
    }

    if (cmd_ln_access("-ceplen")) {
	feat_set_in_veclen(*(int32 *)cmd_ln_access("-ceplen"));
    }
    else {
	E_FATAL("Please set the source feature vector length using -ceplen\n");
    }

    if (model_def_read(&mdef,
		       cmd_ln_access("-moddeffn")) != S3_SUCCESS) {
	return S3_ERROR;
    }

    i = 0;
    accum_dir = (const char **)cmd_ln_access("-accumdirs");
    
    /* must be at least two accum dirs for interpolation */
    assert(accum_dir[i] != NULL);
    rd_param(&i, accum_dir,
	     &mixw_acc_a, &n_mixw, &n_feat, &n_gau);

    assert(accum_dir[i] != NULL);
    rd_param(&i, accum_dir,
	     &mixw_acc_b, &n_mixw, &n_feat, &n_gau);

    while (accum_dir[i] != NULL) {
	rd_param(&i, accum_dir,
		 &mixw_acc_in, &n_mixw, &n_feat, &n_gau);

	/* accumulate additional "a" counts into "a" buffers */
	accum_param(mixw_acc_a,
		    mixw_acc_in, n_mixw, n_feat, n_gau);

	ckd_free_3d((void ***)mixw_acc_in);

	/* must be an even # of accum dirs for interpolation */
	assert(accum_dir[i] != NULL);

	rd_param(&i, accum_dir,
		 &mixw_acc_in, &n_mixw, &n_feat, &n_gau);

	/* accumulate additional "b" counts into "b" buffers */
	accum_param(mixw_acc_b,
		    mixw_acc_in, n_mixw, n_feat, n_gau);
	
	ckd_free_3d((void ***)mixw_acc_in);
    }

    if (interp(&mixw,

	       mdef,

	       mixw_acc_a, mixw_acc_b,
	       n_mixw, n_feat, n_gau) != S3_SUCCESS) {
    }
    
    if (wr_param(mixw,
		 n_mixw, n_feat, n_gau) != S3_SUCCESS) {
	E_FATAL("Error writing parameters\n");
    }
    
    return S3_SUCCESS;
}

int
main(int argc, char *argv[])
{
    parse_cmd_ln(argc, argv);

    if (rd_interp_wr() != S3_SUCCESS)
	exit(1);
    
    return 0;
}

/*
 * Log record.  Maintained by RCS.
 *
 * $Log$
 * Revision 1.1  2000/09/24  21:38:31  awb
 * *** empty log message ***
 * 
 * Revision 1.14  97/07/16  11:36:22  eht
 * *** empty log message ***
 * 
 * Revision 1.13  1996/03/25  15:40:05  eht
 * Added ability to set input feature vector length
 *
 * Revision 1.12  1996/03/04  15:57:41  eht
 * Made some changes so that acmod_set conforms to its interface
 *
 * Revision 1.11  1996/01/26  18:07:00  eht
 * Use the feat module
 *
 * Revision 1.10  1995/12/15  18:37:07  eht
 * Added some type cases for memory alloc/free
 *
 * Revision 1.9  1995/10/10  12:53:35  eht
 * Changed to use <s3/prim_type.h>
 *
 * Revision 1.8  1995/10/09  15:30:33  eht
 * Removed __FILE__, __LINE__ arguments to ckd_alloc routines
 *
 * Revision 1.7  1995/10/05  12:55:06  eht
 * Deal w/ untrained tied states and change in acmod_set interface
 *
 * Revision 1.6  1995/09/08  19:11:14  eht
 * Updated to use new acmod_set module.  Prior to testing
 * on TI digits.
 *
 * Revision 1.5  1995/09/07  19:10:22  eht
 * Don't consider CI states when computing the set of
 * CI states associated with CD states.
 *
 * Revision 1.4  1995/08/15  13:39:57  eht
 * Compute the number of times a seno appears with a given CI distribution.
 *
 * Revision 1.3  1995/08/10  20:29:40  eht
 * Yet another development version
 *
 * Revision 1.2  1995/08/09  00:38:05  eht
 * Another development version
 *
 * Revision 1.1  1995/06/02  20:56:53  eht
 * Initial revision
 *
 *
 */
