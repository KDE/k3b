/***************************************************************************
                          k3bvcdtrackdialog.cpp  -  description
                             -------------------
    begin                : Don Jan 16 2003
    copyright            : (C) 2003 by Sebastian Trueg, Christian Kvasny
    email                : trueg@informatik.uni-freiburg.de
                           chris@ckvsoft.at
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include <qlineedit.h>
#include <qmultilineedit.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qframe.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qhbox.h>
#include <qtabwidget.h>

#include <kiconloader.h>
#include <klocale.h>
#include <knuminput.h>
#include <kmimetype.h>
#include <kurl.h>

#include "k3bvcdtrackdialog.h"
#include "k3bvcdtrack.h"
#include "../kcutlabel.h"
#include "../tools/k3bglobals.h"


K3bVcdTrackDialog::K3bVcdTrackDialog( QPtrList<K3bVcdTrack>& tracks, QWidget *parent, const char *name )
  : KDialogBase( KDialogBase::Plain, i18n("Video Track Properties"), KDialogBase::Ok,
		 KDialogBase::Ok, parent, name )
{
  setupGui();

  m_tracks = tracks;

  if( !m_tracks.isEmpty() ) {

    K3bVcdTrack* track = m_tracks.first();

    m_displayFileName->setText( track->fileName() );
    m_displayLength->setText( track->mpegDuration() );
    m_displaySize->setText( i18n("%1 kb").arg(track->size() / 1024) );

    m_labelMimeType->setPixmap( KMimeType::pixmapForURL( KURL(m_tracks.first()->absPath()), 0, KIcon::Desktop, 48 ) );
   
  }
}

K3bVcdTrackDialog::~K3bVcdTrackDialog()
{
}


void K3bVcdTrackDialog::slotOk()
{
  slotApply();
  done(0);
}


void K3bVcdTrackDialog::setupGui()
{
  QFrame* frame = plainPage();

  QGridLayout* mainLayout = new QGridLayout( frame );
  mainLayout->setSpacing( spacingHint() );
  mainLayout->setMargin( 0 );

  QTabWidget* mainTabbed = new QTabWidget( frame );


  // /////////////////////////////////////////////////
  // AUDIO TAB
  // /////////////////////////////////////////////////
  // /////////////////////////////////////////////////
  QWidget* audioTab = new QWidget( mainTabbed );
  QGridLayout* audioGrid = new QGridLayout( audioTab );
  audioGrid->setSpacing( spacingHint() );
  audioGrid->setMargin( marginHint() );

  audioGrid->setRowStretch( 3, 1 );
  // /////////////////////////////////////////////////
  // /////////////////////////////////////////////////



  // /////////////////////////////////////////////////
  // VIDEO TAB
  // /////////////////////////////////////////////////
  // /////////////////////////////////////////////////
  QWidget* videoTab = new QWidget( mainTabbed );
  QGridLayout* videoTabLayout = new QGridLayout( videoTab );
  videoTabLayout->setAlignment( Qt::AlignTop );
  videoTabLayout->setSpacing( spacingHint() );
  videoTabLayout->setMargin( marginHint() );

  // /////////////////////////////////////////////////
  // /////////////////////////////////////////////////


  // /////////////////////////////////////////////////
  // FILE-INFO BOX
  // /////////////////////////////////////////////////
  // /////////////////////////////////////////////////

  QGroupBox* groupFileInfo = new QGroupBox( 0, Qt::Vertical, i18n( "File Info" ), frame, "groupFileInfo" );
  groupFileInfo->layout()->setSpacing( 0 );
  groupFileInfo->layout()->setMargin( 0 );
  QGridLayout* groupFileInfoLayout = new QGridLayout( groupFileInfo->layout() );
  groupFileInfoLayout->setAlignment( Qt::AlignTop );
  groupFileInfoLayout->setSpacing( spacingHint() );
  groupFileInfoLayout->setMargin( marginHint() );

  m_labelMimeType = new QLabel( groupFileInfo, "m_labelMimeType" );
  m_displayFileName = new KCutLabel( groupFileInfo );
  m_displayFileName->setText( i18n( "Filename" ) );
  m_displayFileName->setAlignment( int( QLabel::AlignTop | QLabel::AlignLeft ) );
  QLabel* labelSize = new QLabel( i18n( "Size" ), groupFileInfo, "labelSize" );
  QLabel* labelLength = new QLabel( i18n( "Length"), groupFileInfo, "labelLength" );
  m_displaySize = new QLabel( groupFileInfo, "m_displaySize" );
  m_displaySize->setText( i18n( "0.0 MB" ) );
  m_displaySize->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );
  m_displayLength = new QLabel( groupFileInfo, "m_displayLength" );
  m_displayLength->setText( i18n( "0:0:0" ) );
  m_displayLength->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );

  QFrame* fileInfoLine = new QFrame( groupFileInfo );
  fileInfoLine->setFrameStyle( QFrame::HLine | QFrame::Sunken );

  groupFileInfoLayout->addWidget( m_labelMimeType, 0, 0 );
  groupFileInfoLayout->addMultiCellWidget( m_displayFileName, 0, 1, 1, 2 );
  groupFileInfoLayout->addMultiCellWidget( fileInfoLine, 2, 2, 0, 2 );
  groupFileInfoLayout->addWidget( labelLength, 3, 0 );
  groupFileInfoLayout->addWidget( labelSize, 4, 0 );
  groupFileInfoLayout->addWidget( m_displayLength, 3, 2 );
  groupFileInfoLayout->addWidget( m_displaySize, 4, 2 );


  groupFileInfoLayout->setRowStretch( 5, 1 );


  QFont f( m_displayLength->font() );
  f.setBold( true );
  m_displayLength->setFont( f );
  m_displaySize->setFont( f );
  // /////////////////////////////////////////////////
  // /////////////////////////////////////////////////


  mainTabbed->addTab( videoTab, i18n("Video") );
  mainTabbed->addTab( audioTab, i18n("Audio") );


  mainLayout->addWidget( groupFileInfo, 0, 0 );
  mainLayout->addWidget( mainTabbed, 0, 1 );

  mainLayout->setColStretch( 0, 1 );

}


#include "k3bvcdtrackdialog.moc"
