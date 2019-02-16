/*
 *
 * Copyright (C) 2005-2007 Sebastian Trueg <trueg@k3b.org>
 *           (C) 2009      Arthur Renato Mello <arthur@mandriva.com>
 *           (C) 2009      Michal Malek <michalm@jabster.pl>
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

#include "k3bvideodvdview.h"
#include "k3bvideodvdburndialog.h"
#include "k3bvideodvddoc.h"

#include <KLocalizedString>
#include <KMessageBox>
#include <KActionCollection>

#include <QAction>


K3b::VideoDvdView::VideoDvdView( K3b::VideoDvdDoc* doc, QWidget *parent )
    : K3b::DataView( doc, parent ),
      m_doc(doc)
{
    QAction* actionImportSession = actionCollection()->action( "project_data_import_session" );
    QAction* actionClearSession = actionCollection()->action( "project_data_clear_imported_session" );
    QAction* actionEditBootImages = actionCollection()->action( "project_data_edit_boot_images" );
    
    actionImportSession->setEnabled( false );
    actionImportSession->setVisible( false );
    actionClearSession->setEnabled( false );
    actionClearSession->setVisible( false );
    actionEditBootImages->setEnabled( false );
    actionEditBootImages->setVisible( false );
}


K3b::VideoDvdView::~VideoDvdView()
{
}


K3b::ProjectBurnDialog* K3b::VideoDvdView::newBurnDialog( QWidget* parent )
{
    return new K3b::VideoDvdBurnDialog( m_doc, parent );
}


void K3b::VideoDvdView::init()
{
    KMessageBox::information( this,
                              i18n("Be aware that you need to provide the complete Video DVD filestructure. "
                                   "K3b does not support video transcoding and preparation of video object "
                                   "files yet. That means you need to already have the VTS_X_YY.VOB "
                                   "and VTS_X_YY.IFO files."),
                              i18n("K3b Video DVD Restrictions"),
                              "video_dvd_restrictions" );
}



