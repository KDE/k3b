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
if test -x ./make_luks_udf.sh; then
	echo -n 1>&2
else
	echo 1>&2
	echo Bad directory `pwd`, investigate!  1>&2
	echo 1>&2
	exit 1
fi


imgloop=`sudo losetup -f`
result=$?
if test "$result" != 0; then
	echo FATAL ERROR, losetup -f failed with return code $result, aborting. 1>&2
	exit 1
fi

echo Doing losetup to loop $imgloop, file $imgfile 1>&2
sudo losetup $imgloop $imgfile


echo Doing cryptsetup luksOpen on loop $imgloop, as $mappername 1>&2
sudo cryptsetup luksOpen $imgloop $mappername

echo Mounting $mapperdev to $udfmountpoint 1>&2
sudo mount $mapperdev $udfmountpoint

echo Chowning $USER:$USER $udfmountpoint 1>&2
sudo chown -R $USER:$USER $udfmountpoint

echo 1>&2
echo Encrypted loop device is REOPENED at $mapperdev or $imgfloop. 1>&2
echo 1>&2

echo Write files to $udfmountpoint 1>&2
echo BE SURE TO UNDO THIS STUFF WITH close_luks_udf.sh $imgloop $disctype 1>&2
##############################################################################################
