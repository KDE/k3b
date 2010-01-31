/*
 *
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

#include "k3bdevicemanager.h"
#include "k3bdevice.h"
#include "k3bdeviceglobals.h"
#include "k3bcore.h"
#include "k3bversion.h"
#include "k3bglobals.h"

#include <QTextEdit>
#include <qcursor.h>
#include <qfile.h>
#include <qclipboard.h>
#include <QTextStream>

#include <klocale.h>
#include <KStandardGuiItem>
#include <kglobalsettings.h>
#include <kapplication.h>
#include <kfiledialog.h>
#include <kmessagebox.h>


K3b::DebuggingOutputDialog::DebuggingOutputDialog( QWidget* parent )
  : KDialog( parent)
{
  setModal(true);
  setCaption(i18n("Debugging Output"));
  setButtons(Close|User1|User2);
  setDefaultButton(Close);
  setButtonGuiItem(User1, KStandardGuiItem::saveAs());
  setButtonGuiItem(User2, KGuiItem( i18n("Copy"), "edit-copy" ));
  setButtonToolTip( User1, i18n("Save to file") );
  setButtonToolTip( User2, i18n("Copy to clipboard") );

  debugView = new QTextEdit( this );
  debugView->setReadOnly(true);
  debugView->setAcceptRichText( false );
  debugView->setCurrentFont( KGlobalSettings::fixedFont() );
  debugView->setWordWrapMode( QTextOption::NoWrap );

  setMainWidget( debugView );
  connect( this, SIGNAL( user1Clicked() ), this, SLOT( slotUser1() ) );
  connect( this, SIGNAL( user2Clicked() ), this, SLOT( slotUser2() ) );

  resize( 600, 300 );
}


void K3b::DebuggingOutputDialog::setOutput( const QString& data )
{
  // the following may take some time
  QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

  debugView->setText( data );

  QApplication::restoreOverrideCursor();
}


void K3b::DebuggingOutputDialog::slotUser1()
{
  QString filename = KFileDialog::getSaveFileName();
  if( !filename.isEmpty() ) {
    QFile f( filename );
    if( !f.exists() || KMessageBox::warningContinueCancel( this,
						  i18n("Do you want to overwrite %1?",filename),
						  i18n("File Exists"), KStandardGuiItem::overwrite() )
	== KMessageBox::Continue ) {

      if( f.open( QIODevice::WriteOnly ) ) {
	QTextStream t( &f );
	t << debugView->toPlainText();
      }
      else {
	KMessageBox::error( this, i18n("Could not open file %1",filename) );
      }
    }
  }
}


void K3b::DebuggingOutputDialog::slotUser2()
{
  QApplication::clipboard()->setText( debugView->toPlainText(), QClipboard::Clipboard );
}

#include "k3bdebuggingoutputdialog.moc"
