#!/bin/bash

source initluks.include
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

command="sudo umount $disc_mountpoint"
echo About to $command
$command
echo

command="sudo cryptsetup luksClose $discmappername"
echo About to $command
$command
echo

command="eject $discdev"
echo About to $command
$command
echo
