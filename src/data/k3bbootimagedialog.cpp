/***************************************************************************
                            k3bbootimagedialog.cpp
                             -------------------
    copyright            : (C) 2002 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "k3bbootimagedialog.h"
#include "k3bbootimagepropertieswidget.h"
#include "k3bbootimage.h"
#include "k3bdatadoc.h"

#include <qtabwidget.h>
#include <qcombobox.h>
#include <qregexp.h>
#include <qvalidator.h>
#include <qlineedit.h>
#include <qcheckbox.h>
#include <qptrlist.h>

#include <klocale.h>
#include <kurlrequester.h>


K3bBootImageDialog::K3bBootImageDialog( K3bBootImage* image, 
					K3bDataDoc* doc, 
					QWidget* parent, 
					const char* name, 
					bool modal )
  : KDialogBase( parent, name, modal, i18n("Boot image"), Ok|Cancel ),
    m_bootImage( image ),
    m_doc( doc )
{
  m_bootImageWidget = new K3bBootImagePropertiesWidget( this );
  setMainWidget( m_bootImageWidget );

  QIntValidator* v = new QIntValidator( this );
  m_bootImageWidget->editLoadSegment->setValidator( v );
  m_bootImageWidget->editLoadSize->setValidator( v );

  m_bootImageWidget->comboImageType->insertItem( i18n("Floppy image"), 0 );
  m_bootImageWidget->comboImageType->insertItem( i18n("Harddisk image"), 1 );

  connect( m_bootImageWidget->editUrl, SIGNAL(textChanged(const QString&)),
	   this, SLOT(slotImageFileChanged(const QString&)) );

  if( m_bootImage ) {
    m_bootImageWidget->editUrl->setURL( m_bootImage->fileItem->localPath() );
    m_bootImageWidget->checkNoEmulate->setChecked( m_bootImage->noEmulate );
    m_bootImageWidget->checkNoBoot->setChecked( m_bootImage->noBoot );
    m_bootImageWidget->checkInfoTable->setChecked( m_bootImage->bootInfoTable );
    m_bootImageWidget->editLoadSegment->setText( m_bootImage->loadSegment == -1 
						 ? QString::null
						 : QString::number(m_bootImage->loadSegment) );
    m_bootImageWidget->editLoadSize->setText( m_bootImage->loadSize == -1 
					      ? QString::null
					      : QString::number(m_bootImage->loadSize) );
    m_bootImageWidget->comboImageType->setCurrentItem( m_bootImage->imageType == K3bBootImage::FLOPPY
						       ? 0 : 1 );
  }
}


K3bBootImageDialog::~K3bBootImageDialog()
{
}


void K3bBootImageDialog::slotImageFileChanged( const QString& filename )
{
  // if the text really changed we need to delete the fileItem
  // since the fileItem does not support changing the local file
  if( m_bootImage )
    if( m_bootImage->fileItem ) {
      if( m_bootImage->fileItem->localPath() != filename ) {
	delete m_bootImage->fileItem;
	m_bootImage->fileItem = 0;
      }
    }
}


void K3bBootImageDialog::slotOk()
{
  // TODO: check if the file in the url lineedit exists

  if( !m_bootImage ) {
    m_bootImage = new K3bBootImage();
    m_doc->bootImages().append( m_bootImage );
  }

  if( !m_bootImage->fileItem ) {
    m_bootImage->fileItem = m_doc->createBootItem( m_bootImageWidget->editUrl->url() );
    m_bootImage->fileItem->setRemoveable(false);
    m_bootImage->fileItem->setExtraInfo( i18n("Boot image") );
  }

  m_bootImage->noEmulate = m_bootImageWidget->checkNoEmulate->isChecked();
  m_bootImage->noBoot = m_bootImageWidget->checkNoBoot->isChecked();
  m_bootImage->bootInfoTable = m_bootImageWidget->checkInfoTable->isChecked();

  m_bootImage->loadSegment = ( m_bootImageWidget->editLoadSegment->text().isEmpty() 
			       ? -1 
			       : m_bootImageWidget->editLoadSegment->text().toInt() );
  m_bootImage->loadSize = ( m_bootImageWidget->editLoadSize->text().isEmpty() 
			    ? -1 
			    : m_bootImageWidget->editLoadSize->text().toInt() );
  m_bootImage->imageType = ( m_bootImageWidget->comboImageType->currentItem() == 0
			     ? K3bBootImage::FLOPPY : K3bBootImage::HARDDISK );

  done( Ok );
}


void K3bBootImageDialog::slotCancel()
{
  done( Cancel );
}

#include "k3bbootimagedialog.moc"
