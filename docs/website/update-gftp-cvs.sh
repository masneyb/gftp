#!/bin/sh
# $Id$

GFTP_DIR=~/gftp
LATEST_TAR=gftp-test.tar
DEST_DIR=/home/gftp/public_html

cd $GFTP_DIR
$GFTP_DIR/cvsclean
rm $GFTP_DIR/po/*.{po,gmo} $GFTP_DIR/po/ChangeLog $GFTP_DIR/ChangeLog $GFTP_DIR/configure.in $GFTP_DIR/Makefile.am
cvs -z3 up
VERSION=`cat $GFTP_DIR/configure.in | grep ^AM_INIT_AUTOMAKE | awk -F, '{print $2}' | sed s/\)//`
touch $GFTP_DIR/NEWS $GFTP_DIR/AUTHORS
$GFTP_DIR/autogen.sh
make dist
mv $GFTP_DIR/gftp-$VERSION.tar.gz $GFTP_DIR/$LATEST_TAR.gz
gunzip $GFTP_DIR/$LATEST_TAR.gz
bzip2 $GFTP_DIR/$LATEST_TAR
mv $LATEST_TAR.bz2 $DEST_DIR
