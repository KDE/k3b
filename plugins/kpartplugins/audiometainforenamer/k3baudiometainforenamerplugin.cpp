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

#include "k3baudiometainforenamerplugin.h"

// the k3b stuff we need
#include <k3bcore.h>
#include <k3bprojectmanager.h>
#include <k3bdatadoc.h>
#include <k3bdataview.h>
#include <k3bdiritem.h>
#include <k3bfileitem.h>
#include <k3blistview.h>

#include <kdebug.h>
#include <kaction.h>
#include <kinstance.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kfilemetainfo.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <kgenericfactory.h>

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


class K3bAudioMetainfoRenamerPluginDialog::Private
{
public:
  K3bDataDoc* doc;
  QString pattern;

  QCheckBox* checkRecursive;
  QCheckBox* checkCompleteDoc;
  KComboBox* comboPattern;
  K3bListView* viewFiles;
  //  KProgressDialog* progressDialog;

  QValueList< QPair<K3bFileItem*, QCheckListItem*> > renamableItems;
  QPtrDict<QListViewItem> dirItemDict;

//   long long scannedSize;
//   int progressCounter;
};


K3bAudioMetainfoRenamerPluginDialog::K3bAudioMetainfoRenamerPluginDialog( K3bDataDoc* doc, 
									  QWidget* parent, 
									  const char* name )
  : K3bInteractionDialog( parent, name,
			  i18n("Rename Audio Files"),
			  i18n("Based on meta info"),
			  START_BUTTON|CANCEL_BUTTON|SAVE_BUTTON )
{
  d = new Private();
  d->doc = doc;
  //  d->progressDialog = 0;

  setStartButtonText( i18n("Scan"), i18n("Scan for renamable files") );
  setSaveButtonText( i18n("Apply"), i18n("Start the renaming") );
  setCancelButtonText( i18n("Close") );

  QWidget* main = mainWidget();

  // pattern group
  QGroupBox* patternGroup = new QGroupBox( 1, Qt::Vertical,
					   i18n("Rename Pattern"), main );
  patternGroup->setInsideMargin( marginHint() );
  patternGroup->setInsideSpacing( spacingHint() );

  d->comboPattern = new KComboBox( patternGroup );
  d->comboPattern->setEditable( true );


  // option group
  QGroupBox* optionGroup = new QGroupBox( 2, Qt::Horizontal,
					  i18n("Options"), main );
  optionGroup->setInsideMargin( marginHint() );
  optionGroup->setInsideSpacing( spacingHint() );

  d->checkRecursive = new QCheckBox( i18n("Recursive"), optionGroup );
  d->checkCompleteDoc = new QCheckBox( i18n("Complete project"), optionGroup );


  // the files view
  QGroupBox* filesGroup = new QGroupBox( 1, Qt::Horizontal,
					  i18n("Found Files"), main );
  filesGroup->setInsideMargin( marginHint() );
  filesGroup->setInsideSpacing( spacingHint() );

  d->viewFiles = new K3bListView( filesGroup );
  d->viewFiles->addColumn( i18n("New Name") );
  d->viewFiles->addColumn( i18n("Old Name") );
  d->viewFiles->setNoItemText( i18n("Please click the Scan button to search for renameable files.") );

  // layout
  QVBoxLayout* box = new QVBoxLayout( main );
  box->setMargin( 0 );
  box->setSpacing( spacingHint() );

  box->addWidget( patternGroup );
  box->addWidget( optionGroup );
  box->addWidget( filesGroup );


  connect( d->checkCompleteDoc, SIGNAL(toggled(bool)), d->checkRecursive, SLOT(setDisabled(bool)) ); 


  QToolTip::add( d->checkRecursive, i18n("Recurse into subdirectories") );
  QToolTip::add( d->checkCompleteDoc, i18n("Scan the whole project for renamable files") );
  QWhatsThis::add( d->comboPattern, i18n("<qt>This specifies how the files should be renamed. "
					 "Currently only the special strings <em>%a</em> (Artist), "
					 "<em>%n</em> (Track number), and <em>%t</em> (Title) ,"
					 "are supported.") );
  
  // we cannot apply without scanning first
  m_buttonSave->setEnabled( false );
					       
  slotLoadUserDefaults();
}


K3bAudioMetainfoRenamerPluginDialog::~K3bAudioMetainfoRenamerPluginDialog()
{
  delete d;
}


void K3bAudioMetainfoRenamerPluginDialog::slotLoadK3bDefaults()
{
  d->checkCompleteDoc->setChecked( false );
  d->checkRecursive->setChecked( false );
  d->comboPattern->setEditText( "%a - %t" );
}


void K3bAudioMetainfoRenamerPluginDialog::slotLoadUserDefaults()
{
  KConfig* c = k3bcore->config();
  c->setGroup( "audio_metainfo_renamer_plugin" );

  d->checkCompleteDoc->setChecked( c->readBoolEntry( "complete doc", false ) );
  d->checkRecursive->setChecked( c->readBoolEntry( "recursive", false ) );
  d->comboPattern->setEditText( c->readEntry( "rename pattern", "%a - %t" ) );
}


void K3bAudioMetainfoRenamerPluginDialog::slotSaveUserDefaults()
{
  KConfig* c = k3bcore->config();
  c->setGroup( "audio_metainfo_renamer_plugin" );

  c->writeEntry( "complete doc", d->checkCompleteDoc->isChecked() );
  c->writeEntry( "recursive", d->checkRecursive->isChecked() );
  c->writeEntry( "rename pattern", d->comboPattern->currentText() );
}


void K3bAudioMetainfoRenamerPluginDialog::slotStartClicked()
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

    K3bDirItem* dir = 0;
    K3bDataView* view = dynamic_cast<K3bDataView*>( d->doc->view() );
    if( !d->checkCompleteDoc->isChecked() && view )
      dir = view->currentDir();
    else
      dir = d->doc->root();

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
    m_buttonSave->setDisabled( d->renamableItems.isEmpty() );
  }
}


void K3bAudioMetainfoRenamerPluginDialog::scanDir( K3bDirItem* dir, QListViewItem* viewRoot )
{
  kdDebug() << "(K3bAudioMetainfoRenamerPluginDialog) scanning dir " << dir->k3bName() << endl;

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
      if( d->checkCompleteDoc->isChecked() || d->checkRecursive->isChecked() ) {
	// create dir item
	KListViewItem* dirViewItem = new KListViewItem( viewRoot, item->k3bName() );
	scanDir( (K3bDirItem*)item, dirViewItem );
	dirViewItem->setOpen(true);
      }
    }
  }
}


void K3bAudioMetainfoRenamerPluginDialog::slotSaveClicked()
{
  for( QValueList< QPair<K3bFileItem*, QCheckListItem*> >::iterator it = d->renamableItems.begin();
       it != d->renamableItems.end(); ++it ) {
    QPair<K3bFileItem*, QCheckListItem*>& item = *it;

    if( item.second->isOn() )
      item.first->setK3bName( item.second->text(0) );
  }

  d->viewFiles->clear();
  d->renamableItems.clear();
  m_buttonSave->setEnabled(false);

  KMessageBox::information( this, i18n("Done.") );
}


QString K3bAudioMetainfoRenamerPluginDialog::createNewName( K3bFileItem* item )
{
  // sometimes ogg-vorbis files go as "application/x-ogg"
  if( item->mimetype().contains( "audio" ) || item->mimetype().contains("ogg") ) {

    QString artist, title,track;

    KFileMetaInfo metaInfo( item->localPath() );
    if( metaInfo.isValid() ) {

      KFileMetaInfoItem artistItem = metaInfo.item( "Artist" );
      KFileMetaInfoItem titleItem = metaInfo.item( "Title" );
      KFileMetaInfoItem trackItem = metaInfo.item( "Tracknumber" );
      
      if( artistItem.isValid() )
	artist = artistItem.string();
      
      if( titleItem.isValid() )
	title = titleItem.string();
      
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
      if( find( item->parent(), newName + extension ) ) {
	kdDebug() << "(K3bAudioMetainfoRenamerPluginDialog) file with name " 
		  << newName << extension << " already exists" << endl;
	int i = 1;
	while( find( item->parent(), newName + QString( " (%1)").arg(i) + extension ) )
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


bool K3bAudioMetainfoRenamerPluginDialog::find( K3bDirItem* item, const QString& name )
{
  if( item->find( name ) )
    return true;

  QListViewItem* dirViewItem = d->dirItemDict[item];
  QListViewItem* current = dirViewItem->firstChild();
  while( current && current->parent() == dirViewItem ) {
    if( current->text(0) == name )
      return true;
    current = current->nextSibling();
  }

  return false;
}



K3bAudioMetainfoRenamerPlugin::K3bAudioMetainfoRenamerPlugin( QObject* parent, 
							      const char* name,
							      const QStringList& )
  : KParts::Plugin( parent, name )
{
  KAction* a = new KAction( i18n("&Rename Audio Files"),
			    0, 0,
			    this, SLOT(slotDoRename()),
			    actionCollection(), "rename_audio_files_plugin" );
  a->setToolTip( i18n("Rename audio files based on their meta info.") );
}


K3bAudioMetainfoRenamerPlugin::~K3bAudioMetainfoRenamerPlugin()
{
}


void K3bAudioMetainfoRenamerPlugin::slotDoRename()
{
  // 1. check if we have a K3bDataDoc
  K3bDataDoc* doc = dynamic_cast<K3bDataDoc*>( K3bProjectManager::instance()->activeDoc() );

  if( !doc ) {
    KMessageBox::sorry( 0, i18n("Please select a data project for renaming.") );
  }
  else {
    //
    // Now just get the dialog running: a K3bInteractionDialog with a CheckBox "recursive" and two 
    // radiobuttons "complete project", "current dir" and a listview which will show the files that 
    // can be renamed with the new name. And: a pattern editor (simpler as the one in the audio ripping)
    // The start button should first be named "scan" and then "apply" or "rename".
    //

    K3bAudioMetainfoRenamerPluginDialog dlg(doc);
    dlg.exec();
  }
}


K_EXPORT_COMPONENT_FACTORY( libk3baudiometainforenamerplugin, KGenericFactory<K3bAudioMetainfoRenamerPlugin> )

#include "k3baudiometainforenamerplugin.moc"
