/*
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
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

#ifndef K3B_AUDIO_CDTEXT_WIDGET_H
#define K3B_AUDIO_CDTEXT_WIDGET_H

#include "ui_base_k3baudiocdtextwidget.h"

namespace K3b {
    class AudioDoc;

    class AudioCdTextWidget : public QWidget, public Ui::base_K3bAudioCdTextWidget
    {
        Q_OBJECT

    public:
        explicit AudioCdTextWidget( QWidget* parent = 0 );
        ~AudioCdTextWidget() override;

        bool isChecked() const;

    public Q_SLOTS:
        void setChecked( bool );
        void load( K3b::AudioDoc* );
        void save( K3b::AudioDoc* );

    private Q_SLOTS:
        void slotCopyTitle();
        void slotCopyPerformer();
        void slotCopyArranger();
        void slotCopySongwriter();
        void slotCopyComposer();
        void slotMoreFields();

    private:
        AudioDoc* m_doc;

        class AllFieldsDialog;
        AllFieldsDialog* m_allFieldsDlg;
    };
}

#endif
