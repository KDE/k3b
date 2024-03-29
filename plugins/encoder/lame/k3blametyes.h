/*

    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_LAME_TYES_H_
#define _K3B_LAME_TYES_H_

#include <KLazyLocalizedString>

#include "k3bplugin_i18n.h"

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


const KLazyLocalizedString s_lame_preset_strings[] = {
    kli18n("Low quality (56 kbps)"),
    kli18n("Low quality (90 kbps)"),

    kli18n("Portable (average 115 kbps)"),
    kli18n("Portable (average 130 kbps)"),
    kli18n("Portable (average 160 kbps)"),

    kli18n("HiFi (average 175 kbps)"),
    kli18n("HiFi (average 190 kbps)"),
    kli18n("HiFi (average 210 kbps)"),
    kli18n("HiFi (average 230 kbps)"),

    kli18n("Archiving (320 kbps)"),
};


const KLazyLocalizedString s_lame_mode_strings[] = {
    kli18n("Stereo"),
    kli18n("Joint Stereo"),
    kli18n("Mono")
};

#endif
