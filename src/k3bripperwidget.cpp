/***************************************************************************
                          k3bripperwidget.cpp  -  description
                             -------------------
    begin                : Tue Mar 27 2001
    copyright            : (C) 2001 by Sebastian Trueg
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

#include "k3bripperwidget.h"

#include <kcombobox.h>
#include <klistview.h>
#include <qgroupbox.h>
#include <qheader.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

K3bRipperWidget::K3bRipperWidget(QWidget *parent, const char *name )
	: QWidget(parent,name)
{
    Form1Layout = new QGridLayout( this );
    Form1Layout->setSpacing( 5 );
    Form1Layout->setMargin( 2 );

    GroupBox3 = new QGroupBox( this, "GroupBox3" );
    GroupBox3->setTitle( tr( "Reading Device" ) );
    GroupBox3->setColumnLayout(0, Qt::Vertical );
    GroupBox3->layout()->setSpacing( 0 );
    GroupBox3->layout()->setMargin( 0 );
    GroupBox3Layout = new QGridLayout( GroupBox3->layout() );
    GroupBox3Layout->setAlignment( Qt::AlignTop );
    GroupBox3Layout->setSpacing( 6 );
    GroupBox3Layout->setMargin( 11 );

    m_comboSource = new KComboBox( FALSE, GroupBox3, "m_comboSource" );

    GroupBox3Layout->addWidget( m_comboSource, 0, 0 );

    m_buttonRefresh = new QPushButton( GroupBox3, "m_buttonRefresh" );
    m_buttonRefresh->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)4, (QSizePolicy::SizeType)0, m_buttonRefresh->sizePolicy().hasHeightForWidth() ) );
    m_buttonRefresh->setText( tr( "Refresh" ) );

    GroupBox3Layout->addWidget( m_buttonRefresh, 0, 1 );

    Form1Layout->addMultiCellWidget( GroupBox3, 0, 0, 0, 2 );

    m_viewTracks = new KListView( this, "m_viewTracks" );
    m_viewTracks->addColumn( tr( "No" ) );
    m_viewTracks->header()->setClickEnabled( FALSE, m_viewTracks->header()->count() - 1 );
    m_viewTracks->header()->setResizeEnabled( FALSE, m_viewTracks->header()->count() - 1 );
    m_viewTracks->addColumn( tr( "Artist" ) );
    m_viewTracks->header()->setClickEnabled( FALSE, m_viewTracks->header()->count() - 1 );
    m_viewTracks->addColumn( tr( "Title" ) );
    m_viewTracks->header()->setClickEnabled( FALSE, m_viewTracks->header()->count() - 1 );
    m_viewTracks->addColumn( tr( "Length" ) );
    m_viewTracks->header()->setClickEnabled( FALSE, m_viewTracks->header()->count() - 1 );
    m_viewTracks->addColumn( tr( "Checksum" ) );
    m_viewTracks->header()->setClickEnabled( FALSE, m_viewTracks->header()->count() - 1 );

    Form1Layout->addMultiCellWidget( m_viewTracks, 1, 1, 0, 2 );

    TextLabel2 = new QLabel( this, "TextLabel2" );
    TextLabel2->setText( tr( "Path to rip to" ) );

    Form1Layout->addWidget( TextLabel2, 2, 0 );

    m_editRipPath = new QLineEdit( this, "m_editRipPath" );

    Form1Layout->addMultiCellWidget( m_editRipPath, 2, 2, 1, 2 );

    m_buttonStart = new QPushButton( this, "m_buttonStart" );
    m_buttonStart->setText( tr( "Start Ripping" ) );

    Form1Layout->addWidget( m_buttonStart, 3, 2 );
    QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    Form1Layout->addMultiCell( spacer, 3, 3, 0, 1 );
}


K3bRipperWidget::~K3bRipperWidget(){
}
