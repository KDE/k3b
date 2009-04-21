/*
*
* Copyright (C) 2003-2004 Christian Kvasny <chris@k3b.org>
*           (C) 2009      Arthur Renato Mello <arthur@mandriva.com>
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

// K3b Includes
#include "k3bvcdprojectmodel.h"
#include "k3bvcdview.h"
#include "k3bvcddoc.h"
#include "k3bvcdlistview.h"
#include "k3bvcdburndialog.h"
#include "k3bvcdtrackdialog.h"
#include "k3bfillstatusdisplay.h"
#include "k3bexternalbinmanager.h"
#include "k3bcore.h"
#include "k3baction.h"

// QT-includes
#include <qlayout.h>
#include <qstring.h>


// KDE-includes
#include <kaction.h>
#include <kmenu.h>
#include <klocale.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kmessagebox.h>

K3b::VcdView::VcdView( K3b::VcdDoc* pDoc, QWidget* parent )
        : K3b::StandardView( pDoc, parent )
{
    m_doc = pDoc;

    m_model = new K3b::VcdProjectModel(m_doc, this);
    // set the model for the K3b::StandardView's views
    setModel(m_model);
    setShowDirPanel(false);

#if 0
    // --- setup GUI ---------------------------------------------------

    m_vcdlist = new K3b::VcdListView( this, pDoc, this );
    setMainWidget( m_vcdlist );
    fillStatusDisplay() ->showSize();

    connect( m_vcdlist, SIGNAL( lengthReady() ), fillStatusDisplay(), SLOT( update() ) );
    connect( m_doc, SIGNAL( newTracks() ), fillStatusDisplay(), SLOT( update() ) );
#endif
    // setup actions
    m_actionProperties = K3b::createAction( this, i18n("Properties"), "document-properties",
                                            0, this, SLOT(showPropertiesDialog()),
                                            actionCollection(), "vcd_show_props" );

    m_actionRemove = K3b::createAction( this, i18n( "Remove" ), "edit-delete",
                                        Qt::Key_Delete, this, SLOT(slotRemoveSelectedIndexes()),
                                        actionCollection(), "vcd_remove_track" );

    m_popupMenu = new KMenu( this );
    m_popupMenu->addAction( m_actionRemove );
    m_popupMenu->addSeparator();
    m_popupMenu->addAction( m_actionProperties );
}

K3b::VcdView::~VcdView()
{}


K3b::ProjectBurnDialog* K3b::VcdView::newBurnDialog( QWidget * parent)
{
  return new K3b::VcdBurnDialog( m_doc, parent );
}


void K3b::VcdView::init()
{
  if( !k3bcore->externalBinManager()->foundBin( "vcdxbuild" ) ) {
    kDebug() << "(K3b::VcdView) could not find vcdxbuild executable";
    KMessageBox::information( this,
			      i18n( "Could not find VcdImager executable. "
				    "To create VideoCD's you must install VcdImager >= 0.7.12. "
				    "You can find this on your distribution disks or download "
				    "it from http://www.vcdimager.org" ) );
  }
}


void K3b::VcdView::contextMenuForSelection(const QModelIndexList &selectedIndexes, const QPoint &pos)
{
    if( selectedIndexes.count() >= 1 ) {
        m_actionRemove->setEnabled(true);
    }
    else {
        m_actionRemove->setEnabled(false);
    }

    m_popupMenu->popup( pos );
}


void K3b::VcdView::showPropertiesDialog()
{
    QModelIndexList selection = currentSelection();

    if ( selection.isEmpty() )
    {
        // show project properties
        slotProperties();
    }
    else
    {
        QList<K3b::VcdTrack*> selected;

        foreach(const QModelIndex &index, selection)
            selected.append(m_model->trackForIndex(index));

        QList<K3b::VcdTrack*> tracks = *m_doc->tracks();

        K3b::VcdTrackDialog dlg( m_doc, tracks, selected, this );
        dlg.exec();
    }
}

#include "k3bvcdview.moc"
