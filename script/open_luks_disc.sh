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

eject -t $discdev


go_on=true
pat="^[[:space:]]*has[[:space:]]\+media:[[:space:]]\+1"
echo "Waiting for disc to completely load..."
while $go_on; do
	if udisks --show-info $discdev | grep $pat > /dev/null; then
		go_on=false
	fi
	echo -n "*"
	sleep 1
done
echo

go_on=true
pat="^Cipher[[:space:]]\+name:[[:space:]]\+[^[:space:]]"
echo "Waiting for LUKS filesystem to register..."
while $go_on; do
	if sudo cryptsetup luksDump $discdev | grep $pat > /dev/null; then
		go_on=false
	fi
	echo -n "*"
	sleep 1
done
echo


command="sudo cryptsetup luksOpen $discdev $discmappername"
echo About to $command
$command
echo

command="sudo mount $discmapperdev $disc_mountpoint"
echo About to $command
$command
echo
echo
echo Read files from $disc_mountpoint
echo Undo with close_luks_disc.sh
echo
