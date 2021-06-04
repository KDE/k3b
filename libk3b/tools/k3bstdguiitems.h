/*
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
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
