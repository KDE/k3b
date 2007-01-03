/* 
 *
 * $Id$
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bdebuggingoutputdialog.h"

#include <k3bdevicemanager.h>
#include <k3bdevice.h>
#include <k3bdeviceglobals.h>
#include <k3bcore.h>
#include <k3bversion.h>
#include <k3bglobals.h>

#include <qtextedit.h>
#include <qcursor.h>
#include <qfile.h>
#include <qclipboard.h>

#include <klocale.h>
#include <kstdguiitem.h>
#include <kglobalsettings.h>
#include <kapplication.h>
#include <kfiledialog.h>
#include <kmessagebox.h>


K3bDebuggingOutputDialog::K3bDebuggingOutputDialog( QWidget* parent )
  : KDialogBase( parent, "debugViewDialog", true, i18n("Debugging Output"), Close|User1|User2, Close, 
		 false, 
		 KStdGuiItem::saveAs(), 
		 KGuiItem( i18n("Copy"), "editcopy" ) )
{
  setButtonTip( User1, i18n("Save to file") );
  setButtonTip( User2, i18n("Copy to clipboard") );

  debugView = new QTextEdit( this );
  debugView->setReadOnly(true);
  debugView->setTextFormat( QTextEdit::PlainText );
  debugView->setCurrentFont( KGlobalSettings::fixedFont() );
  debugView->setWordWrap( QTextEdit::NoWrap );

  setMainWidget( debugView );

  resize( 600, 300 );
}


void K3bDebuggingOutputDialog::setOutput( const QMap<QString, QStringList>& map )
{
  // the following may take some time
  QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

  clear();

  // add the debugging output
  for( QMap<QString, QStringList>::ConstIterator itMap = map.begin(); itMap != map.end(); ++itMap ) {
    const QStringList& list = itMap.data();
    debugView->append( itMap.key() + "\n" );
    debugView->append( "-----------------------\n" );
    for( QStringList::ConstIterator it = list.begin(); it != list.end(); ++it ) {
       QStringList lines = QStringList::split( "\n", *it );
       // do every line
       QStringList::ConstIterator end( lines.end() );
       for( QStringList::ConstIterator str = lines.begin(); str != end; ++str )
	 debugView->append( *str + "\n" );
    }
    m_paragraphMap[itMap.key()] = debugView->paragraphs();
    debugView->append( "\n" );
  }

  QApplication::restoreOverrideCursor();
}


void K3bDebuggingOutputDialog::addOutput( const QString& app, const QString& msg )
{
  QMap<QString, int>::Iterator it = m_paragraphMap.find( app );

  if( it == m_paragraphMap.end() ) {
    // create new section
    debugView->append( app + "\n" );
    debugView->append( "-----------------------\n" );
    debugView->append( msg + "\n" );
    m_paragraphMap[app] = debugView->paragraphs();
    debugView->append( "\n" );
  }
  else {
    debugView->insertParagraph( msg, *it );
    // update the paragraphs map 
    // FIXME: we cannot count on the map to be sorted properly!
    while( it != m_paragraphMap.end() ) {
      it.data() += 1;
      ++it;
    }
  }
}


void K3bDebuggingOutputDialog::clear()
{
  debugView->clear();
  m_paragraphMap.clear();

  addOutput( "System", "K3b Version: " + k3bcore->version() );
  addOutput( "System", "KDE Version: " + QString(KDE::versionString()) );
  addOutput( "System", "QT Version:  " + QString(qVersion()) );
  addOutput( "System", "Kernel:      " + K3b::kernelVersion() );
  
  // devices in the logfile
  for( QPtrListIterator<K3bDevice::Device> it( k3bcore->deviceManager()->allDevices() ); *it; ++it ) {
    K3bDevice::Device* dev = *it;
    addOutput( "Devices", 
	       QString( "%1 (%2, %3) [%5] [%6] [%7]" )
	       .arg( dev->vendor() + " " + dev->description() + " " + dev->version() )
	       .arg( dev->blockDeviceName() )
	       .arg( dev->genericDevice() )
	       .arg( K3bDevice::deviceTypeString( dev->type() ) )
	       .arg( K3bDevice::mediaTypeString( dev->supportedProfiles() ) )
	       .arg( K3bDevice::writingModeString( dev->writingModes() ) ) );
  }
}


void K3bDebuggingOutputDialog::slotUser1()
{
  QString filename = KFileDialog::getSaveFileName();
  if( !filename.isEmpty() ) {
    QFile f( filename );
    if( !f.exists() || KMessageBox::warningContinueCancel( this,
						  i18n("Do you want to overwrite %1?").arg(filename),
						  i18n("File Exists"), i18n("Overwrite") )
	== KMessageBox::Continue ) {

      if( f.open( IO_WriteOnly ) ) {
	QTextStream t( &f );
	t << debugView->text();
      }
      else {
	KMessageBox::error( this, i18n("Could not open file %1").arg(filename) );
      }
    }
  }
}


void K3bDebuggingOutputDialog::slotUser2()
{
  QApplication::clipboard()->setText( debugView->text(), QClipboard::Clipboard );
}

#include "k3bdebuggingoutputdialog.moc"
