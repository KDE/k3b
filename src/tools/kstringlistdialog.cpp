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


#include "kstringlistdialog.h"

#include <qlayout.h>
#include <qlabel.h>
#include <klistbox.h>


KStringListDialog::KStringListDialog( const QStringList& list, const QString& caption, 
				      const QString& message, bool modal, 
				      QWidget *parent, const char *name ) 
  : KDialogBase( parent, name, modal, caption, Ok ) 
{
  QWidget* main = new QWidget( this );
  QVBoxLayout* layout = new QVBoxLayout( main );
  layout->setAutoAdd( true );
  layout->setMargin( marginHint() );
  layout->setSpacing( spacingHint() );

  QLabel* label = new QLabel( message, main );
  KListBox* box = new KListBox( main );
  box->insertStringList( list );

  setMainWidget( main );
}


KStringListDialog::~KStringListDialog()
{
}

QSize KStringListDialog::sizeHint() const
{
  return KDialogBase::sizeHint().expandedTo( QSize(350, 250) );
}

#include "kstringlistdialog.moc"
