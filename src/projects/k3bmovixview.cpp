/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
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

#include <klocale.h>
#include <kdebug.h>
#include <kaction.h>
#include <kpopupmenu.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kurl.h>

#include <qlayout.h>


K3bMovixView::K3bMovixView( K3bMovixDoc* doc, QWidget* parent, const char* name )
  : K3bView( doc, parent, name ),
    m_doc(doc)
{
  m_listView = new K3bMovixListView( m_doc, this );
  setMainWidget( m_listView );

  connect( m_listView, SIGNAL(contextMenuRequested( QListViewItem*, const QPoint& , int )),
	   this, SLOT(slotContextMenuRequested(QListViewItem*, const QPoint& , int )) );


  // setup actions
  m_actionProperties = new KAction( i18n("Properties"), "misc",
				    0, this, SLOT(showPropertiesDialog()),
				    actionCollection(), "movix_show_props" );
  m_actionRemove = new KAction( i18n( "Remove" ), "editdelete",
				Key_Delete, this, SLOT(slotRemoveItems()),
				actionCollection(), "movix_remove_item" );
  m_actionRemoveSubTitle = new KAction( i18n( "Remove Subtitle File" ), "editdelete",
					0, this, SLOT(slotRemoveSubTitleItems()),
					actionCollection(), "movix_remove_subtitle_item" );
  m_actionAddSubTitle = new KAction( i18n("Add Subtitle File..."), "",
				     0, this, SLOT(slotAddSubTitleFile()),
				     actionCollection(), "movix_add_subtitle" );

  m_popupMenu = new KPopupMenu( this );
  m_actionRemove->plug( m_popupMenu );
  m_actionRemoveSubTitle->plug( m_popupMenu );
  m_actionAddSubTitle->plug( m_popupMenu );
  m_popupMenu->insertSeparator();
  m_actionProperties->plug( m_popupMenu );
  m_popupMenu->insertSeparator();
  //  k3bMain()->actionCollection()->action("file_burn")->plug( m_popupMenu );
}


K3bMovixView::~K3bMovixView()
{
}


void K3bMovixView::slotContextMenuRequested(QListViewItem* item, const QPoint& p, int )
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
  K3bFileItem* dataItem = 0;

  // get selected item
  if( K3bMovixListViewItem* viewItem = dynamic_cast<K3bMovixListViewItem*>( m_listView->selectedItems().first() ) ) {
    dataItem = viewItem->fileItem();
  }

  if( dataItem ) {
    K3bDataPropertiesDialog d( dataItem, this );
    d.exec();
  }
  else
    slotProperties();
}


void K3bMovixView::slotRemoveItems()
{
  QPtrList<QListViewItem> list = m_listView->selectedItems();
  QPtrListIterator<QListViewItem> it(list);

  if( list.isEmpty() )
    kdDebug() << "nothing to remove" << endl;

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
  QPtrList<QListViewItem> list = m_listView->selectedItems();
  QPtrListIterator<QListViewItem> it(list);

  if( list.isEmpty() )
    kdDebug() << "nothing to remove" << endl;

  for( ; it.current(); ++it ) {
    K3bMovixListViewItem* vi = static_cast<K3bMovixListViewItem*>(*it);
    m_doc->removeSubTitleItem( vi->fileItem() );
  }
}


void K3bMovixView::slotAddSubTitleFile()
{
  QListViewItem* item = m_listView->selectedItems().first();
  if( K3bMovixListViewItem* vi = dynamic_cast<K3bMovixListViewItem*>(item) ) {

    KURL url = KFileDialog::getOpenURL();
    if( url.isValid() ) {
      if( url.isLocalFile() )
	m_doc->addSubTitleItem( vi->fileItem(), url );
      else
	KMessageBox::error( 0, i18n("K3b currently only supports local files.") );
    }
  }
}


K3bProjectBurnDialog* K3bMovixView::newBurnDialog( QWidget* parent, const char* name )
{
  return new K3bMovixBurnDialog( m_doc, parent, name, true );
}

#include "k3bmovixview.moc"
