/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2008 Carnegie Mellon University.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * This work was supported in part by funding from the Defense Advanced 
 * Research Projects Agency and the National Science Foundation of the 
 * United States of America, and the CMU Sphinx Speech Consortium.
 *
 * THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND 
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
 * NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ====================================================================
 *
 */

/* System headers. */
#include <stdio.h>
#include <assert.h>

/* SphinxBase headers. */
#include <sphinxbase/err.h>
#include <sphinxbase/strfuncs.h>
#include <sphinxbase/filename.h>
#include <sphinxbase/pio.h>

/* Local headers. */
#include "cmdln_macro.h"
#include "pocketsphinx_internal.h"
#include "fwdtree_search.h"
#include "fwdflat_search.h"

static const arg_t ps_args_def[] = {
    POCKETSPHINX_OPTIONS,
    CMDLN_EMPTY_OPTION
};

/* I'm not sure what the portable way to do this is. */
static int
file_exists(const char *path)
{
    FILE *tmp;

    tmp = fopen(path, "rb");
    if (tmp) fclose(tmp);
    return (tmp != NULL);
}

static int
hmmdir_exists(const char *path)
{
    FILE *tmp;
    char *mdef = string_join(path, "/mdef", NULL);

    tmp = fopen(mdef, "rb");
    if (tmp) fclose(tmp);
    ckd_free(mdef);
    return (tmp != NULL);
}

static void
ps_add_file(cmd_ln_t *config, const char *arg,
            const char *hmmdir, const char *file)
{
    char *tmp = string_join(hmmdir, "/", file, NULL);

    if (cmd_ln_str_r(config, arg) == NULL && file_exists(tmp))
        cmd_ln_set_str_r(config, arg, tmp);
    ckd_free(tmp);
}

void
ps_init_defaults(cmd_ln_t *config)
{
    char const *hmmdir, *lmfile, *dictfile;

    /* Disable memory mapping on Blackfin (FIXME: should be uClinux in general). */
#ifdef __ADSPBLACKFIN__
    E_INFO("Will not use mmap() on uClinux/Blackfin.");
    cmd_ln_set_boolean_r(config, "-mmap", FALSE);
#endif

#ifdef MODELDIR
    /* Set default acoustic and language models. */
    hmmdir = cmd_ln_str_r(config, "-hmm");
    lmfile = cmd_ln_str_r(config, "-lm");
    dictfile = cmd_ln_str_r(config, "-dict");
    if (hmmdir == NULL && hmmdir_exists(MODELDIR "/hmm/en_US/hub4wsj_sc_8k")) {
        hmmdir = MODELDIR "/hmm/en_US/hub4wsj_sc_8k";
        cmd_ln_set_str_r(config, "-hmm", hmmdir);
    }
    if (lmfile == NULL && !cmd_ln_str_r(config, "-fsg")
        && !cmd_ln_str_r(config, "-jsgf")
        && file_exists(MODELDIR "/lm/en_US/hub4.5000.DMP")) {
        lmfile = MODELDIR "/lm/en_US/hub4.5000.DMP";
        cmd_ln_set_str_r(config, "-lm", lmfile);
    }
    if (dictfile == NULL && file_exists(MODELDIR "/lm/en_US/cmu07a.dic")) {
        dictfile = MODELDIR "/lm/en_US/cmu07a.dic";
        cmd_ln_set_str_r(config, "-dict", dictfile);
    }

    /* Expand acoustic and language model filenames relative to installation path. */
    if (hmmdir && !path_is_absolute(hmmdir) && !hmmdir_exists(hmmdir)) {
        char *tmphmm = string_join(MODELDIR "/hmm/", hmmdir, NULL);
        cmd_ln_set_str_r(config, "-hmm", tmphmm);
        ckd_free(tmphmm);
    }
    if (lmfile && !path_is_absolute(lmfile) && !file_exists(lmfile)) {
        char *tmplm = string_join(MODELDIR "/lm/", lmfile, NULL);
        cmd_ln_set_str_r(config, "-lm", tmplm);
        ckd_free(tmplm);
    }
    if (dictfile && !path_is_absolute(dictfile) && !file_exists(dictfile)) {
        char *tmpdict = string_join(MODELDIR "/lm/", dictfile, NULL);
        cmd_ln_set_str_r(config, "-dict", tmpdict);
        ckd_free(tmpdict);
    }
#endif

    /* Get acoustic model filenames and add them to the command-line */
    if ((hmmdir = cmd_ln_str_r(config, "-hmm")) != NULL) {
        ps_add_file(config, "-mdef", hmmdir, "mdef");
        ps_add_file(config, "-mean", hmmdir, "means");
        ps_add_file(config, "-var", hmmdir, "variances");
        ps_add_file(config, "-tmat", hmmdir, "transition_matrices");
        ps_add_file(config, "-mixw", hmmdir, "mixture_weights");
        ps_add_file(config, "-sendump", hmmdir, "sendump");
        ps_add_file(config, "-fdict", hmmdir, "noisedict");
        ps_add_file(config, "-lda", hmmdir, "feature_transform");
        ps_add_file(config, "-featparams", hmmdir, "feat.params");
        ps_add_file(config, "-senmgau", hmmdir, "senmgau");
    }
}

int
ps_reinit(ps_decoder_t *ps, cmd_ln_t *config)
{
    /* No longer does anything because we will add words dynamically
     * (really, we will) */
    return 0;
}

ps_decoder_t *
ps_init(cmd_ln_t *config)
{
    ps_decoder_t *ps = NULL;
    acmod_t *acmod2 = NULL;
    dict_t *dict = NULL;
    dict2pid_t *d2p = NULL;

    ps = ckd_calloc(1, sizeof(*ps));
    ps->refcount = 1;

    ps->config = config;

#ifndef _WIN32_WCE
    /* Set up logging. */
    if (cmd_ln_str_r(ps->config, "-logfn"))
        err_set_logfile(cmd_ln_str_r(ps->config, "-logfn"));
#endif
    err_set_debug_level(cmd_ln_int32_r(ps->config, "-debug"));
    ps->mfclogdir = cmd_ln_str_r(ps->config, "-mfclogdir");
    ps->rawlogdir = cmd_ln_str_r(ps->config, "-rawlogdir");
    ps->senlogdir = cmd_ln_str_r(ps->config, "-senlogdir");

    /* Fill in some default arguments. */
    ps_init_defaults(ps->config);

    /* Logmath computation.  We share this between all acmods and
     * search models, otherwise the scores will never make sense.
     * This might change. */
    ps->lmath = logmath_init
        ((float64)cmd_ln_float32_r(ps->config, "-logbase"), 0,
         cmd_ln_boolean_r(ps->config, "-bestpath"));

    /* FIXME: For the time being we will just clone a single acmod
     * between search passes.  We may use different models in the
     * future (would require good posterior probability calculation). */
    if ((ps->acmod = acmod_init(ps->config, ps->lmath, NULL, NULL)) == NULL)
        goto error_out;

    /* FIXME: For the time being we share a single dict (and dict2pid)
     * between search passes, but this will change in the future. */
    if ((dict = dict_init(config, ps->acmod->mdef)) == NULL)
        goto error_out;
    if ((d2p = dict2pid_build(ps->acmod->mdef, dict)) == NULL)
        goto error_out;

    /* FIXME: For the time being hardcode fwdflat and fwdtree as the
     * two searches. */
    ps->fwdtree = fwdtree_search_init(config, ps->acmod, dict, d2p);
    acmod2 = acmod_copy(ps->acmod);
    ps->fwdflat = fwdflat_search_init(config, acmod2, dict, d2p,
                                      ((fwdtree_search_t *)ps->fwdtree)->bptbl);

    /* Release pointers to things now owned by the searches. */
    acmod_free(acmod2);
    dict_free(dict);
    dict2pid_free(d2p);

    /* Initialize performance timer (but each search has its own). */
    ps->perf.name = "decode";
    ptmr_init(&ps->perf);

    return ps;
error_out:
    acmod_free(acmod2);
    dict_free(dict);
    dict2pid_free(d2p);
    ps_free(ps);
    return NULL;
}


arg_t const *
ps_args(void)
{
    return ps_args_def;
}

ps_decoder_t *
ps_retain(ps_decoder_t *ps)
{
    ++ps->refcount;
    return ps;
}

int
ps_free(ps_decoder_t *ps)
{
    if (ps == NULL)
        return 0;
    if (--ps->refcount > 0)
        return ps->refcount;

    ps_search_free(ps->fwdtree);
    ps_search_free(ps->fwdflat);
    acmod_free(ps->acmod);
    logmath_free(ps->lmath);
    cmd_ln_free_r(ps->config);
    ckd_free(ps->uttid);
    ckd_free(ps);
    return 0;
}

char const *
ps_get_uttid(ps_decoder_t *ps)
{
    return ps->uttid;
}

cmd_ln_t *
ps_get_config(ps_decoder_t *ps)
{
    return ps->config;
}

logmath_t *
ps_get_logmath(ps_decoder_t *ps)
{
    return ps->lmath;
}

fe_t *
ps_get_fe(ps_decoder_t *ps)
{
    return ps->acmod->fe;
}

feat_t *
ps_get_feat(ps_decoder_t *ps)
{
    return ps->acmod->fcb;
}

int
ps_decode_raw(ps_decoder_t *ps, FILE *rawfh,
              char const *uttid, long maxsamps)
{
    long total, pos;

    ps_start_utt(ps, uttid);
    /* If this file is seekable or maxsamps is specified, then decode
     * the whole thing at once. */
    if (maxsamps != -1 || (pos = ftell(rawfh)) >= 0) {
        int16 *data;

        if (maxsamps == -1) {
            long endpos;
            fseek(rawfh, 0, SEEK_END);
            endpos = ftell(rawfh);
            fseek(rawfh, pos, SEEK_SET);
            maxsamps = endpos - pos;
        }
        data = ckd_calloc(maxsamps, sizeof(*data));
        total = fread(data, sizeof(*data), maxsamps, rawfh);
        ps_process_raw(ps, data, total, FALSE, TRUE);
        ckd_free(data);
    }
    else {
        /* Otherwise decode it in a stream. */
        total = 0;
        while (!feof(rawfh)) {
            int16 data[256];
            size_t nread;

            nread = fread(data, sizeof(*data), sizeof(data)/sizeof(*data), rawfh);
            ps_process_raw(ps, data, nread, FALSE, FALSE);
            total += nread;
        }
    }
    ps_end_utt(ps);
    return total;
}

int
ps_start_utt(ps_decoder_t *ps, char const *uttid)
{
    ptmr_reset(&ps->perf);
    ptmr_start(&ps->perf);

    if (uttid) {
        ckd_free(ps->uttid);
        ps->uttid = ckd_salloc(uttid);
    }
    else {
        char nuttid[16];
        ckd_free(ps->uttid);
        sprintf(nuttid, "%09u", ps->uttno);
        ps->uttid = ckd_salloc(nuttid);
        ++ps->uttno;
    }

    /* Start logging features and audio if requested. */
    /* FIXME: In theory ps->acmod is owned by fwdflat search so we
     * shouldn't be screwing with it here. */
    if (ps->mfclogdir) {
        char *logfn = string_join(ps->mfclogdir, "/",
                                  ps->uttid, ".mfc", NULL);
        FILE *mfcfh;
        E_INFO("Writing MFCC log file: %s\n", logfn);
        if ((mfcfh = fopen(logfn, "wb")) == NULL) {
            E_ERROR_SYSTEM("Failed to open MFCC log file %s", logfn);
            ckd_free(logfn);
            return -1;
        }
        ckd_free(logfn);
        acmod_set_mfcfh(ps->acmod, mfcfh);
    }
    if (ps->rawlogdir) {
        char *logfn = string_join(ps->rawlogdir, "/",
                                  ps->uttid, ".raw", NULL);
        FILE *rawfh;
        E_INFO("Writing raw audio log file: %s\n", logfn);
        if ((rawfh = fopen(logfn, "wb")) == NULL) {
            E_ERROR_SYSTEM("Failed to open raw audio log file %s", logfn);
            ckd_free(logfn);
            return -1;
        }
        ckd_free(logfn);
        acmod_set_rawfh(ps->acmod, rawfh);
    }

    if (ps_search_start(ps->fwdtree) < 0)
        return -1;
    return ps_search_start(ps->fwdflat);
}

int
ps_process_raw(ps_decoder_t *ps,
               int16 const *data,
               size_t n_samples,
               int no_search,
               int full_utt)
{
    int n_searchfr = 0;

    while (n_samples) {
        int nfr;

        /* Process some data into features. */
        if ((nfr = acmod_process_raw(ps->acmod, &data,
                                     &n_samples, full_utt)) < 0)
            return nfr;

        /* Score and search as much data as possible */
        if (no_search)
            continue;
        /* FIXME: It doesn't work this way anymore. */
        if ((nfr = ps_search_step(ps->fwdtree)) < 0)
            return nfr;
        ps->n_frame += nfr;
        n_searchfr += nfr;
    }

    return n_searchfr;
}

int
ps_process_cep(ps_decoder_t *ps,
               mfcc_t **data,
               int32 n_frames,
               int no_search,
               int full_utt)
{
    int n_searchfr = 0;

    while (n_frames) {
        int nfr;

        /* Process some data into features. */
        if ((nfr = acmod_process_cep(ps->acmod, &data,
                                     &n_frames, full_utt)) < 0)
            return nfr;

        /* Score and search as much data as possible */
        if (no_search)
            continue;
        /* FIXME: It doesn't work this way anymore. */
        if ((nfr = ps_search_step(ps->fwdtree)) < 0)
            return nfr;
        ps->n_frame += nfr;
        n_searchfr += nfr;
    }

    return n_searchfr;
}

int
ps_end_utt(ps_decoder_t *ps)
{
    int rv;

    acmod_end_utt(ps->acmod);

    /* FIXME: It doesn't work this way anymore. */
    /* Search any remaining frames. */
    if ((rv = ps_search_step(ps->fwdtree)) < 0) {
        ptmr_stop(&ps->perf);
        return rv;
    }
    ps->n_frame += rv;
    /* Finish search. */
    if ((rv = ps_search_finish(ps->fwdtree)) < 0) {
        ptmr_stop(&ps->perf);
        return rv;
    }
    ps->n_frame += rv;
    ptmr_stop(&ps->perf);

    /* Log a backtrace if requested. */
    if (cmd_ln_boolean_r(ps->config, "-backtrace")) {
        char const *uttid, *hyp;
        ps_seg_t *seg;
        int32 score;

        hyp = ps_get_hyp(ps, &score, &uttid);
        E_INFO("%s: %s (%d)\n", uttid, hyp, score);
        E_INFO_NOFN("%-20s %-5s %-5s %-5s %-10s %-10s %-3s\n",
                    "word", "start", "end", "pprob", "ascr", "lscr", "lback");
        for (seg = ps_seg_iter(ps, &score); seg;
             seg = ps_seg_next(seg)) {
            char const *word;
            int sf, ef;
            int32 post, lscr, ascr, lback;

            word = ps_seg_word(seg);
            ps_seg_frames(seg, &sf, &ef);
            post = ps_seg_prob(seg, &ascr, &lscr, &lback);
            E_INFO_NOFN("%-20s %-5d %-5d %-1.3f %-10d %-10d %-3d\n",
                        word, sf, ef, logmath_exp(ps_get_logmath(ps), post), ascr, lscr, lback);
        }
    }
    return rv;
}

char const *
ps_get_hyp(ps_decoder_t *ps, int32 *out_best_score, char const **out_uttid)
{
    char const *hyp;

    ptmr_start(&ps->perf);
    /* FIXME: Nope. */
    hyp = ps_search_hyp(ps->fwdtree, out_best_score);
    if (out_uttid)
        *out_uttid = ps->uttid;
    ptmr_stop(&ps->perf);
    return hyp;
}

int32
ps_get_prob(ps_decoder_t *ps, char const **out_uttid)
{
    int32 prob;

    ptmr_start(&ps->perf);
    /* FIXME: Nope. */
    prob = ps_search_prob(ps->fwdtree);
    if (out_uttid)
        *out_uttid = ps->uttid;
    ptmr_stop(&ps->perf);
    return prob;
}

ps_seg_t *
ps_seg_iter(ps_decoder_t *ps, int32 *out_best_score)
{
    ps_seg_t *itor;

    ptmr_start(&ps->perf);
    /* FIXME: Nope. */
    itor = ps_search_seg_iter(ps->fwdtree, out_best_score);
    ptmr_stop(&ps->perf);
    return itor;
}

ps_seg_t *
ps_seg_next(ps_seg_t *seg)
{
    return ps_search_seg_next(seg);
}

char const *
ps_seg_word(ps_seg_t *seg)
{
    return seg->word;
}

void
ps_seg_frames(ps_seg_t *seg, int *out_sf, int *out_ef)
{
    if (out_sf) *out_sf = seg->sf;
    if (out_ef) *out_ef = seg->ef;
}

int32
ps_seg_prob(ps_seg_t *seg, int32 *out_ascr, int32 *out_lscr, int32 *out_lback)
{
    if (out_ascr) *out_ascr = seg->ascr;
    if (out_lscr) *out_lscr = seg->lscr;
    if (out_lback) *out_lback = seg->lback;
    return seg->prob;
}

void
ps_seg_free(ps_seg_t *seg)
{
    ps_search_seg_free(seg);
}

int
ps_get_n_frames(ps_decoder_t *ps)
{
    return ps->acmod->output_frame + 1;
}

void
ps_get_utt_time(ps_decoder_t *ps, double *out_nspeech,
                double *out_ncpu, double *out_nwall)
{
    int32 frate;

    frate = cmd_ln_int32_r(ps->config, "-frate");
    *out_nspeech = (double)ps->acmod->output_frame / frate;
    *out_ncpu = ps->perf.t_cpu;
    *out_nwall = ps->perf.t_elapsed;
}

void
ps_get_all_time(ps_decoder_t *ps, double *out_nspeech,
                double *out_ncpu, double *out_nwall)
{
    int32 frate;

    frate = cmd_ln_int32_r(ps->config, "-frate");
    *out_nspeech = (double)ps->n_frame / frate;
    *out_ncpu = ps->perf.t_tot_cpu;
    *out_nwall = ps->perf.t_tot_elapsed;
}

void
ps_search_init(ps_search_t *search, ps_searchfuncs_t *vt,
               cmd_ln_t *config, acmod_t *acmod, dict_t *dict,
               dict2pid_t *d2p)
{
    search->vt = vt;
    search->config = cmd_ln_retain(config);
    search->acmod = acmod_retain(acmod);
    if (d2p)
        search->d2p = dict2pid_retain(d2p);
    else
        search->d2p = NULL;
    if (dict) {
        search->dict = dict_retain(dict);
        search->start_wid = dict_startwid(dict);
        search->finish_wid = dict_finishwid(dict);
        search->silence_wid = dict_silwid(dict);
        search->n_words = dict_size(dict);
    }
    else {
        search->dict = NULL;
        search->start_wid = search->finish_wid = search->silence_wid = -1;
        search->n_words = 0;
    }
}

void
ps_search_base_reinit(ps_search_t *search, dict_t *dict,
                      dict2pid_t *d2p)
{
    dict_free(search->dict);
    dict2pid_free(search->d2p);
    /* FIXME: _retain() should just return NULL if passed NULL. */
    if (dict) {
        search->dict = dict_retain(dict);
        search->start_wid = dict_startwid(dict);
        search->finish_wid = dict_finishwid(dict);
        search->silence_wid = dict_silwid(dict);
        search->n_words = dict_size(dict);
    }
    else {
        search->dict = NULL;
        search->start_wid = search->finish_wid = search->silence_wid = -1;
        search->n_words = 0;
    }
    if (d2p)
        search->d2p = dict2pid_retain(d2p);
    else
        search->d2p = NULL;
}


void
ps_search_deinit(ps_search_t *search)
{
    cmd_ln_free_r(search->config);
    acmod_free(search->acmod);
    dict_free(search->dict);
    dict2pid_free(search->d2p);
    ckd_free(search->hyp_str);
}
