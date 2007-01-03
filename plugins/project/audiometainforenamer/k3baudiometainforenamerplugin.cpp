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

#include <config.h>

#include "k3baudiometainforenamerplugin.h"

// the k3b stuff we need
#include <k3bcore.h>
#include <k3bdatadoc.h>
#include <k3bdiritem.h>
#include <k3bfileitem.h>
#include <k3blistview.h>
#include <k3bpluginfactory.h>

#include <kdebug.h>
#include <kinstance.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kfilemetainfo.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <kmimetype.h>
#include <kdialog.h>

#include <qstring.h>
#include <qfile.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qpushbutton.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qpair.h>
#include <qvaluelist.h>
#include <qlayout.h>
#include <qptrdict.h>



K_EXPORT_COMPONENT_FACTORY( libk3baudiometainforenamerplugin, K3bPluginFactory<K3bAudioMetainfoRenamerPlugin>("libk3baudiometainforenamerplugin") )


class K3bAudioMetainfoRenamerPluginWidget::Private
{
public:
  K3bDataDoc* doc;
  QString pattern;

  KComboBox* comboPattern;
  K3bListView* viewFiles;
  //  KProgressDialog* progressDialog;
  QPushButton* scanButton;

  QValueList< QPair<K3bFileItem*, QCheckListItem*> > renamableItems;
  QPtrDict<QListViewItem> dirItemDict;

//   long long scannedSize;
//   int progressCounter;
};


K3bAudioMetainfoRenamerPluginWidget::K3bAudioMetainfoRenamerPluginWidget( K3bDoc* doc, 
									  QWidget* parent, 
									  const char* name )
  : QWidget( parent, name )
{
  d = new Private();
  d->doc = dynamic_cast<K3bDataDoc*>(doc);
  //  d->progressDialog = 0;

  // pattern group
  QGroupBox* patternGroup = new QGroupBox( 2, Qt::Horizontal,
					   i18n("Rename Pattern"), this );
  patternGroup->setInsideMargin( KDialog::marginHint() );
  patternGroup->setInsideSpacing( KDialog::spacingHint() );

  d->comboPattern = new KComboBox( patternGroup );
  d->comboPattern->setEditable( true );

  d->scanButton = new QPushButton( i18n("Scan"), patternGroup );

  // the files view
  QGroupBox* filesGroup = new QGroupBox( 1, Qt::Horizontal,
					  i18n("Found Files"), this );
  filesGroup->setInsideMargin( KDialog::marginHint() );
  filesGroup->setInsideSpacing( KDialog::spacingHint() );

  d->viewFiles = new K3bListView( filesGroup );
  d->viewFiles->addColumn( i18n("New Name") );
  d->viewFiles->addColumn( i18n("Old Name") );
  d->viewFiles->setNoItemText( i18n("Please click the Scan button to search for renameable files.") );

  // layout
  QVBoxLayout* box = new QVBoxLayout( this );
  box->setMargin( 0 );
  box->setSpacing( KDialog::spacingHint() );

  box->addWidget( patternGroup );
  box->addWidget( filesGroup );

  connect( d->scanButton, SIGNAL(clicked()), this, SLOT(slotScanClicked()) );

  QToolTip::add( d->scanButton, i18n("Scan for renamable files") );
  QWhatsThis::add( d->comboPattern, i18n("<qt>This specifies how the files should be renamed. "
					 "Currently only the special strings <em>%a</em> (Artist), "
					 "<em>%n</em> (Track number), and <em>%t</em> (Title) ,"
					 "are supported.") );
}


K3bAudioMetainfoRenamerPluginWidget::~K3bAudioMetainfoRenamerPluginWidget()
{
  delete d;
}


QString K3bAudioMetainfoRenamerPluginWidget::title() const
{
  return i18n("Rename Audio Files");
}


QString K3bAudioMetainfoRenamerPluginWidget::subTitle() const
{
  return i18n("Based on meta info");
}


void K3bAudioMetainfoRenamerPluginWidget::loadDefaults()
{
  d->comboPattern->setEditText( "%a - %t" );
}


void K3bAudioMetainfoRenamerPluginWidget::readSettings( KConfigBase* c )
{
  d->comboPattern->setEditText( c->readEntry( "rename pattern", "%a - %t" ) );
}


void K3bAudioMetainfoRenamerPluginWidget::saveSettings( KConfigBase* c )
{
  c->writeEntry( "rename pattern", d->comboPattern->currentText() );
}


void K3bAudioMetainfoRenamerPluginWidget::slotScanClicked()
{
  d->pattern = d->comboPattern->currentText();
  if( d->pattern.isEmpty() ) {
    KMessageBox::error( this, i18n("Please specify a valid pattern.") );
  }
  else {
//     if( d->progressDialog == 0 ) {
//       d->progressDialog = new KProgressDialog( this, "scanning_progress",
// 					       i18n("Scanning..."),
// 					       i18n("Scanning for renameable files."),
// 					       true );
//       d->progressDialog->setAllowCancel(false);
//     }

    K3bDirItem* dir = d->doc->root();

    // clear old searches
    d->viewFiles->clear();
    d->renamableItems.clear();
    d->dirItemDict.clear();
//     d->scannedSize = 0;
//     d->progressCounter = 0;

    // create root item
    KListViewItem* rootItem = new KListViewItem( d->viewFiles, "/" );

    //  d->progressDialog->show();
    scanDir( dir, rootItem );
    //    d->progressDialog->close();

    rootItem->setOpen(true);

    if( d->renamableItems.isEmpty() )
      KMessageBox::sorry( this, i18n("No renameable files found.") );
  }
}


void K3bAudioMetainfoRenamerPluginWidget::scanDir( K3bDirItem* dir, QListViewItem* viewRoot )
{
  kdDebug() << "(K3bAudioMetainfoRenamerPluginWidget) scanning dir " << dir->k3bName() << endl;

  d->dirItemDict.insert( dir, viewRoot );

  for( QPtrListIterator<K3bDataItem> it( dir->children() ); it.current(); ++it ) {
    K3bDataItem* item = it.current();

    if( item->isFile() ) {
      if( item->isRenameable() ) {
	QString newName = createNewName( (K3bFileItem*)item );
	if( !newName.isEmpty() ) {
	  QCheckListItem* fileViewItem =  new QCheckListItem( viewRoot, 
							      newName, 
							      QCheckListItem::CheckBox );
	  fileViewItem->setText(1, item->k3bName() );
	  fileViewItem->setOn(true);
	  d->renamableItems.append( qMakePair( (K3bFileItem*)item, fileViewItem ) );
	}
      }

//       d->scannedSize += item->k3bSize();
//       d->progressCounter++;
//       if( d->progressCounter > 50 ) {
// 	d->progressCounter = 0;
// 	d->progressDialog->progressBar()->setProgress( 100*d->scannedSize/d->doc->root()->k3bSize() );
// 	qApp->processEvents();
//       }
    }
    else if( item->isDir() ) {
      // create dir item
      KListViewItem* dirViewItem = new KListViewItem( viewRoot, item->k3bName() );
      scanDir( (K3bDirItem*)item, dirViewItem );
      dirViewItem->setOpen(true);
    }
  }
}


void K3bAudioMetainfoRenamerPluginWidget::activate()
{
  if( d->renamableItems.isEmpty() ) {
    KMessageBox::sorry( this, i18n("Please click the Scan button to search for renameable files.") );
  }
  else {
    for( QValueList< QPair<K3bFileItem*, QCheckListItem*> >::iterator it = d->renamableItems.begin();
	 it != d->renamableItems.end(); ++it ) {
      QPair<K3bFileItem*, QCheckListItem*>& item = *it;
      
      if( item.second->isOn() )
	item.first->setK3bName( item.second->text(0) );
    }
    
    d->viewFiles->clear();
    d->renamableItems.clear();
    
    KMessageBox::information( this, i18n("Done.") );
  }
}


QString K3bAudioMetainfoRenamerPluginWidget::createNewName( K3bFileItem* item )
{
  KMimeType::Ptr mimetype = KMimeType::findByPath( item->localPath() );
  // sometimes ogg-vorbis files go as "application/x-ogg"
  if( mimetype != 0 && 
      ( mimetype->name().contains( "audio" ) || mimetype->name().contains("ogg") ) ) {

    QString artist, title, track;

    KFileMetaInfo metaInfo( item->localPath() );
    if( metaInfo.isValid() ) {

      KFileMetaInfoItem artistItem = metaInfo.item( "Artist" );
      KFileMetaInfoItem titleItem = metaInfo.item( "Title" );
      KFileMetaInfoItem trackItem = metaInfo.item( "Tracknumber" );
      
      if( artistItem.isValid() )
	artist = artistItem.string().stripWhiteSpace();
      
      if( titleItem.isValid() )
	title = titleItem.string().stripWhiteSpace();
      
      if( trackItem.isValid() )
	track = track.sprintf("%02d",trackItem.string().toInt());
    }

    QString newName;
    for( unsigned int i = 0; i < d->pattern.length(); ++i ) {

      if( d->pattern[i] == '%' ) {
	++i;

	if( i < d->pattern.length() ) {
	  if( d->pattern[i] == 'a' ) {
	    if( artist.isEmpty() )
	      return QString::null;
	    newName.append(artist);
	  }
	  else if( d->pattern[i] == 'n' ) {
	    if( title.isEmpty() )
	      return QString::null;
	    newName.append(track);
	  }
	  else if( d->pattern[i] == 't' ) {
	    if( title.isEmpty() )
	      return QString::null;
	    newName.append(title);
	  }
	  else {
	    newName.append( "%" );
	    newName.append( d->pattern[i] );
	  }
	}
	else {  // end of pattern
	  newName.append( "%" );
	}
      }
      else {
	newName.append( d->pattern[i] );
      }
    }

    // remove white spaces from end and beginning
    newName = newName.stripWhiteSpace();

    QString extension = item->k3bName().mid( item->k3bName().findRev(".") );

    if( !newName.isEmpty() ) {
      //
      // Check if files with that name exists and if so append number
      //
      if( existsOtherItemWithSameName( item, newName + extension ) ) {
	kdDebug() << "(K3bAudioMetainfoRenamerPluginWidget) file with name " 
		  << newName << extension << " already exists" << endl;
	int i = 1;
	while( existsOtherItemWithSameName( item, newName + QString( " (%1)").arg(i) + extension ) )
	  i++;
	newName.append( QString( " (%1)").arg(i) );
      }

      // append extension
      newName.append( extension );
    }

    return newName;
  }
  else
    return QString::null;
}


bool K3bAudioMetainfoRenamerPluginWidget::existsOtherItemWithSameName( K3bFileItem* item, const QString& name )
{
  K3bDirItem* dir = item->parent();
  K3bDataItem* otherItem = dir->find( name );
  if( otherItem && otherItem != item )
    return true;

  QListViewItem* dirViewItem = d->dirItemDict[dir];
  QListViewItem* current = dirViewItem->firstChild();
  while( current && current->parent() == dirViewItem ) {
    if( current->text(0) == name )
      return true;
    current = current->nextSibling();
  }

  return false;
}



K3bAudioMetainfoRenamerPlugin::K3bAudioMetainfoRenamerPlugin( QObject* parent, 
							      const char* name )
  : K3bProjectPlugin( DATA_PROJECTS, true, parent, name )
{
  setText( i18n("Rename Audio Files") );
  setToolTip( i18n("Rename audio files based on their meta info.") );
}


K3bAudioMetainfoRenamerPlugin::~K3bAudioMetainfoRenamerPlugin()
{
}


K3bProjectPluginGUIBase* K3bAudioMetainfoRenamerPlugin::createGUI( K3bDoc* doc, QWidget* parent, const char* name )
{
  return new K3bAudioMetainfoRenamerPluginWidget( doc, parent, name );
}

#include "k3baudiometainforenamerplugin.moc"
