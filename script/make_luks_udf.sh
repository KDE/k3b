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

echo 1>&2

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

echo Truncating $imgfile to size $imgsize 1>&2
truncate -s $imgsize  $imgfile

echo Doing losetup to loop $imgloop, file $imgfile 1>&2
sudo losetup $imgloop $imgfile

echo Doing cryptsetup luksFormat on loop $imgloop, the loop mount of $imgfile 1>&2
echo Strongarming --cipher aes-xts-plain64, as per  1>&2
echo http://www.jakoblell.com/blog/2013/12/22/practical-malleability-attack-against-cbc-encrypted-luks-partitions/#toc-4 1>&2

#### SAFETY: WARN THE USER. ####
mycommand="sudo cryptsetup luksFormat --cipher aes-xts-plain64 $imgloop"
sudo losetup -j $imgloop
echo 1>&2
echo "WARNING: Next command is: $mycommand" 1>&2
echo "$img loop file connection on next line:" 1>&2
sudo losetup $imgloop
echo "If the command is wrong it can cause data loss." 1>&2
echo "If that command's not exactly what you want to do, either" 1>&2
echo "Ctrl+C out right now, or type "no" when asked if it's OK." 1>&2
echo
$mycommand

#### SAFETY: ABORT IF $imgloop NOT LUKS FORMATTED
if ! sudo cryptsetup isLuks $imgloop; then
	echo 1>&2
	echo Quitting at User request. 1>&2
	exit 1
fi

echo Doing cryptsetup luksOpen on loop $imgloop, as $mappername 1>&2
sudo cryptsetup luksOpen $imgloop $mappername

#### SAFETY: WARN THE USER. ####
mycommand="sudo mkudffs $mapperdev"
#sudo cryptsetup luksDump $mapperdev
echo 1>&2
echo "WARNING: Next command is: $mycommand" 1>&2
echo "If the command is wrong it can cause data loss." 1>&2
echo "If that command's not exactly what you want to do, either" 1>&2
echo "Ctrl+C out right now, because you'll get no other opportunity not to UDF format $mapperdev." 1>&2
echo "If you really want to do this, hit Enter." 1>&2
read junk
$mycommand

echo Mounting $mapperdev to $udfmountpoint 1>&2
sudo mount $mapperdev $udfmountpoint

echo Chowning $USER:$USER $udfmountpoint 1>&2
sudo chown -R $USER:$USER $udfmountpoint

echo 1>&2
echo Encrypted loop device is at $mapperdev or $imgfloop. 1>&2
echo 1>&2

echo Write files to $udfmountpoint 1>&2
echo BE SURE TO UNDO THIS STUFF WITH close_luks_udf.sh $imgloop $disctype 1>&2
##############################################################################################

