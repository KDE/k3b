/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bbootimageview.h"

#include "k3bdatadoc.h"
#include "k3bbootitem.h"

#include <klocale.h>
#include <klistview.h>
#include <kfiledialog.h>

#include <qpushbutton.h>
#include <qstring.h>
#include <qvalidator.h>
#include <qgroupbox.h>
#include <qlineedit.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>



class K3bBootImageView::PrivateBootImageViewItem : public KListViewItem
{
public:
  PrivateBootImageViewItem( K3bBootItem* image, QListView* parent ) 
    : KListViewItem( parent ), 
      m_image( image ) {

  }

  PrivateBootImageViewItem( K3bBootItem* image, QListView* parent, QListViewItem* after )
    : KListViewItem( parent, after ),
      m_image( image ) {

  }

  QString text( int col ) const {
    if( col == 0 ) {
      if( m_image->imageType() == K3bBootItem::FLOPPY )
	return i18n("Floppy");
      else if( m_image->imageType() == K3bBootItem::HARDDISK )
	return i18n("Harddisk");
      else
	return i18n("None");
    }
    else if( col == 1 )
      return m_image->localPath();
    else
      return QString::null;
  }

  K3bBootItem* bootImage() const { return m_image; }

private:
  K3bBootItem* m_image;
};


K3bBootImageView::K3bBootImageView( K3bDataDoc* doc, QWidget* parent, const char* name )
  : base_K3bBootImageView( parent, name ),
    m_doc(doc)
{
  connect( m_buttonNew, SIGNAL(clicked()), 
	   this, SLOT(slotNewBootImage()) );
  connect( m_buttonDelete, SIGNAL(clicked()), 
	   this, SLOT(slotDeleteBootImage()) );
  connect( m_buttonToggleOptions, SIGNAL(clicked()),
	   this, SLOT(slotToggleOptions()) );
  connect( m_viewImages, SIGNAL(selectionChanged()),
	   this, SLOT(slotSelectionChanged()) );
  connect( m_radioNoEmulation, SIGNAL(toggled(bool)),
	   this, SLOT(slotNoEmulationToggled(bool)) );

  QIntValidator* v = new QIntValidator( this );
  m_editLoadSegment->setValidator( v );
  m_editLoadSize->setValidator( v );

  updateBootImages();

  showAdvancedOptions( false );
  loadBootItemSettings(0);
}

K3bBootImageView::~K3bBootImageView()
{
}


void K3bBootImageView::slotToggleOptions()
{
  showAdvancedOptions( !m_groupOptions->isVisible() );
}


void K3bBootImageView::showAdvancedOptions( bool show )
{
  if( show ) {
    m_groupOptions->show();
    m_buttonToggleOptions->setText( i18n("Hide Advanced Options") );
  }
  else {
    m_groupOptions->hide();
    m_buttonToggleOptions->setText( i18n("Show Advanced Options") );
  }
}


void K3bBootImageView::slotNewBootImage()
{
  QString file = KFileDialog::getOpenFileName( QString::null, QString::null, this, i18n("Please choose a boot image") );
  if( !file.isEmpty() ) {
    m_doc->createBootItem( file );
    updateBootImages();
  }
}


void K3bBootImageView::slotDeleteBootImage()
{
  QListViewItem* item = m_viewImages->selectedItem();
  if( item ) {
    K3bBootItem* i = ((PrivateBootImageViewItem*)item)->bootImage();
    delete item;
    m_doc->removeItem( i );
  }
}


void K3bBootImageView::slotSelectionChanged()
{
  QListViewItem* item = m_viewImages->selectedItem();
  if( item )
    loadBootItemSettings( ((PrivateBootImageViewItem*)item)->bootImage() );
  else
    loadBootItemSettings( 0 );
}


void K3bBootImageView::updateBootImages()
{
  m_viewImages->clear();
  for( QPtrListIterator<K3bBootItem> it( m_doc->bootImages() ); it.current(); ++it ) {
    (void)new PrivateBootImageViewItem( *it, m_viewImages, 
					m_viewImages->lastItem() );
  }
}


void K3bBootImageView::loadBootItemSettings( K3bBootItem* item )
{
  // this is needed to prevent the slots to change stuff
  m_loadingItem = true;

  if( item ) {
    m_groupOptions->setEnabled(true);
    m_groupImageType->setEnabled(true);

    m_checkNoBoot->setChecked( item->noBoot() );
    m_checkInfoTable->setChecked( item->bootInfoTable() );
    m_editLoadSegment->setText( QString::number( item->loadSegment() ) );
    m_editLoadSize->setText( QString::number( item->loadSize() ) );

    if( item->imageType() == K3bBootItem::FLOPPY )
      m_radioFloppy->setChecked(true);
    else if( item->imageType() == K3bBootItem::HARDDISK )
      m_radioHarddisk->setChecked(true);      
    else
      m_radioNoEmulation->setChecked(true);
  }
  else {
    m_groupOptions->setEnabled(false);
    m_groupImageType->setEnabled(false);
  }

  m_loadingItem = false;
}


void K3bBootImageView::slotOptionsChanged()
{
  if( !m_loadingItem ) {
    QListViewItem* item = m_viewImages->selectedItem();
    if( item ) {
      K3bBootItem* i = ((PrivateBootImageViewItem*)item)->bootImage();
      
      i->setNoBoot( m_checkNoBoot->isChecked() );
      i->setBootInfoTable( m_checkInfoTable->isChecked() );
      i->setLoadSegment( m_editLoadSegment->text().toInt() ); 
      i->setLoadSize( m_editLoadSize->text().toInt() ); 
      if( m_radioFloppy->isChecked() )
	i->setImageType( K3bBootItem::FLOPPY );
      else if( m_radioHarddisk->isChecked() )
	i->setImageType( K3bBootItem::HARDDISK );
      else
	i->setImageType( K3bBootItem::NONE );
    }
  }
}


void K3bBootImageView::slotNoEmulationToggled( bool on )
{
  // it makes no sense to combine no emulation and no boot!
  // the base_widget takes care of the disabling
  if( on )
    m_checkNoBoot->setChecked(false);
}

#include "k3bbootimageview.moc"
