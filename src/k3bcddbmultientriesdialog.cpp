/***************************************************************************
                          k3bcddbmultientriesdialog.cpp  -  description
                             -------------------
    begin                : Sun Feb 10 2002
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

#include "k3bcddbmultientriesdialog.h"

#include <qlayout.h>
#include <qframe.h>
#include <qlabel.h>

#include <klistbox.h>
#include <klocale.h>



K3bCddbMultiEntriesDialog::K3bCddbMultiEntriesDialog( QWidget* parent, const char* name )
   : KDialogBase( Plain, i18n("CDDB Database Entry"), Ok, Ok, parent, name ) 
{
  QFrame* frame = plainPage();
  QVBoxLayout* layout = new QVBoxLayout( frame );
  layout->setAutoAdd( true );
  layout->setSpacing( spacingHint() );
  layout->setMargin( 0 );

  QLabel* infoLabel = new QLabel( i18n("K3b found multiple inexact CDDB entries. Please select one."), frame );
  infoLabel->setAlignment( WordBreak );

  m_listBox = new KListBox( frame, "list_box");

  setMinimumSize( 280, 200 );
}

int K3bCddbMultiEntriesDialog::selectCddbEntry( const K3bCddbResult& query, QWidget* parent )
{
  K3bCddbMultiEntriesDialog d( parent );

  for( int i = 0; i < query.foundEntries(); i++ ) {
    d.m_listBox->insertItem( QString::number(i) + " " + 
			     query.entry(i).cdArtist + " - " + 
			     query.entry(i).cdTitle + " (" + 
			     query.entry(i).category + ")" );
  }

  d.m_listBox->setSelected( 0, true );

  d.exec();
  return ( d.m_listBox->currentItem() >= 0 ? d.m_listBox->currentItem() : 0 );
}


K3bCddbMultiEntriesDialog::~K3bCddbMultiEntriesDialog(){
}


#include "k3bcddbmultientriesdialog.moc"
