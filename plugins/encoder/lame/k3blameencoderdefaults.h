/* 
 *
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_LAME_ENCODER_DEFAULTS_H_
#define _K3B_LAME_ENCODER_DEFAULTS_H_

const char DEFAULT_MODE[] = "stereo";
const bool DEFAULT_MANUAL_BITRATE = false;
const bool DEFAULT_VBR = false;
const int DEFAULT_CONSTANT_BITRATE = 128;
const int DEFAULT_MAXIMUM_BITRATE = 224;
const int DEFAULT_MINIMUM_BITRATE = 32;
const int DEFAULT_AVERAGE_BITRATE = 128;
const bool DEFAULT_USE_MAXIMUM_BITRATE = false;
const bool DEFAULT_USE_MINIMUM_BITRATE = false;
const bool DEFAULT_USE_AVERAGE_BITRATE = true;
const int DEFAULT_QUALITY_LEVEL = 5;
const bool DEFAULT_COPYRIGHT = false;
const bool DEFAULT_ORIGINAL = true;
const bool DEFAULT_ISO_COMPLIANCE = false;
const bool DEFAULT_ERROR_PROTECTION = false;
const int DEFAULT_ENCODER_QUALITY = 7;

#endif // _K3B_LAME_ENCODER_DEFAULTS_H_