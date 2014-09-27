check_PROGRAMS = test_bitvec

TESTS = test_bitvec
AM_CFLAGS =-I$(top_srcdir) \
	-DTESTDATADIR=\"$(top_srcdir)/test/data\"

noinst_HEADERS = test_macros.h

LDADD = ${top_builddir}/libmultisphinx.la
