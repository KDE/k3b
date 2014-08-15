/*
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

#ifndef K3B_STD_GUIITEMS_H
#define K3B_STD_GUIITEMS_H

#include "k3b_export.h"

class QWidget;
class QCheckBox;
class QComboBox;
class QFrame;

namespace K3b {
    namespace StdGuiItems
    {
        LIBK3B_EXPORT QCheckBox* simulateCheckbox( QWidget* parent = 0 );
        LIBK3B_EXPORT QCheckBox* daoCheckbox( QWidget* parent = 0 );
        LIBK3B_EXPORT QCheckBox* burnproofCheckbox( QWidget* parent = 0 );
        LIBK3B_EXPORT QCheckBox* onlyCreateImagesCheckbox( QWidget* parent = 0 );
        LIBK3B_EXPORT QCheckBox* createCacheImageCheckbox( QWidget* parent = 0 );
        LIBK3B_EXPORT QCheckBox* removeImagesCheckbox( QWidget* parent = 0 );
        LIBK3B_EXPORT QCheckBox* onTheFlyCheckbox( QWidget* parent = 0 );
        LIBK3B_EXPORT QCheckBox* cdTextCheckbox( QWidget* parent = 0);
        LIBK3B_EXPORT QComboBox* paranoiaModeComboBox( QWidget* parent = 0 );
        LIBK3B_EXPORT QCheckBox* startMultisessionCheckBox( QWidget* parent = 0 );
        LIBK3B_EXPORT QCheckBox* normalizeCheckBox( QWidget* parent = 0 );
        LIBK3B_EXPORT QCheckBox* verifyCheckBox( QWidget* parent = 0 );
        LIBK3B_EXPORT QCheckBox* ignoreAudioReadErrorsCheckBox( QWidget* parent = 0 );
        LIBK3B_EXPORT QFrame* horizontalLine( QWidget* parent = 0 );
        LIBK3B_EXPORT QFrame* verticalLine( QWidget* parent = 0 );
    }
}

#endif
