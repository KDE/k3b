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

#ifndef _K3B_LAME_TYPES_H_
#define _K3B_LAME_TYPES_H_

#include <KI18n/KLocalizedString>

#include <lame/lame.h>


const int s_lame_bitrates[] = {
    32,
    40,
    48,
    56,
    64,
    80,
    96,
    112,
    128,
    160,
    192,
    224,
    256,
    320,
    0 // just used for the loops below
};


const int s_lame_presets[] = {
    56, // ABR for Voice, Radio, Mono, etc.
    90, //

    V6, // ~115 kbps
    V5, // ~130 kbps  | Portable - small size
    V4, // ~160 kbps

    V3, // ~175 kbps
    V2, // ~190 kbps  | HiFi - for home or quite listening
    V1, // ~210 kbps  |
    V0, // ~230 kbps

    320 // ABR 320 neary lossless for archiving (not recommended, use flac instead)
};


const int s_lame_preset_approx_bitrates[] = {
    56,
    90,
    115,
    130,
    160,
    175,
    190,
    210,
    230,
    320
};


const char* s_lame_preset_strings[] = {
    I18N_NOOP("Low quality (56 kbps)"),
    I18N_NOOP("Low quality (90 kbps)"),

    I18N_NOOP("Portable (average 115 kbps)"),
    I18N_NOOP("Portable (average 130 kbps)"),
    I18N_NOOP("Portable (average 160 kbps)"),

    I18N_NOOP("HiFi (average 175 kbps)"),
    I18N_NOOP("HiFi (average 190 kbps)"),
    I18N_NOOP("HiFi (average 210 kbps)"),
    I18N_NOOP("HiFi (average 230 kbps)"),

    I18N_NOOP("Archiving (320 kbps)"),
};


const char* s_lame_mode_strings[] = {
    I18N_NOOP("Stereo"),
    I18N_NOOP("Joint Stereo"),
    I18N_NOOP("Mono")
};

#endif
