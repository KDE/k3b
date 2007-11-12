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


#include "k3bdatapropertiesdialog.h"

#include "k3bdiritem.h"
#include "k3bfileitem.h"
#include <k3bvalidators.h>

#include <qpushbutton.h>
#include <qlayout.h>
#include <qlabel.h>
#include <q3frame.h>
#include <qcheckbox.h>
#include <qtooltip.h>

#include <qtabwidget.h>
#include <qvalidator.h>
//Added by qt3to4:
#include <Q3GridLayout>
#include <Q3ValueList>

#include <klineedit.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmimetype.h>
#include <kurl.h>
#include <kio/global.h>
#include <kfileitem.h>
#include <ksqueezedtextlabel.h>


K3bDataPropertiesDialog::K3bDataPropertiesDialog( const Q3ValueList<K3bDataItem*>& dataItems, QWidget* parent, const char* name )
  : KDialogBase( Plain, i18n("File Properties"), Ok|Cancel, Ok, parent, name, true, false )
{
  m_dataItems = dataItems;

  m_labelIcon = new QLabel( plainPage() );
  if ( dataItems.count() == 1 ) {
      m_editName = new KLineEdit( plainPage() );
      m_editName->setValidator( K3bValidators::iso9660Validator( false, this ) );
      m_labelType = new QLabel( plainPage() );
      m_labelLocalName = new KSqueezedTextLabelLabel( plainPage() );
      m_labelLocalLocation = new KSqueezedTextLabelLabel( plainPage() );
      m_extraInfoLabel = new QLabel( plainPage() );
  }
  else {
      m_multiSelectionLabel = new QLabel( plainPage() );
  }

  m_labelLocation = new KSqueezedTextLabelLabel( plainPage() );
  m_labelSize = new QLabel( plainPage() );
  m_labelBlocks = new QLabel( plainPage() );

  // layout
  // -----------------------------
  Q3GridLayout* grid = new Q3GridLayout( plainPage() );
  grid->setSpacing( spacingHint() );
  grid->setMargin( marginHint() );

  grid->addWidget( m_labelIcon, 0, 0 );
  if ( dataItems.count() == 1 ) {
      grid->addWidget( m_editName, 0, 2 );
  }
  else {
      grid->addWidget( m_multiSelectionLabel, 0, 2 );
  }
  int row = 1;

  m_spacerLine = new QFrame( plainPage() );
  m_spacerLine->setFrameStyle( Q3Frame::HLine | Q3Frame::Sunken );
  grid->addMultiCellWidget( m_spacerLine, row, row, 0, 2 );
  ++row;
  if ( dataItems.count() == 1 ) {
      grid->addWidget( new QLabel( i18n("Type:"), plainPage() ), row, 0 );
      grid->addWidget( m_labelType, row++, 2 );
      grid->addWidget( m_extraInfoLabel, row++, 2 );
  }
  grid->addWidget( new QLabel( i18n("Location:"), plainPage() ), row, 0 );
  grid->addWidget( m_labelLocation, row++, 2 );
  grid->addWidget( new QLabel( i18n("Size:"), plainPage() ), row, 0 );
  grid->addWidget( m_labelSize, row++, 2 );
  grid->addWidget( new QLabel( i18n("Used blocks:"), plainPage() ), row, 0 );
  grid->addWidget( m_labelBlocks, row++, 2 );

  m_spacerLine = new QFrame( plainPage() );
  m_spacerLine->setFrameStyle( Q3Frame::HLine | Q3Frame::Sunken );
  grid->addMultiCellWidget( m_spacerLine, row, row, 0, 2 );
  ++row;

  if ( dataItems.count() == 1 ) {
      m_labelLocalNameText = new QLabel( i18n("Local name:"), plainPage() );
      grid->addWidget( m_labelLocalNameText, row, 0 );
      grid->addWidget( m_labelLocalName, row++, 2 );
      m_labelLocalLocationText = new QLabel( i18n("Local location:"), plainPage() );
      grid->addWidget( m_labelLocalLocationText, row, 0 );
      grid->addWidget( m_labelLocalLocation, row++, 2 );
  }

  grid->addColSpacing( 1, 50 );
  grid->setColStretch( 2, 1 );


  // OPTIONS
  // /////////////////////////////////////////////////
  QTabWidget* optionTab = new QTabWidget( plainPage() );
  m_spacerLine = new QFrame( plainPage() );
  m_spacerLine->setFrameStyle( Q3Frame::HLine | Q3Frame::Sunken );

  grid->addMultiCellWidget( m_spacerLine, 10, 10, 0, 2 );
  grid->addMultiCellWidget( optionTab, 12, 12, 0, 2 );
  grid->setRowStretch( 11, 1 );

  QWidget* hideBox = new QWidget( optionTab );
  Q3GridLayout* hideBoxGrid = new Q3GridLayout( hideBox );
  hideBoxGrid->setSpacing( spacingHint() );
  hideBoxGrid->setMargin( marginHint() );
  m_checkHideOnRockRidge = new QCheckBox( i18n("Hide on Rockridge"), hideBox );
  m_checkHideOnJoliet = new QCheckBox( i18n("Hide on Joliet"), hideBox );
  hideBoxGrid->addWidget( m_checkHideOnRockRidge, 0, 0 );
  hideBoxGrid->addWidget( m_checkHideOnJoliet, 1, 0 );
  hideBoxGrid->setRowStretch( 2, 1 );
//   grid->addMultiCellWidget( m_checkHideOnRockRidge, 10, 10, 0, 2 );
//   grid->addMultiCellWidget( m_checkHideOnJoliet, 11, 11, 0, 2 );

  QWidget* sortingBox = new QWidget( optionTab );
  Q3GridLayout* sortingBoxGrid = new Q3GridLayout( sortingBox );
  sortingBoxGrid->setSpacing( spacingHint() );
  sortingBoxGrid->setMargin( marginHint() );
  m_editSortWeight = new KLineEdit( sortingBox );
  m_editSortWeight->setValidator( new QIntValidator( -2147483647, 2147483647, m_editSortWeight ) );
  m_editSortWeight->setAlignment( Qt::AlignRight );
  sortingBoxGrid->addWidget( new QLabel( i18n("Sort weight:"), sortingBox ), 0, 0 );
  sortingBoxGrid->addWidget( m_editSortWeight, 0, 1 );
  sortingBoxGrid->setColStretch( 1, 1 );
  sortingBoxGrid->setRowStretch( 1, 1 );

  optionTab->addTab( hideBox, i18n("Settings") );
  optionTab->addTab( sortingBox, i18n("Advanced") );


  // load the data
  // ----------------------------
  if ( dataItems.count() == 1 ) {
      loadItemProperties( dataItems.first() );
  }
  else {
      loadListProperties( dataItems );
  }


  QToolTip::add( m_checkHideOnRockRidge, i18n("Hide this file in the RockRidge filesystem") );
  QToolTip::add( m_checkHideOnJoliet, i18n("Hide this file in the Joliet filesystem") );
  QToolTip::add( m_editSortWeight, i18n("Modify the physical sorting") );
  m_checkHideOnRockRidge->setWhatsThis( i18n("<p>If this option is checked, the file or directory "
						"(and its entire contents) will be hidden on the "
						"ISO9660 and RockRidge filesystem.</p>"
						"<p>This is useful, for example, for having different README "
						"files for RockRidge and Joliet, which can be managed "
						"by hiding README.joliet on RockRidge and README.rr "
						"on the Joliet filesystem.</p>") );
  m_checkHideOnJoliet->setWhatsThis( i18n("<p>If this option is checked, the file or directory "
					     "(and its entire contents) will be hidden on the "
					     "Joliet filesystem.</p>"
					     "<p>This is useful, for example, for having different README "
					     "files for RockRidge and Joliet, which can be managed "
					     "by hiding README.joliet on RockRidge and README.rr "
					     "on the Joliet filesystem.</p>") );
  m_editSortWeight->setWhatsThis( i18n("<p>This value modifies the physical sort order of the files "
					  "in the ISO9660 filesystem. A higher weighting means that the "
					  "file will be located closer to the beginning of the image "
					  "(and the disk)."
					  "<p>This option is useful in order to optimize the data layout "
					  "on a CD/DVD."
					  "<p><b>Caution:</b> This does not sort the order of the file "
					  "names that appear in the ISO9660 directory."
					  "It sorts the order in which the file data is "
					  "written to the image.") );
}


K3bDataPropertiesDialog::~K3bDataPropertiesDialog()
{
}


void K3bDataPropertiesDialog::loadItemProperties( K3bDataItem* dataItem )
{
  if( K3bFileItem* fileItem = dynamic_cast<K3bFileItem*>(dataItem) ) {
    KFileItem kFileItem( KFileItem::Unknown, KFileItem::Unknown, KUrl::fromPathOrUrl(fileItem->localPath()) );
    m_labelIcon->setPixmap( kFileItem.pixmap(KIconLoader::SizeLarge) );
    if( fileItem->isSymLink() )
      m_labelType->setText( i18n("Link to %1",kFileItem.mimeComment()) );
    else
      m_labelType->setText( kFileItem.mimeComment() );
    m_labelLocalName->setText( kFileItem.name() );
    QString localLocation = kFileItem.url().path(-1);
    localLocation.truncate( localLocation.findRev('/') );
    m_labelLocalLocation->setText( localLocation );
    m_labelSize->setText( KIO::convertSize(dataItem->size()) );
  }
  else if( K3bDirItem* dirItem = dynamic_cast<K3bDirItem*>(dataItem) ) {
    m_labelIcon->setPixmap( KMimeType::pixmapForURL( KUrl( "/" )) );
    m_labelType->setText( i18n("Directory") );
    m_labelLocalNameText->hide();
    m_labelLocalLocationText->hide();
    m_labelLocalName->hide();
    m_labelLocalLocation->hide();
    m_spacerLine->hide();
    m_labelSize->setText( KIO::convertSize(dataItem->size()) + "\n(" +
			  i18n("in 1 file", "in %n files", dirItem->numFiles()) + " " +
			  i18n("and 1 directory", "and %n directories", dirItem->numDirs()) + ")" );
  }
  else {
    m_labelIcon->setPixmap( DesktopIcon("unknown", KIconLoader::SizeLarge) );
    m_labelType->setText( i18n("Special file") );
    m_labelLocalName->hide();
    m_labelLocalLocation->hide();
    m_labelLocalNameText->hide();
    m_labelLocalLocationText->hide();
    m_spacerLine->hide();
    m_labelSize->setText( KIO::convertSize(dataItem->size()) );
  }

  m_editName->setText( dataItem->k3bName() );
  m_labelBlocks->setText( QString::number(dataItem->blocks().lba()) );

  QString location = "/" + dataItem->k3bPath();
  if( location[location.length()-1] == '/' )
    location.truncate( location.length()-1 );
  location.truncate( location.findRev('/') );
  if( location.isEmpty() )
    location = "/";
  m_labelLocation->setText( location );
  m_extraInfoLabel->setText( QString( "(%1)" ).arg(dataItem->extraInfo()) );
  if( dataItem->extraInfo().isEmpty() )
    m_extraInfoLabel->hide();

  m_checkHideOnJoliet->setChecked( dataItem->hideOnJoliet() );
  m_checkHideOnRockRidge->setChecked( dataItem->hideOnRockRidge() );
  m_editSortWeight->setText( QString::number(dataItem->sortWeight()) );

  // if the parent is hidden the value cannot be changed (see K3bDataItem::setHide...)
  if( dataItem->parent() ) {
    m_checkHideOnRockRidge->setDisabled( dataItem->parent()->hideOnRockRidge() );
    m_checkHideOnJoliet->setDisabled( dataItem->parent()->hideOnJoliet() );
  }

  if( !dataItem->isHideable() ) {
    m_checkHideOnJoliet->setDisabled(true);
    m_checkHideOnRockRidge->setDisabled(true);
    //    m_spacerLine->hide();
  }

  m_editName->setReadOnly( !dataItem->isRenameable() );
  m_editName->setFocus();
}


void K3bDataPropertiesDialog::loadListProperties( const Q3ValueList<K3bDataItem*>& items )
{
    m_labelIcon->setPixmap( DesktopIcon( "kmultiple", KIconLoader::SizeLarge ) );

    int files = 0;
    int folders = 0;
    KIO::filesize_t size = 0;
    K3b::Msf blocks = 0;
    for ( Q3ValueList<K3bDataItem*>::iterator it = m_dataItems.begin();
          it != m_dataItems.end(); ++it ) {
        K3bDataItem* item = *it;
        if ( item->isFile() )
            ++files;
        else if ( item->isDir() )
            ++folders;
        blocks += item->blocks();
        size += item->size();
    }
    QString s = i18n( "1 Item", "%n Items", items.count() );
    s += " - ";
    if ( files > 0 )
        s += i18n( "1 File", "%n Files", files );
    else
        s += "No Files";
    s += " - ";
    if ( folders > 0 )
        s += i18n( "1 Folder", "%n Folders", folders );
    else
        s += "No Folders";
    m_multiSelectionLabel->setText( s );

    m_labelSize->setText( KIO::convertSize(size) );
    m_labelBlocks->setText( QString::number(blocks.lba()) );

    // the location of all items are the same since it is not possible to
    // select items from different folders
    // FIXME: maybe better use QString::section?
    QString location = "/" + items.first()->k3bPath();
    if( location[location.length()-1] == '/' )
        location.truncate( location.length()-1 );
    location.truncate( location.findRev('/') );
    if( location.isEmpty() )
        location = "/";
    m_labelLocation->setText( location );


    m_checkHideOnJoliet->setChecked( items.first()->hideOnJoliet() );
    for ( Q3ValueList<K3bDataItem*>::iterator it = m_dataItems.begin();
          it != m_dataItems.end(); ++it ) {
        K3bDataItem* item = *it;
        if ( m_checkHideOnJoliet->isChecked() != item->hideOnJoliet() ) {
            m_checkHideOnJoliet->setNoChange();
            break;
        }
    }
    m_checkHideOnRockRidge->setChecked( items.first()->hideOnRockRidge() );
    for ( Q3ValueList<K3bDataItem*>::iterator it = m_dataItems.begin();
          it != m_dataItems.end(); ++it ) {
        K3bDataItem* item = *it;
        if ( m_checkHideOnRockRidge->isChecked() != item->hideOnRockRidge() ) {
            m_checkHideOnRockRidge->setNoChange();
            break;
        }
    }

    int weight = items.first()->sortWeight();
    for ( Q3ValueList<K3bDataItem*>::iterator it = m_dataItems.begin();
          it != m_dataItems.end(); ++it ) {
        K3bDataItem* item = *it;
        if ( weight != item->sortWeight() ) {
            weight = 0;
            break;
        }
    }
    m_editSortWeight->setText( QString::number( weight ) );
}


void K3bDataPropertiesDialog::slotOk()
{
    if ( m_dataItems.count() == 1 ) {
        m_dataItems.first()->setK3bName( m_editName->text() );
    }

    for ( Q3ValueList<K3bDataItem*>::iterator it = m_dataItems.begin();
          it != m_dataItems.end(); ++it ) {
        K3bDataItem* item = *it;
        if ( m_checkHideOnRockRidge->state() != QButton::NoChange )
            item->setHideOnRockRidge( m_checkHideOnRockRidge->isChecked() );
        if ( m_checkHideOnJoliet->state() != QButton::NoChange )
            item->setHideOnJoliet( m_checkHideOnJoliet->isChecked() );
        if ( m_editSortWeight->isModified() )
            item->setSortWeight( m_editSortWeight->text().toInt() );
    }

    KDialogBase::slotOk();
}


#include "k3bdatapropertiesdialog.moc"
