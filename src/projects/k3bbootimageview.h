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


#ifndef K3B_BOOTIMAGEVIEW_H
#define K3B_BOOTIMAGEVIEW_H

#include "ui_base_k3bbootimageview.h"

class K3bDataDoc;
class K3bBootItem;

class base_K3bBootImageView : public QWidget, public Ui::base_K3bBootImageView
{
public:
  base_K3bBootImageView( QWidget *parent ) : QWidget( parent ) {
    setupUi( this );
  }
};

class K3bBootImageView : public base_K3bBootImageView
{
    Q_OBJECT

public:
    K3bBootImageView( K3bDataDoc* doc, QWidget* parent = 0 );
    ~K3bBootImageView();

private slots:
    void slotNewBootImage();
    void slotDeleteBootImage();
    void slotToggleOptions();
    void slotSelectionChanged();

    /* reimplemeted from base_...*/
    void slotOptionsChanged();

    void slotNoEmulationToggled( bool );

private:
    void updateBootImages();
    void showAdvancedOptions( bool );
    void loadBootItemSettings( K3bBootItem* );

    class PrivateBootImageViewItem;

    K3bDataDoc* m_doc;

    bool m_loadingItem;
};

#endif
