/*
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009      Arthur Renato Mello <arthur@mandriva.com>
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


#include "k3bmovixview.h"
#include "k3bmovixprojectmodel.h"
#include "k3bmovixdoc.h"
#include "k3bmovixlistview.h"
#include "k3bmovixburndialog.h"
#include "k3bmovixfileitem.h"

#include <k3bfillstatusdisplay.h>
#include <k3bdatapropertiesdialog.h>
#include <k3bprojectplugin.h>
#include <k3baction.h>

#include <klocale.h>
#include <kdebug.h>
#include <kaction.h>
#include <kmenu.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kurl.h>

#include <qlayout.h>
#include <qlabel.h>
#include <qlineedit.h>
//Added by qt3to4:
#include <QList>
#include <KToolBar>

K3bMovixView::K3bMovixView( K3bMovixDoc* doc, QWidget* parent )
    : K3bStandardView( doc, parent ),
      m_doc(doc)
{
    m_model = new K3b::MovixProjectModel(m_doc, this);
    // set the model for the K3bStandardView's views
    setModel(m_model);

    // and hide the side panel as the audio project has no tree hierarchy
    setShowDirPanel(false);

#if 0
    connect( m_listView, SIGNAL(contextMenuRequested( Q3ListViewItem*, const QPoint& , int )),
             this, SLOT(slotContextMenuRequested(Q3ListViewItem*, const QPoint& , int )) );


    // setup actions
    m_actionProperties = K3b::createAction( this, i18n("Properties"), "document-properties",
                                            0, this, SLOT(showPropertiesDialog()),
                                            actionCollection(), "movix_show_props" );
    m_actionRemove = K3b::createAction( this, i18n( "Remove" ), "edit-delete",
                                        Qt::Key_Delete, this, SLOT(slotRemoveItems()),
                                        actionCollection(), "movix_remove_item" );
    m_actionRemoveSubTitle = K3b::createAction( this, i18n( "Remove Subtitle File" ), "edit-delete",
                                                0, this, SLOT(slotRemoveSubTitleItems()),
                                                actionCollection(), "movix_remove_subtitle_item" );
    m_actionAddSubTitle = K3b::createAction( this, i18n("Add Subtitle File..."), 0,
                                             0, this, SLOT(slotAddSubTitleFile()),
                                             actionCollection(), "movix_add_subtitle" );

    m_popupMenu = new KMenu( this );
    m_popupMenu->addAction( m_actionRemove );
    m_popupMenu->addAction( m_actionRemoveSubTitle );
    m_popupMenu->addAction( m_actionAddSubTitle );
    m_popupMenu->addSeparator();
    m_popupMenu->addAction( m_actionProperties );
    m_popupMenu->addSeparator();
    //  k3bMain()->actionCollection()->action("file_burn")->plug( m_popupMenu );


    addPluginButtons( K3bProjectPlugin::MOVIX_CD );

    m_volumeIDEdit = new QLineEdit( doc->isoOptions().volumeID(), toolBox() );
    toolBox()->addWidget( new QLabel( i18n("Volume Name:"), toolBox() ) );
    toolBox()->addWidget( m_volumeIDEdit );
    connect( m_volumeIDEdit, SIGNAL(textChanged(const QString&)),
             m_doc,
             SLOT(setVolumeID(const QString&)) );
#endif

    connect( m_doc, SIGNAL(changed()), this, SLOT(slotDocChanged()) );
}


K3bMovixView::~K3bMovixView()
{
}


void K3bMovixView::slotContextMenuRequested(Q3ListViewItem* item, const QPoint& p, int )
{
    if( item ) {
        m_actionRemove->setEnabled(true);
        m_actionRemoveSubTitle->setEnabled( true );
    }
    else {
        m_actionRemove->setEnabled(false);
        m_actionRemoveSubTitle->setEnabled( false );
    }

    m_popupMenu->popup( p );
}


void K3bMovixView::showPropertiesDialog()
{
    QList<K3bDataItem*> dataItems;

    // get selected item
    QList<Q3ListViewItem*> viewItems = m_listView->selectedItems();
    Q_FOREACH( Q3ListViewItem* item, viewItems ) {
        if( K3bMovixListViewItem* viewItem = dynamic_cast<K3bMovixListViewItem*>( item ) ) {
            dataItems.append( viewItem->fileItem() );
        }
    }

    if( !dataItems.isEmpty() ) {
        K3bDataPropertiesDialog d( dataItems, this );
        d.exec();
    }
    else
        slotProperties();
}


void K3bMovixView::slotRemoveItems()
{
    QList<Q3ListViewItem*> list = m_listView->selectedItems();

    if( list.isEmpty() )
        kDebug() << "nothing to remove";

    Q_FOREACH( Q3ListViewItem* item, list ) {
        K3bMovixListViewItem* vi = static_cast<K3bMovixListViewItem*>(item);
        if( vi->isMovixFileItem() )
            m_doc->removeItem( vi->fileItem() );
        else
            m_doc->removeSubTitleItem( ((K3bMovixSubTitleViewItem*)item)->fileItem() );
    }
}


void K3bMovixView::slotRemoveSubTitleItems()
{
    QList<Q3ListViewItem*> list = m_listView->selectedItems();

    if( list.isEmpty() )
        kDebug() << "nothing to remove";

    Q_FOREACH( Q3ListViewItem* item, list ) {
        K3bMovixListViewItem* vi = static_cast<K3bMovixListViewItem*>(item);
        m_doc->removeSubTitleItem( vi->fileItem() );
    }
}


void K3bMovixView::slotAddSubTitleFile()
{
    if ( m_listView->selectedItems().isEmpty() )
        return;
    Q3ListViewItem* item = m_listView->selectedItems().first();
    if( K3bMovixListViewItem* vi = dynamic_cast<K3bMovixListViewItem*>(item) ) {

        KUrl url = KFileDialog::getOpenUrl();
        if( url.isValid() ) {
            if( url.isLocalFile() )
                m_doc->addSubTitleItem( vi->fileItem(), url );
            else
                KMessageBox::error( 0, i18n("K3b currently only supports local files.") );
        }
    }
}


K3bProjectBurnDialog* K3bMovixView::newBurnDialog( QWidget* parent )
{
    return new K3bMovixBurnDialog( m_doc, parent );
}


void K3bMovixView::slotDocChanged()
{
    // do not update the editor in case it changed the volume id itself
    if( m_doc->isoOptions().volumeID() != m_volumeIDEdit->text() )
        m_volumeIDEdit->setText( m_doc->isoOptions().volumeID() );
}

#include "k3bmovixview.moc"
