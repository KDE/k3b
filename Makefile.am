if with_k3bsetup1
  K3BSETUPDIR=k3bsetup
endif

EXTRA_DIST = AUTHORS COPYING ChangeLog INSTALL README TODO k3b.lsm admin configure.in.in

AUTOMAKE_OPTIONS = foreign 1.5

#include admin/deps.am

SUBDIRS = libk3bdevice libk3b src kioslaves plugins $(K3BSETUPDIR) dvd+rw-tools
