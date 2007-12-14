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

#include "base_k3baudiocdtextwidget.h"
#include "ui_base_k3baudiocdtextallfieldswidget.h"

class K3bAudioDoc;

class base_K3bAudioCdTextAllFieldsWidget : public QWidget, public Ui::base_K3bAudioCdTextAllFieldsWidget
{
public:
  base_K3bAudioCdTextAllFieldsWidget( QWidget *parent ) : QWidget( parent ) {
    setupUi( this );
  }
};


class K3bAudioCdTextWidget : public base_K3bAudioCdTextWidget
{
  Q_OBJECT

public:
    K3bAudioCdTextWidget( QWidget* parent = 0 );
    ~K3bAudioCdTextWidget();

    bool isChecked() const;

public slots:
    void setChecked( bool );
    void load( K3bAudioDoc* );
    void save( K3bAudioDoc* );

private slots:
    void slotCopyTitle();
    void slotCopyPerformer();
    void slotCopyArranger();
    void slotCopySongwriter();
    void slotCopyComposer();
    void slotMoreFields();

private:
    K3bAudioDoc* m_doc;

    class AllFieldsDialog;
    AllFieldsDialog* m_allFieldsDlg;
};

#endif
