#!/bin/bash

# make sure the toplevel subdirs are set properly. We cannot use 
# TOPLEVELDIRS becasue the order is important.

echo "Post processing K3b package in "`pwd`"..."

if [ -d "doc" ]; then
    echo "   --- Found doc subdir. Patching Makefile.am..."
    sed -i "s/SUBDIRS \?=/SUBDIRS = doc /g" Makefile.am
else
    echo "No doc subdir found."
fi

echo "   --- Fixing version field in libk3b/core/k3bcore.h"
sed -i "s/svn//g" libk3b/core/k3bcore.h

if [ -e tests ]; then
	echo "   --- Removing tests"
	rm -rf tests
fi

if [ -e scripts ]; then
	echo "   --- Removing scripts"
	rm -rf scripts
fi

if [ -e specs ]; then
	echo "   --- Removing specs"
	rm -rf specs
fi


# we have no toplevel files in the stable branch. So we have to create
# them in the i18n dir
DIR=`pwd`
cd ..
L10NDIR=`ls -d *i18n*`
if [ -e $L10NDIR ] && [ ! -e $L10NDIR/INSTALL ]; then
	echo "   --- Creating i18n toplevel files"
	echo "#MIN_CONFIG(3.2)" > $L10NDIR/configure.in.in
	touch $L10NDIR/ChangeLog
	echo "K3b translation files." > $L10NDIR/README
	touch $L10NDIR/AUTHORS
	touch $L10NDIR/NEWS		
	cp $DIR/COPYING $L10NDIR
	echo "./configure; make; make install (old school)" > $L10NDIR/INSTALL
fi
cd $DIR
