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

#include <KLocalizedString>
#include <KStandardGuiItem>
#include <KMessageBox>

#include <QFile>
#include <QTextStream>
#include <QClipboard>
#include <QCursor>
#include <QFontDatabase>
#include <QApplication>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>


K3b::DebuggingOutputDialog::DebuggingOutputDialog( QWidget* parent )
  : QDialog( parent)
{
  setModal(true);
  setWindowTitle(i18n("Debugging Output"));

  debugView = new QTextEdit( this );
  debugView->setReadOnly(true);
  debugView->setAcceptRichText( false );
  debugView->setCurrentFont( QFontDatabase::systemFont( QFontDatabase::FixedFont ) );
  debugView->setWordWrapMode( QTextOption::NoWrap );

  QPushButton* saveButton = new QPushButton( this );
  KStandardGuiItem::assign( saveButton, KStandardGuiItem::SaveAs );
  saveButton->setToolTip( i18n("Save to file") );
  connect( saveButton, SIGNAL(clicked()), this, SLOT(slotSaveAsClicked()) );

  QPushButton* copyButton = new QPushButton( this );
  KGuiItem::assign( copyButton, KGuiItem( i18n("Copy"), QString::fromLatin1( "edit-copy" ), i18n("Copy to clipboard") ) );
  connect( copyButton, SIGNAL(clicked()), this, SLOT(slotCopyClicked()) );

  QDialogButtonBox* buttonBox = new QDialogButtonBox( this );
  buttonBox->addButton( QDialogButtonBox::Close );
  buttonBox->addButton( saveButton, QDialogButtonBox::NoRole );
  buttonBox->addButton( copyButton, QDialogButtonBox::NoRole );
  connect( buttonBox->button( QDialogButtonBox::Close ), SIGNAL(clicked()), this, SLOT(accept()) );

  QVBoxLayout* layout = new QVBoxLayout( this );
  layout->addWidget( debugView );
  layout->addWidget( buttonBox );

  resize( 600, 300 );
}


void K3b::DebuggingOutputDialog::setOutput( const QString& data )
{
  // the following may take some time
  QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

  debugView->setText( data );

  QApplication::restoreOverrideCursor();
}


void K3b::DebuggingOutputDialog::slotSaveAsClicked()
{
  QString filename = QFileDialog::getSaveFileName( this );
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


void K3b::DebuggingOutputDialog::slotCopyClicked()
{
  QApplication::clipboard()->setText( debugView->toPlainText(), QClipboard::Clipboard );
}


