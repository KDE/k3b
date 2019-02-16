/*
 *
 * Copyright (C) 2007-2009 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_MEDIA_FORMATTING_DIALOG_H_
#define _K3B_MEDIA_FORMATTING_DIALOG_H_

#include "k3binteractiondialog.h"


class QCheckBox;

namespace K3b {
    class WritingModeWidget;
    class WriterSelectionWidget;

    namespace Device {
        class Device;
    }

    class MediaFormattingDialog : public InteractionDialog
    {
        Q_OBJECT

    public:
        explicit MediaFormattingDialog( QWidget* = 0 );
        ~MediaFormattingDialog() override;

    public Q_SLOTS:
        void setDevice( Device::Device* );

    protected Q_SLOTS:
        void slotStartClicked() override;

    protected:
        void toggleAll() override;

    private:
        void loadSettings( const KConfigGroup& ) override;
        void saveSettings( KConfigGroup ) override;

        WriterSelectionWidget* m_writerSelectionWidget;
        WritingModeWidget* m_writingModeWidget;
        QCheckBox* m_checkForce;
        QCheckBox* m_checkQuickFormat;
    };
}

#endif
