/***************************************************************************
                          k3bdatapropertiesdialog.cpp  -  description
                             -------------------
    begin                : Mon Dec 17 2001
    copyright            : (C) 2001 by Sebastian Trueg
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

#include "k3bdatapropertiesdialog.h"

#include "k3bdataitem.h"
#include "k3bfileitem.h"

#include <qpushbutton.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qframe.h>

#include <klineedit.h>
#include <klocale.h>
#include <kmimetype.h>
#include <kurl.h>
#include <kio/global.h>



K3bDataPropertiesDialog::K3bDataPropertiesDialog( K3bDataItem* dataItem, QWidget* parent, const char* name )
  : KDialogBase( KDialogBase::Plain, i18n("File properties"), KDialogBase::Ok|KDialogBase::Cancel, KDialogBase::Ok,
		 parent, name, true, true )
{
  m_dataItem = dataItem;

  QLabel* labelMimeType = new QLabel( plainPage() );
  m_editName = new KLineEdit( plainPage() );
  m_labelType = new QLabel( plainPage() );
  m_labelLocation = new QLabel( plainPage() );
  m_labelSize = new QLabel( plainPage() );
  m_labelLocalName = new QLabel( plainPage() );
  m_labelLocalLocation = new QLabel( plainPage() );


  QGridLayout* grid = new QGridLayout( plainPage() );
  grid->setSpacing( spacingHint() );
  grid->setMargin( marginHint() );

  grid->addWidget( labelMimeType, 0, 0 );
  grid->addWidget( m_editName, 0, 2 );
  QFrame* line = new QFrame( plainPage() );
  line->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  grid->addMultiCellWidget( line, 1, 1, 0, 2 );
  grid->addWidget( new QLabel( i18n("Type:"), plainPage() ), 2, 0 );
  grid->addWidget( new QLabel( i18n("Location:"), plainPage() ), 3, 0 );
  grid->addWidget( new QLabel( i18n("Size:"), plainPage() ), 4, 0 );
  grid->addWidget( m_labelType, 2, 2 );
  grid->addWidget( m_labelLocation, 3, 2 );
  grid->addWidget( m_labelSize, 4, 2 );
  line = new QFrame( plainPage() );
  line->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  grid->addMultiCellWidget( line, 5, 5, 0, 2 );
  QLabel* label1 = new QLabel( i18n("Local name:"), plainPage() );
  grid->addWidget(  label1, 6, 0 );
  QLabel* label2 = new QLabel( i18n("Local location:"), plainPage() );
  grid->addWidget( label2, 7, 0 );
  grid->addWidget( m_labelLocalName, 6, 2 );
  grid->addWidget( m_labelLocalLocation, 7, 2 );

  grid->addColSpacing( 1, 50 );
  grid->setColStretch( 2, 1 );
  grid->setRowStretch( 8, 1 );



  if( K3bFileItem* fileItem = dynamic_cast<K3bFileItem*>(dataItem) ) {
    labelMimeType->setPixmap( fileItem->pixmap(KIcon::SizeLarge) );
    m_labelType->setText( fileItem->mimeComment() );
    m_labelLocalName->setText( fileItem->name() );
    QString localLocation = fileItem->url().path(-1);
    localLocation.truncate( localLocation.findRev('/') );
    m_labelLocalLocation->setText( localLocation );
  }
  else {
    labelMimeType->setPixmap( KMimeType::pixmapForURL( KURL( "/" ), 0, KIcon::SizeLarge ) );
    m_labelType->setText( i18n("Directory") );
    label1->hide();
    label2->hide();
    m_labelLocalName->hide();
    m_labelLocalLocation->hide();
    line->hide();
  }

  m_editName->setText( dataItem->k3bName() );

  QString location = "/" + dataItem->k3bPath();
  if( location[location.length()-1] == '/' )
    location.truncate( location.length()-1 );
  location.truncate( location.findRev('/') );
  if( location.isEmpty() )
    location = "/";
  m_labelLocation->setText( location );

  m_labelSize->setText( KIO::convertSize(dataItem->k3bSize()) );

  m_editName->setFocus();
}


K3bDataPropertiesDialog::~K3bDataPropertiesDialog()
{
}


void K3bDataPropertiesDialog::slotOk()
{
  // save filename
  m_dataItem->setK3bName( m_editName->text() );

  KDialogBase::slotOk();
}


#include "k3bdatapropertiesdialog.moc"
