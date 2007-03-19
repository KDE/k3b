/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 * Copyright (c) 2005 Laurent Montel <montel@kde.org>
 * Copyright (C) 2005-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_EXPORT_H_
#define _K3B_EXPORT_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef __KDE_HAVE_GCC_VISIBILITY
#define LIBK3B_NO_EXPORT __attribute__ ((visibility("hidden")))
#define LIBK3B_EXPORT __attribute__ ((visibility("default")))
#else
#define LIBK3B_NO_EXPORT
#define LIBK3B_EXPORT
#endif
 
#endif

