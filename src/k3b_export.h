/*
    This file is part of digikam project
    Copyright (c) 2005 Laurent Montel <montel@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#ifndef _K3B_EXPORT_H
#define _K3B_EXPORT_H

#include <kdeversion.h>

#if 0 /*KDE_IS_VERSION(3,3,90)*/

#include <kdemacros.h>

#define K3B_EXPORT KDE_EXPORT
#define LIBK3BDEVICE_EXPORT KDE_EXPORT
#define LIBK3BPROJECT_EXPORT KDE_EXPORT
#define LIBK3BTOOLS_EXPORT KDE_EXPORT
#define LIBK3BCORE_EXPORT KDE_EXPORT
#define LIBK3BJOB_EXPORT KDE_EXPORT
#define LIBK3BAUDIOCD_EXPORT KDE_EXPORT
#define LIBK3BPLUGIN_EXPORT KDE_EXPORT
#else
#define K3B_EXPORT
#define LIBK3BDEVICE_EXPORT
#define LIBK3BPROJECT_EXPORT
#define LIBK3BTOOLS_EXPORT
#define LIBK3BCORE_EXPORT
#define LIBK3BJOB_EXPORT
#define LIBK3BAUDIOCD_EXPORT
#define LIBK3BPLUGIN_EXPORT
#endif
#endif /* _K3B_EXPORT_H */

