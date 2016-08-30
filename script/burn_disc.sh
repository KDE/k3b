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
	if udisks --show-info $discdev | grep -q $pat > /dev/null; then
		go_on=false
	fi
	echo -n "*"
	sleep 1
done
growisofs -dvd-compat -Z $discdev=$imgfile
