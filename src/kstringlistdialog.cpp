/***************************************************************************
                          kstringlistdialog.cpp  -  description
                             -------------------
    begin                : Fri Mar 8 2002
    copyright            : (C) 2002 by Sebastian Trueg
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
  // TODO: fixme
  return KDialogBase::sizeHint();
}

#include "kstringlistdialog.moc"
