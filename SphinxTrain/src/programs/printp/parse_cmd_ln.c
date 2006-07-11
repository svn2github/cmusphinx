/* ====================================================================
 * Copyright (c) 1995-2000 Carnegie Mellon University.  All rights 
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
/*********************************************************************
 *
 * File: parse_cmd_ln.c
 * 
 * Description: 
 * 
 * Author: 
 * 	Eric H. Thayer (eht@cs.cmu.edu)
 *********************************************************************/

#include "parse_cmd_ln.h"

#include <s3/common.h>
#include <s3/s3.h>

#include <stdio.h>
#include <assert.h>

#include <sys/stat.h>
#include <sys/types.h>

/* defines, parses and (partially) validates the arguments
   given on the command line */

int
parse_cmd_ln(int argc, char *argv[])
{
  uint32      isHelp;
  uint32      isExample;

    const char helpstr[] =  
"Description: \n\
\n\
Display numerical values of resources generated by Sphinx \n\
Current we supoort the following formats \n\
\n\
-tmatfn : transition matrix \n\
\n\
-mixwfn : mixture weight file  \n\
\n\
-gaufn  : mean or variance \n\
\n\
-fullgaufn  : full covariance \n\
\n\
-gaucntn : sufficient statistics for mean and diagonal covariance \n\
\n\
-lambdafn : interpolation weight \n\
\n\
Currently, some parameters can be specified as intervals such as mixture weight.  \n\
\n\
You can also specified -sigfig the number of significant digits by you would like to see. \n\
\n\
and normalize the parameters by -norm";

    const char examplestr[] = 
"Example: \n\
\n\
Print the mean of a Gaussian: \n\
printp -gaufn mean \n\
\n\
Print the variance of a Gaussian: \n\
printp -gaufn var \n\
\n\
Print the sufficient statistic: \n\
printp -gaucntfn gaucnt: \n\
\n\
Print the mixture weights: \n\
printp -mixw mixw\n\
\n\
Print the interpolation weight: \n\
printp -lambdafn lambda ";

    static arg_def_t defn[] = {
	{ "-help",
	  CMD_LN_BOOLEAN,
	  CMD_LN_NO_VALIDATION,
	  "no",
	  "Shows the usage of the tool"},

	{ "-example",
	  CMD_LN_BOOLEAN,
	  CMD_LN_NO_VALIDATION,
	  "no",
	  "Shows example of how to use the tool"},

	{ "-tmatfn",
	  CMD_LN_STRING,
	  CMD_LN_NO_VALIDATION,
	  CMD_LN_NO_DEFAULT,
	  "The transition matrix parameter file name"},

	{ "-mixwfn",
	  CMD_LN_STRING,
	  CMD_LN_NO_VALIDATION,
	  CMD_LN_NO_DEFAULT,
	  "The mixture weight parameter file name"},

	{ "-mixws",
	  CMD_LN_INT32,
	  CMD_LN_NO_VALIDATION,
	  CMD_LN_NO_DEFAULT,
	  "Start id of mixing weight subinterval"},

	{ "-mixwe",
	  CMD_LN_INT32,
	  CMD_LN_NO_VALIDATION,
	  CMD_LN_NO_DEFAULT,
	  "End id of mixing weight subinterval"},

	{ "-gaufn",
	  CMD_LN_STRING,
	  CMD_LN_NO_VALIDATION,
	  CMD_LN_NO_DEFAULT,
	  "A Gaussian parameter file name (either for means or vars)"},

	{ "-fullgaufn",
	  CMD_LN_STRING,
	  CMD_LN_NO_VALIDATION,
	  CMD_LN_NO_DEFAULT,
	  "A full Gaussian covariance file name"},

	{ "-gaucntfn",
	  CMD_LN_STRING,
	  CMD_LN_NO_VALIDATION,
	  CMD_LN_NO_DEFAULT,
	  "A Gaussian parameter weighted vector file"},

	{ "-regmatcntfn",
	  CMD_LN_STRING,
	  CMD_LN_NO_VALIDATION,
	  CMD_LN_NO_DEFAULT,
	  "MLLR regression matrix count file"},

	{ "-moddeffn",
	  CMD_LN_STRING,
	  CMD_LN_NO_VALIDATION,
	  CMD_LN_NO_DEFAULT,
	  "The model definition file"},

	{ "-lambdafn",
	  CMD_LN_STRING,
	  CMD_LN_NO_VALIDATION,
	  CMD_LN_NO_DEFAULT,
	  "The interpolation weight file"},

	{ "-lambdamin",
	  CMD_LN_FLOAT32,
	  CMD_LN_NO_VALIDATION,
	  "0",
	  "Print int. wt. >= this"},

	{ "-lambdamax",
	  CMD_LN_FLOAT32,
	  CMD_LN_NO_VALIDATION,
	  "1",
	  "Print int. wt. <= this"},

	{ "-norm",
	  CMD_LN_BOOLEAN,
	  CMD_LN_NO_VALIDATION,
	  "yes",
	  "Print normalized parameters"},

	{ "-sigfig",
	  CMD_LN_INT32,
	  CMD_LN_NO_VALIDATION,
	  "4",
	  "Number of significant digits in 'e' notation" },

	{ NULL, CMD_LN_UNDEF, CMD_LN_NO_VALIDATION, CMD_LN_NO_DEFAULT, NULL }
    };

    cmd_ln_define(defn);

    if (argc == 1) {
	cmd_ln_print_definitions();
	exit(1);
    }

    cmd_ln_parse(argc, argv);

    if (cmd_ln_validate() == FALSE) {
	/* one or more command line arguments were
	   deemed invalid */
	exit(1);
    }

    isHelp    = *(uint32 *) cmd_ln_access("-help");
    isExample    = *(uint32 *) cmd_ln_access("-example");


    if(isHelp){
      printf("%s\n\n",helpstr);
    }

    if(isExample){
      printf("%s\n\n",examplestr);
    }

    if(isHelp || isExample){
      E_INFO("User asked for help or example.\n");
      exit(0);
    }
    if(!isHelp && !isExample){
      cmd_ln_print_configuration();
    }


    return 0;
}


/*
 * Log record.  Maintained by RCS.
 *
 * $Log$
 * Revision 1.7  2004/11/29  01:43:51  egouvea
 * Replaced handling of help or example so that user gets an INFO message instead of a scarier FATAL_ERROR
 * 
 * Revision 1.6  2004/08/07 21:29:48  arthchan2003
 * Update command line info and make tex happy
 *
 * Revision 1.5  2004/08/07 21:24:24  arthchan2003
 * add help and example to printp
 *
 * Revision 1.4  2004/07/21 19:17:26  egouvea
 * Changed the license terms to make it the same as sphinx2 and sphinx3.
 *
 * Revision 1.3  2001/04/05 20:02:31  awb
 * *** empty log message ***
 *
 * Revision 1.2  2000/09/29 22:35:14  awb
 * *** empty log message ***
 *
 * Revision 1.1  2000/09/24 21:38:32  awb
 * *** empty log message ***
 *
 * Revision 1.6  97/07/16  11:36:22  eht
 * *** empty log message ***
 * 
 * Revision 1.5  97/03/07  08:51:42  eht
 * - added -sigfig argument
 * - added -regmatcntfn argument
 * - added interpolation weight arguments
 * 
 * Revision 1.4  1996/01/30  17:06:44  eht
 * Include "-gaucntfn" argument and coalesce "-meanfn" and "-varfn"
 * into "-gaufn"
 *
 * Revision 1.3  1995/09/07  20:03:56  eht
 * Include defn of TRUE/FALSE for machines like HP's running HPUX
 *
 * Revision 1.2  1995/08/09  20:37:06  eht
 * *** empty log message ***
 *
 * Revision 1.1  1995/06/02  20:36:50  eht
 * Initial revision
 *
 *
 */
