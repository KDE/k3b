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
#include "k3bcddb.h"

#include <qstringlist.h>
#include <qlayout.h>
#include <qframe.h>
#include <qmessagebox.h>
#include <qhgroupbox.h>

#include <klistbox.h>
#include <klocale.h>

K3bCddbMultiEntriesDialog::K3bCddbMultiEntriesDialog(  QStringList &entries, const char name=0 )
   : KDialogBase( Plain, i18n("CDDB Database Entry"), Apply|Cancel, Apply, 0, "CDDB_selection" ) {

   init( );
   setup( entries );
}

K3bCddbMultiEntriesDialog::~K3bCddbMultiEntriesDialog(){
}

void K3bCddbMultiEntriesDialog::setup( QStringList &entries ){

    QFrame *_frame = plainPage( );
    setMainWidget(_frame);
    setMinimumWidth(400);
    setMinimumHeight(150);
    QHGroupBox *_box = new QHGroupBox( _frame, "group_box");
    _box->setTitle( i18n( "Inexact cddb requests" ) );
    QGridLayout *frameLayout = new QGridLayout( _frame );
    frameLayout->setSpacing( KDialog::spacingHint() );
    frameLayout->setMargin( KDialog::marginHint() );

    m_listBox = new KListBox( _box, "list_box");
    QStringList::Iterator it;
    for( it = entries.begin(); it != entries.end(); ++it ){
          if( *it != ".")
            m_listBox->insertItem( *it );
    }

    frameLayout->addWidget( _box, 0, 0);
    setButtonApplyText( i18n( "Ok" ) );
}

void K3bCddbMultiEntriesDialog::init( ){
    connect(this, SIGNAL(applyClicked()), this, SLOT( apply()) );
}

void K3bCddbMultiEntriesDialog::apply( ){
    unsigned int id = 0;
    // get selected entry
    int selected = m_listBox->currentItem();
    if ( selected != -1 ){
        QString entry = m_listBox->currentText();
        qDebug("Entry: " + entry);
        QStringList line = QStringList::split( " ", entry );
        bool ok;
        id = (*line.at( 1 )).toUInt( &ok, 16);
        qDebug("(K3bCddbMultiEntriesDialog) Chosen Id: " + QString::number(id, 16) );
    } else {
        QMessageBox::critical( this, i18n("CDDB Error"), i18n("You must choose an entry."), i18n("Ok") );
        return;
    }
    emit chosenId( id );
    done(0);
}

#include "k3bcddbmultientriesdialog.moc"



