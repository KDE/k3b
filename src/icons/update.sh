#!/bin/sh

PREFIX=svn://anonsvn.kde.org/home/kde/trunk/kdesupport/oxygen-icons
SIZES="16 22 32 48 64 128 scalable"

function update_icon() {
	if [ ${3} = "scalable" ]
	then
		svn export ${PREFIX}/${3}/${2}/${1}.svgz ${2}/hisc-${2}-${1}.svgz
	else
		svn export ${PREFIX}/${3}x${3}/${2}/${1}.png ${2}/hi${3}-${2}-${1}.png
	fi
}

function update_icons() {
	for x in ${SIZES}; do
		update_icon ${1} ${2} ${x}
	done
}

update_icons "application-x-k3b" mimetypes
update_icons "tools-rip-audio-cd" actions
update_icons "tools-rip-video-cd" actions
update_icons "tools-rip-video-dvd" actions
update_icons "k3b" apps
update_icons "media-optical-data" devices
update_icons "media-optical-audio" devices
update_icons "media-optical-mixed-cd" devices
update_icons "media-optical-video" devices
update_icons "media-optical-cd-video" devices
update_icons "media-optical-dvd-video" devices
