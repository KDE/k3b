#ifndef WORKMAN_H
#define WORKMAN_H
/*
 * $Id$
 *
 * This file is part of WorkMan, the civilized CD player library
 * (c) 1991-1997 by Steven Grimm (original author)
 * (c) by Dirk Försterling (current 'author' = maintainer)
 * The maintainer can be contacted by his e-mail address:
 * milliByte@DeathsDoor.com 
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * all-in-one libworkman include file.
 *
 */


/*
 * wm_config should always be included first
 */
#include "wm_config.h" 

#include "workman_defs.h"
#ifdef BUILD_CDDA
#include "wm_cdda.h"
#endif 
#include "wm_cddb.h"
#include "wm_cdinfo.h" 
#include "wm_cdrom.h" 
#include "wm_database.h" 
#include "wm_helpers.h" 
#include "wm_index.h" 
#include "wm_platform.h" 
#include "wm_scsi.h" 
#include "wm_struct.h"
#include "wm_cdtext.h"

#endif /* WORKMAN_H */

