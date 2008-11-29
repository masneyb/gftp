## Process this file with automake to produce Makefile.in

lib_LTLIBRARIES = libfsplib.la
libfsplib_la_SOURCES = fsplib.c lock.c
include_HEADERS = fsplib.h
noinst_HEADERS = lock.h 

# test program
noinst_PROGRAMS = test
test_SOURCES = test.c
test_LDADD   = libfsplib.la

# scons files
EXTRA_DIST = SConstruct SConscript
