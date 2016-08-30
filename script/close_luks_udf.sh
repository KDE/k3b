#!/bin/bash

dirr=`dirname $0`
cd $dirr

#### SAFETY: ABORT ON FAILED source ./initluks.include ####
source ./initluks.include
result=$?
if test "$result" == "0"; then
	echo initluks.include ran successfully 1>&2
elif test "$result" != "0"; then
	echo FATAL ERROR: initluks.include failed to run 1>&2
	echo " Probably a directory error, exiting..." 1>&2
	exit 1
elif test "$result" != "0"; then
	echo FATAL ERROR: Unknown error trying to run initluks.include, exiting... 1>&2
	exit 1
fi

##############################################################################################

#### SAFETY: ABORT ON EMPTY $exe_dir ####
if test "$exe_dir" == ""; then
	echo 1>&2
	echo Error: No exec dir. Run through init script. 1>&2
	echo 1>&2
	exit
fi

#### SAFETY: ABORT IF NO EXECUTABLE make_luks_udf.sh IN CURRENT DIR ####
if test -e make_luks_udf.sh; then
	echo -n 1>&2
else
	echo 1>&2
	echo Bad directory `pwd`, investigate!  1>&2
	echo 1>&2
	exit 1
fi

imgloop=""
imgloop=$1

echo 1>&2

if test "$imgloop" == "dvd" || test "$imgloop" == "blu"; then
	echo WARNING, arg1 is $imgloop, but it must be a loop device  1>&2
	echo Continuing the rest of the close  1>&2
	imgloop=""
fi

if test "$imgloop" == ""; then
	echo WARNING, NO IMAGE LOOP ARGUMENT, FIX IT LATER MANUALLY...  1>&2
fi

echo 1>&2

echo "Unmounting >$udfmountpoint<"  1>&2
sudo umount $udfmountpoint

echo "cryptsetup luksClosing >$mapperdev<"  1>&2
sudo cryptsetup luksClose $mapperdev

if test "$imgloop" == ""; then
	echo 1>&2
	echo "Your command lacked a loop device to close"  1>&2
	echo "So you'll need to do it manually, after the fact,"
	echo "using sudo losetup -d. See the losetup man page."  1>&2
	echo "The loop device to close should be listed below:"  1>&2
	sudo losetup -a
	echo 1>&2
else
	echo "losetupping -d >$imgloop<"  1>&2
	sudo losetup -d $imgloop
	echo 1>&2
fi

echo To reopen image file $imgfile, use reopen_luks_udf.sh $disctype  1>&2
