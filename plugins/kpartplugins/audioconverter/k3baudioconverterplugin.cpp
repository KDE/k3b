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

#include "k3baudioconverterplugin.h"
#include "base_k3baudioconverterwidget.h"
#include "k3baudioconverterviewitem.h"
#include "k3baudioconverterjob.h"
#include "k3baudioconverterview.h"

// the k3b stuff we need
#include <k3bcore.h>
#include <k3baudiodecoder.h>
#include <k3baudioencoder.h>
#include <k3bjobprogressdialog.h>
#include <k3bglobals.h>
#include <k3bpluginmanager.h>

#include <kdebug.h>
#include <kaction.h>
#include <kinstance.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kconfig.h>
#include <kgenericfactory.h>
#include <kurlrequester.h>
#include <kfiledialog.h>
#include <kurldrag.h>

#include <qstring.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qlayout.h>
#include <qintdict.h>
#include <qcombobox.h>
#include <qfileinfo.h>
#include <qevent.h>


class K3bAudioConverterPluginDialog::Private
{
public:
  QIntDict<K3bAudioEncoder> encoderMap;
  QMap<int, QString> extensionMap;
};


K3bAudioConverterPluginDialog::K3bAudioConverterPluginDialog( QWidget* parent,
							      const char* name )
  : K3bInteractionDialog( parent, name,
			  i18n("Convert Audio Files"),
			  i18n("from all into all supported audio formats"),
			  START_BUTTON|CANCEL_BUTTON,
			  START_BUTTON,
			  "Audio File Converter" ) // config group
{
  d = new Private();

  setStartButtonText( i18n("Start"), i18n("Start the conversion") );
  setCancelButtonText( i18n("Close") );

  m_w = new base_K3bAudioConverterWidget( this );
  setMainWidget( m_w );
  m_w->editDir->setMode( KFile::Directory );
  m_w->editDir->setCaption( i18n("Please Choose Destination Directory") );

  connect( m_w->buttonAddFiles, SIGNAL(clicked()),
	   this, SLOT(slotAddFiles()) );
  connect( m_w->buttonRemove, SIGNAL(clicked()),
	   this, SLOT(slotRemove()) );
  connect( m_w->buttonClear, SIGNAL(clicked()),
	   this, SLOT(slotClear()) );
  connect( m_w->buttonConfigure, SIGNAL(clicked()),
	   this, SLOT(slotConfigureEncoder()) );

  connect( m_w->comboFormat, SIGNAL(activated(int)), this, SLOT(slotToggleAll()) );
  connect( m_w->viewFiles, SIGNAL(dropped(QDropEvent*, QListViewItem*)),
	   this, SLOT(slotDropped(QDropEvent*)) );

  loadAudioEncoder();
  slotToggleAll();
}


K3bAudioConverterPluginDialog::~K3bAudioConverterPluginDialog()
{
  delete d;
}


void K3bAudioConverterPluginDialog::loadAudioEncoder()
{
  d->encoderMap.clear();
  m_w->comboFormat->clear();
  m_w->comboFormat->insertItem( i18n("Wave") );

  // check the available encoding plugins
  QPtrList<K3bPlugin> fl = k3bpluginmanager->plugins( "AudioEncoder" );
  for( QPtrListIterator<K3bPlugin> it( fl ); it.current(); ++it ) {
    K3bAudioEncoder* f = (K3bAudioEncoder*)it.current();
    QStringList exL = f->extensions();

    for( QStringList::const_iterator exIt = exL.begin();
	 exIt != exL.end(); ++exIt ) {
      d->extensionMap.insert( m_w->comboFormat->count(), *exIt );
      d->encoderMap.insert( m_w->comboFormat->count(), f );
      m_w->comboFormat->insertItem( f->fileTypeComment(*exIt) );
    }
  }
}


void K3bAudioConverterPluginDialog::slotAddFiles()
{
  KURL::List urls = KFileDialog::getOpenURLs( QString::null,
					      i18n("*|All Files"),
					      this,
					      i18n("Select Audio Files to Convert") );
  addFiles( urls );
}


void K3bAudioConverterPluginDialog::slotDropped( QDropEvent* e )
{
  KURL::List urls;
  KURLDrag::decode( e, urls );
  addFiles( urls );
}


void K3bAudioConverterPluginDialog::slotRemove()
{
  delete m_w->viewFiles->selectedItem();
}


void K3bAudioConverterPluginDialog::slotClear()
{
  m_w->viewFiles->clear();
}


void K3bAudioConverterPluginDialog::slotConfigureEncoder()
{
  K3bAudioEncoder* encoder = d->encoderMap[m_w->comboFormat->currentItem()];  // 0 for wave
  if( encoder )
    k3bpluginmanager->execPluginDialog( encoder, this );
}


void K3bAudioConverterPluginDialog::slotToggleAll()
{
  m_w->buttonConfigure->setEnabled( d->encoderMap[m_w->comboFormat->currentItem()] != 0 );  // 0 for wave
}


void K3bAudioConverterPluginDialog::loadK3bDefaults()
{
  m_w->comboFormat->setCurrentItem(0);
  m_w->editDir->setURL( K3b::defaultTempPath() );
}


void K3bAudioConverterPluginDialog::loadUserDefaults( KConfig* c )
{
  QString filetype = c->readEntry( "filetype", "wav" );
  if( filetype == "wav" )
    m_w->comboFormat->setCurrentItem(0);
  else {
    for( QMap<int, QString>::iterator it = d->extensionMap.begin();
	 it != d->extensionMap.end(); ++it ) {
      if( it.data() == filetype ) {
	m_w->comboFormat->setCurrentItem( it.key() );
	break;
      }
    }
  }

  m_w->editDir->setURL( c->readEntry( "destination dir", K3b::defaultTempPath() ) );
}


void K3bAudioConverterPluginDialog::saveUserDefaults( KConfig* c )
{
  if( d->extensionMap.contains(m_w->comboFormat->currentItem()) )
    c->writeEntry( "filetype", d->extensionMap[m_w->comboFormat->currentItem()] );
  else
    c->writeEntry( "filetype", "wav" );

  c->writeEntry( "destination dir", m_w->editDir->url() );
}


void K3bAudioConverterPluginDialog::slotStartClicked()
{
  if( !QFileInfo(m_w->editDir->url()).isDir() ) {
    KMessageBox::sorry( this, i18n("Please provide an existing destination directory.") );
  }
  else {
    K3bAudioEncoder* encoder = d->encoderMap[m_w->comboFormat->currentItem()];  // 0 for wave
    QString type;
    if( encoder )
      type = d->extensionMap[m_w->comboFormat->currentItem()];

    K3bJobProgressDialog* dlg = new K3bJobProgressDialog( this );
    K3bAudioConverterJob* job = new K3bAudioConverterJob( m_w->viewFiles, encoder, type, m_w->editDir->url(), dlg );

    hide();

    dlg->startJob( job );

    delete job;
    delete dlg;
  }
}


void K3bAudioConverterPluginDialog::addFiles( const KURL::List& urls )
{
  for( KURL::List::const_iterator it = urls.begin();
       it != urls.end(); ++it )
    addFile( *it );
}


void K3bAudioConverterPluginDialog::addFile( const KURL& url )
{
  K3bAudioDecoder* decoder = K3bAudioDecoderFactory::createDecoder( url );
  if( decoder )
    (void)new K3bAudioConverterViewItem( url.path(), decoder,
					 m_w->viewFiles, m_w->viewFiles->lastItem() );
  else
    KMessageBox::sorry( this, i18n("Unknown format: %1").arg(url.path()) );
}




K3bAudioConverterPlugin::K3bAudioConverterPlugin( QObject* parent,
						  const char* name,
						  const QStringList& )
  : KParts::Plugin( parent, name )
{
  (void) new KAction( i18n("C&onvert Audio Files..."),
		      0, 0,
		      this, SLOT(slotConvert()),
		      actionCollection(), "convert_audio_files_plugin" );
}


K3bAudioConverterPlugin::~K3bAudioConverterPlugin()
{
}


void K3bAudioConverterPlugin::slotConvert()
{
  K3bAudioConverterPluginDialog dlg( dynamic_cast<QWidget*>(parent()) );
  dlg.exec();
}


K_EXPORT_COMPONENT_FACTORY( libk3baudioconverterplugin, KGenericFactory<K3bAudioConverterPlugin> )

#include "k3baudioconverterplugin.moc"
