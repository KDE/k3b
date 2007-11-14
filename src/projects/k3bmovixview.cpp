/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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


#include "k3bmovixview.h"
#include "k3bmovixdoc.h"
#include "k3bmovixlistview.h"
#include "k3bmovixburndialog.h"
#include "k3bmovixfileitem.h"

#include <k3bfillstatusdisplay.h>
#include <k3bdatapropertiesdialog.h>
#include <k3bprojectplugin.h>

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
#include <Q3ValueList>
#include <Q3PtrList>
#include <KToolBar>

K3bMovixView::K3bMovixView( K3bMovixDoc* doc, QWidget* parent )
  : K3bView( doc, parent ),
    m_doc(doc)
{
  m_listView = new K3bMovixListView( m_doc, this );
  setMainWidget( m_listView );

  connect( m_listView, SIGNAL(contextMenuRequested( Q3ListViewItem*, const QPoint& , int )),
	   this, SLOT(slotContextMenuRequested(Q3ListViewItem*, const QPoint& , int )) );


  // setup actions
  m_actionProperties = new KAction( i18n("Properties"), "misc",
				    0, this, SLOT(showPropertiesDialog()),
				    actionCollection(), "movix_show_props" );
  m_actionRemove = new KAction( i18n( "Remove" ), "editdelete",
				Qt::Key_Delete, this, SLOT(slotRemoveItems()),
				actionCollection(), "movix_remove_item" );
  m_actionRemoveSubTitle = new KAction( i18n( "Remove Subtitle File" ), "editdelete",
					0, this, SLOT(slotRemoveSubTitleItems()),
					actionCollection(), "movix_remove_subtitle_item" );
  m_actionAddSubTitle = new KAction( i18n("Add Subtitle File..."), "",
				     0, this, SLOT(slotAddSubTitleFile()),
				     actionCollection(), "movix_add_subtitle" );

  m_popupMenu = new KMenu( this );
  m_actionRemove->plug( m_popupMenu );
  m_actionRemoveSubTitle->plug( m_popupMenu );
  m_actionAddSubTitle->plug( m_popupMenu );
  m_popupMenu->insertSeparator();
  m_actionProperties->plug( m_popupMenu );
  m_popupMenu->insertSeparator();
  //  k3bMain()->actionCollection()->action("file_burn")->plug( m_popupMenu );


  addPluginButtons( K3bProjectPlugin::MOVIX_CD );

  toolBox()->addStretch();

  m_volumeIDEdit = new QLineEdit( doc->isoOptions().volumeID(), toolBox() );
  toolBox()->addLabel( i18n("Volume Name:") );
  toolBox()->addSpacing();
  toolBox()->addWidget( m_volumeIDEdit );
  connect( m_volumeIDEdit, SIGNAL(textChanged(const QString&)),
	   m_doc,
	   SLOT(setVolumeID(const QString&)) );

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
  Q3ValueList<K3bDataItem*> dataItems;

  // get selected item
  Q3PtrList<Q3ListViewItem> viewItems = m_listView->selectedItems();
  for ( Q3PtrListIterator<Q3ListViewItem> it( viewItems ); *it; ++it ) {
      if( K3bMovixListViewItem* viewItem = dynamic_cast<K3bMovixListViewItem*>( *it ) ) {
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
  Q3PtrList<Q3ListViewItem> list = m_listView->selectedItems();
  Q3PtrListIterator<Q3ListViewItem> it(list);

  if( list.isEmpty() )
    kDebug() << "nothing to remove";

  for( ; it.current(); ++it ) {
    K3bMovixListViewItem* vi = static_cast<K3bMovixListViewItem*>(*it);
    if( vi->isMovixFileItem() )
      m_doc->removeItem( vi->fileItem() );
    else
      m_doc->removeSubTitleItem( ((K3bMovixSubTitleViewItem*)*it)->fileItem() );
  }
}


void K3bMovixView::slotRemoveSubTitleItems()
{
  Q3PtrList<Q3ListViewItem> list = m_listView->selectedItems();
  Q3PtrListIterator<Q3ListViewItem> it(list);

  if( list.isEmpty() )
    kDebug() << "nothing to remove";

  for( ; it.current(); ++it ) {
    K3bMovixListViewItem* vi = static_cast<K3bMovixListViewItem*>(*it);
    m_doc->removeSubTitleItem( vi->fileItem() );
  }
}


void K3bMovixView::slotAddSubTitleFile()
{
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
  return new K3bMovixBurnDialog( m_doc, parent, true );
}


void K3bMovixView::slotDocChanged()
{
  // do not update the editor in case it changed the volume id itself
  if( m_doc->isoOptions().volumeID() != m_volumeIDEdit->text() )
    m_volumeIDEdit->setText( m_doc->isoOptions().volumeID() );
}

#include "k3bmovixview.moc"
