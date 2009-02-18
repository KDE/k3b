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

class K3bMovixDoc;
class K3bMovixBin;

class K3bMovixOptionsWidget : public QWidget, public Ui::base_K3bMovixOptionsWidget
{
    Q_OBJECT

public:
    K3bMovixOptionsWidget( QWidget* parent = 0 );
    ~K3bMovixOptionsWidget();

public Q_SLOTS:
    void init( const K3bMovixBin* );
    void readSettings( K3bMovixDoc* );
    void saveSettings( K3bMovixDoc* );
    void loadConfig( const KConfigGroup & c );
    void saveConfig( KConfigGroup &c );
    void loadDefaults();

private:
    class LanguageSelectionHelper;
    LanguageSelectionHelper* m_keyboardLangHelper;
    LanguageSelectionHelper* m_helpLangHelper;
};


#endif
