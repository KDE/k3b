/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef K3B_ADVANCED_OPTION_TAB_H
#define K3B_ADVANCED_OPTION_TAB_H

#include <QWidget>

class QCheckBox;
class QLabel;
class QSpinBox;



namespace K3b {
    class AdvancedOptionTab : public QWidget
    {
        Q_OBJECT

    public:
        explicit AdvancedOptionTab( QWidget* parent = 0 );
        ~AdvancedOptionTab() override;

        void saveSettings();
        void readSettings();

    private Q_SLOTS:
        void slotSetDefaultBufferSizes( bool );

    private:
        void setupGui();

        QCheckBox*    m_checkBurnfree;
        QCheckBox*    m_checkEject;
        QCheckBox*    m_checkAutoErasingRewritable;
        QCheckBox*    m_checkOverburn;
        QCheckBox*    m_checkManualWritingBufferSize;
        QSpinBox*     m_editWritingBufferSize;
        QCheckBox*    m_checkShowForceGuiElements;
        QCheckBox*    m_checkForceUnsafeOperations;
    };
}


#endif
