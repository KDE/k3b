/* 
 *
 * $Id: $
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


#include "k3bdatapropertiesdialog.h"

#include "k3bdiritem.h"
#include "k3bfileitem.h"
#include <kcutlabel.h>

#include <qpushbutton.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qframe.h>
#include <qcheckbox.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

#include <klineedit.h>
#include <klocale.h>
#include <kmimetype.h>
#include <kurl.h>
#include <kio/global.h>



K3bDataPropertiesDialog::K3bDataPropertiesDialog( K3bDataItem* dataItem, QWidget* parent, const char* name )
  : KDialogBase( Plain, i18n("File properties"), Ok|Cancel, Ok, parent, name, true, true )
{
  m_dataItem = dataItem;

  QLabel* labelMimeType = new QLabel( plainPage() );
  QLabel* extraInfoLabel = new QLabel( plainPage() );
  m_editName = new KLineEdit( plainPage() );
  m_labelType = new QLabel( plainPage() );
  m_labelLocation = new KCutLabel( plainPage() );
  m_labelSize = new QLabel( plainPage() );
  m_labelLocalName = new KCutLabel( plainPage() );
  m_labelLocalLocation = new KCutLabel( plainPage() );


  QGridLayout* grid = new QGridLayout( plainPage() );
  grid->setSpacing( spacingHint() );
  grid->setMargin( marginHint() );

  grid->addWidget( labelMimeType, 0, 0 );
  grid->addWidget( m_editName, 0, 2 );
  QFrame* line = new QFrame( plainPage() );
  line->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  grid->addMultiCellWidget( line, 1, 1, 0, 2 );
  grid->addWidget( new QLabel( i18n("Type:"), plainPage() ), 2, 0 );
  grid->addWidget( new QLabel( i18n("Location:"), plainPage() ), 4, 0 );
  grid->addWidget( new QLabel( i18n("Size:"), plainPage() ), 5, 0 );
  grid->addWidget( m_labelType, 2, 2 );
  grid->addWidget( extraInfoLabel, 3, 2 );
  grid->addWidget( m_labelLocation, 4, 2 );
  grid->addWidget( m_labelSize, 5, 2 );
  line = new QFrame( plainPage() );
  line->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  grid->addMultiCellWidget( line, 6, 6, 0, 2 );
  QLabel* label1 = new QLabel( i18n("Local name:"), plainPage() );
  grid->addWidget(  label1, 7, 0 );
  QLabel* label2 = new QLabel( i18n("Local location:"), plainPage() );
  grid->addWidget( label2, 8, 0 );
  grid->addWidget( m_labelLocalName, 7, 2 );
  grid->addWidget( m_labelLocalLocation, 8, 2 );

  grid->addColSpacing( 1, 50 );
  grid->setColStretch( 2, 1 );



  if( K3bFileItem* fileItem = dynamic_cast<K3bFileItem*>(dataItem) ) {
    labelMimeType->setPixmap( fileItem->pixmap(KIcon::SizeLarge) );
    if( fileItem->isSymLink() )
      m_labelType->setText( i18n("Link to %1").arg(fileItem->mimeComment()) );
    else
      m_labelType->setText( fileItem->mimeComment() );
    m_labelLocalName->setText( fileItem->name() );
    QString localLocation = fileItem->url().path(-1);
    localLocation.truncate( localLocation.findRev('/') );
    m_labelLocalLocation->setText( localLocation );
    m_labelSize->setText( KIO::convertSize(dataItem->k3bSize()) );
  }
  else {
    labelMimeType->setPixmap( KMimeType::pixmapForURL( KURL( "/" )) );
    m_labelType->setText( i18n("Directory") );
    label1->hide();
    label2->hide();
    m_labelLocalName->hide();
    m_labelLocalLocation->hide();
    line->hide();
    K3bDirItem* dirItem = (K3bDirItem*)dataItem;
    m_labelSize->setText( KIO::convertSize(dataItem->k3bSize()) + "\n(" +
			  i18n("in 1 file", "in %n files", dirItem->numFiles()) + " " +
			  i18n("and 1 directory", "and %n directories", dirItem->numDirs()) + ")" );
  }

  m_editName->setText( dataItem->k3bName() );

  QString location = "/" + dataItem->k3bPath();
  if( location[location.length()-1] == '/' )
    location.truncate( location.length()-1 );
  location.truncate( location.findRev('/') );
  if( location.isEmpty() )
    location = "/";
  m_labelLocation->setText( location );
  extraInfoLabel->setText( QString( "(%1)" ).arg(dataItem->extraInfo()) );
  if( dataItem->extraInfo().isEmpty() )
    extraInfoLabel->hide();

  // OPTIONS
  // /////////////////////////////////////////////////
  line = new QFrame( plainPage() );
  line->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  grid->addMultiCellWidget( line, 9, 9, 0, 2 );
  m_checkHideOnRockRidge = new QCheckBox( i18n("Hide on Rockridge"), plainPage() );
  m_checkHideOnJoliet = new QCheckBox( i18n("Hide on Joliet"), plainPage() );

  grid->addMultiCellWidget( m_checkHideOnRockRidge, 10, 10, 0, 2 );
  grid->addMultiCellWidget( m_checkHideOnJoliet, 11, 11, 0, 2 );
  grid->setRowStretch( 12, 1 );


  m_checkHideOnJoliet->setChecked( dataItem->hideOnJoliet() );
  m_checkHideOnRockRidge->setChecked( dataItem->hideOnRockRidge() );

  // if the parent is hidden the value cannot be changed (see K3bDataItem::setHide...)
  if( dataItem->parent() ) {
    m_checkHideOnRockRidge->setDisabled( dataItem->parent()->hideOnRockRidge() );
    m_checkHideOnJoliet->setDisabled( dataItem->parent()->hideOnJoliet() );
  }

  if( !dataItem->isHideable() ) {
    m_checkHideOnJoliet->hide();
    m_checkHideOnRockRidge->hide();
    line->hide();
  }

  QToolTip::add( m_checkHideOnRockRidge, i18n("") );
  QToolTip::add( m_checkHideOnJoliet, i18n("") );
  QWhatsThis::add( m_checkHideOnRockRidge, i18n("<p>If this option is checked, the file or directory "
						"(and its entire contents) will be hidden on the "
						"ISO9660 and RockRidge filesystem.</p>"
						"<p>This is useful, for example, for having different README "
						"files for RockRidge and Joliet, which can be managed "
						"by hiding README.joliet on RockRidge and README.rr "
						"on the Joliet filesystem.</p>") );
  QWhatsThis::add( m_checkHideOnJoliet, i18n("<p>If this option is checked, the file or directory "
					     "(and its entire contents) will be hidden on the "
					     "Joliet filesystem.</p>"
					     "<p>This is useful, for example, for having different README "
					     "files for RockRidge and Joliet, which can be managed "
					     "by hiding README.joliet on RockRidge and README.rr "
					     "on the Joliet filesystem.</p>") );


  m_editName->setReadOnly( !dataItem->isRenameable() );
  m_editName->setFocus();
}


K3bDataPropertiesDialog::~K3bDataPropertiesDialog()
{
}


void K3bDataPropertiesDialog::slotOk()
{
  // save filename
  m_dataItem->setK3bName( m_editName->text() );
  m_dataItem->setHideOnRockRidge( m_checkHideOnRockRidge->isChecked() );
  m_dataItem->setHideOnJoliet( m_checkHideOnJoliet->isChecked() );

  KDialogBase::slotOk();
}


#include "k3bdatapropertiesdialog.moc"
