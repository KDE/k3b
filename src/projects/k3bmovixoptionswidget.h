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


#ifndef _K3B_MOVIX_OPTIONSWIDGET_H_
#define _K3B_MOVIX_OPTIONSWIDGET_H_

#include "ui_base_k3bmovixoptionswidget.h"

#include <KConfigGroup>

namespace K3b {
    class MovixDoc;
    class MovixBin;

    class MovixOptionsWidget : public QWidget, public Ui::base_K3bMovixOptionsWidget
    {
        Q_OBJECT

    public:
        explicit MovixOptionsWidget( QWidget* parent = 0 );
        ~MovixOptionsWidget() override;

    public Q_SLOTS:
        void init( const K3b::MovixBin* );
        void readSettings( K3b::MovixDoc* );
        void saveSettings( K3b::MovixDoc* );

        void loadConfig( const KConfigGroup & c );
        void saveConfig( KConfigGroup c );

    private:
        class LanguageSelectionHelper;
        LanguageSelectionHelper* m_keyboardLangHelper;
        LanguageSelectionHelper* m_helpLangHelper;
    };
}


#endif
