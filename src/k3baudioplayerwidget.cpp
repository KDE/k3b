/***************************************************************************
                          k3baudioplayerwidget.cpp  -  description
                             -------------------
    begin                : Mon Feb 11 2002
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

#include "k3baudioplayerwidget.h"
#include "k3baudioplayer.h"

#include <qlabel.h>
#include <qtoolbutton.h>
#include <qlayout.h>
#include <qtimer.h>
#include <qdatetime.h>
#include <qfont.h>
#include <qslider.h>

#include <kiconloader.h>
#include <klocale.h>

// #include <vector>
// #include <map>
// #include <klistview.h>




K3bAudioPlayerWidget::K3bAudioPlayerWidget( K3bAudioPlayer* player, bool skipButtons, QWidget *parent, const char *name )
  : QWidget( parent, name ), m_bLengthReady( false )
{
  m_player = player;

  // initialize
  // ------------------------------------------------------------------------
  m_labelFilename    = new QLabel( i18n("no file"), this );
  m_labelOverallTime = new QLabel( "00:00:00", this );
  m_labelCurrentTime = new QLabel( "00:00:00", this );

  m_labelOverallTime->setAlignment( AlignHCenter | AlignVCenter );
  m_labelCurrentTime->setAlignment( AlignHCenter | AlignVCenter );
  m_labelOverallTime->setFrameStyle( QFrame::StyledPanel | QFrame::Plain );
  m_labelCurrentTime->setFrameStyle( QFrame::StyledPanel | QFrame::Plain );
  m_labelFilename->setFrameStyle( QFrame::StyledPanel | QFrame::Plain );
  m_labelOverallTime->setPalette( QPalette( QColor(238, 238, 205) ) );
  m_labelCurrentTime->setPalette( QPalette( QColor(238, 238, 205) ) );
  m_labelFilename->setPalette( QPalette( QColor(238, 238, 205) ) );
  QFont font = m_labelOverallTime->font();
  font.setPointSize( 8 );
  m_labelOverallTime->setFont( font );
  m_labelCurrentTime->setFont( font );
  m_labelFilename->setFont( font );

  m_buttonPlay = new QToolButton( this );
  m_buttonPause = new QToolButton( this );
  m_buttonStop = new QToolButton( this );
  m_buttonPlay->setIconSet( SmallIconSet("1rightarrow") );
  m_buttonPause->setIconSet( SmallIconSet("player_pause") );
  m_buttonStop->setIconSet( SmallIconSet("player_stop") );



  if( skipButtons ) {
    m_buttonForward = new QToolButton( this );
    m_buttonBack = new QToolButton( this );
    m_buttonForward->setIconSet( SmallIconSet("player_start") );
    m_buttonBack->setIconSet( SmallIconSet("player_end") );
  }

  m_seekSlider = new QSlider( QSlider::Horizontal, this );

  m_displayTimer = new QTimer( this );
  // ------------------------------------------------------------------------


  // layout
  // ------------------------------------------------------------------------
  QGridLayout* grid = new QGridLayout( this );
  grid->setSpacing( 2 );
  grid->setMargin( 2 );

  grid->addWidget( m_buttonPlay, 1, 0 );
  grid->addWidget( m_buttonPause, 1, 1 );
  grid->addWidget( m_buttonStop, 1, 2 );
  if(skipButtons) {
    grid->addColSpacing( 3, 5 );
    grid->addWidget( m_buttonBack, 1, 4 );
    grid->addWidget( m_buttonForward, 1, 5 );
  }

  grid->addMultiCellWidget( m_labelFilename, 0, 0, 0, 6 );

  grid->addWidget( m_seekSlider, 1, 6 );

  grid->addWidget( m_labelOverallTime, 0, 7 );
  grid->addWidget( m_labelCurrentTime, 1, 7 );

  grid->setRowStretch( 2, 1 );
  // ------------------------------------------------------------------------


//         KListView *listView = new KListView(this);
// 	grid->addMultiCellWidget( listView, 3, 3, 0, 7 );
//         listView->addColumn(i18n("Media Type"));

//         Arts::TraderQuery q;
//         std::vector<Arts::TraderOffer> *results = q.query();
//         std::map<std::string, bool> done;
//         QString str;

//         for(std::vector<Arts::TraderOffer>::iterator i = results->begin(); i != results->end(); i++)
//         {
//                 std::vector<string> *ext = (*i).getProperty("Extension");

//                 for(vector<string>::iterator it = ext->begin(); it != ext->end(); it++)
//                 {
//                         if(!(*it).length() || done[*it])
//                             continue;

//                         done[*it] = true;
//                         (void) new QListViewItem(listView, (*it).c_str());
//                 }
//                 delete ext;
//         }
//         delete results;





  // connections
  // ------------------------------------------------------------------------
  connect( m_buttonPlay, SIGNAL(clicked()), m_player, SLOT(play()) );
  connect( m_buttonStop, SIGNAL(clicked()), m_player, SLOT(stop()) );
  connect( m_buttonPause, SIGNAL(clicked()), m_player, SLOT(pause()) );

  if( skipButtons ) {
    connect( m_buttonForward, SIGNAL(clicked()), this, SIGNAL(forward()) );
    connect( m_buttonBack, SIGNAL(clicked()), this, SIGNAL(back()) );
  }

  connect( m_player, SIGNAL(started()), this, SLOT(slotStarted()) );
  connect( m_player, SIGNAL(started(const QString&)), this, SLOT(slotStarted(const QString&)) );
  connect( m_player, SIGNAL(stopped()), this, SLOT(slotStopped()) );
  connect( m_player, SIGNAL(ended()), this, SLOT(slotEnded()) );
  connect( m_player, SIGNAL(paused()), this, SLOT(slotPaused()) );

  connect( m_seekSlider, SIGNAL(valueChanged(int)), m_player, SLOT(seek(int)) );
  connect( m_seekSlider, SIGNAL(valueChanged(int)), this, SLOT(slotUpdateCurrentTime(int)) );

  connect( m_displayTimer, SIGNAL(timeout()), this, SLOT(slotUpdateSlider()) );
  // ------------------------------------------------------------------------

  init();
}


K3bAudioPlayerWidget::~K3bAudioPlayerWidget()
{
}


void K3bAudioPlayerWidget::init()
{
  if( m_player ) {
    if( m_player->state() != K3bAudioPlayer::EMPTY ) {
      slotUpdateFilename( m_player->filename() );
      slotUpdateSlider();
    }
    switch( m_player->state() ) {
    case K3bAudioPlayer::PLAYING:
      slotStarted();
      break;
    case K3bAudioPlayer::PAUSED:
      slotPaused();
      break;
    case K3bAudioPlayer::STOPPED:
      slotStopped();
      break;
    default:
      m_buttonPlay->setDisabled( true );
      m_buttonPause->setDisabled( true );
      m_buttonStop->setDisabled( true );
      break;
    }
  }
  else {
    qDebug("(K3bAudioPlayerWidget) player is NULL" );
  }
}


void K3bAudioPlayerWidget::slotStarted()
{
  m_labelFilename->setText( i18n("%1 (playing)").arg(m_filename) );

  m_buttonPlay->setDisabled( true );
  m_buttonPause->setEnabled( true );
  m_buttonStop->setEnabled( true );

  m_displayTimer->start( 1000 );
}


void K3bAudioPlayerWidget::slotStarted( const QString& filename )
{
  slotUpdateFilename( filename );
  m_bLengthReady = false;
  slotUpdateSlider();
  slotStarted();
}


void K3bAudioPlayerWidget::slotPaused()
{
  m_labelFilename->setText( i18n("%1 (paused)").arg(m_filename) );

  m_buttonPlay->setEnabled( true );
  m_buttonPause->setDisabled( true );
  m_buttonStop->setEnabled( true );
  
  m_displayTimer->stop();
}


void K3bAudioPlayerWidget::slotStopped()
{
  m_labelFilename->setText( i18n("%1 (stopped)").arg(m_filename) );

  m_buttonPlay->setEnabled( true );
  m_buttonPause->setDisabled( true );
  m_buttonStop->setDisabled( true );
  
  // we need to disconnect here to avoid recursive value setting
  m_seekSlider->disconnect(m_player);
  m_seekSlider->setValue( 0 );
  connect( m_seekSlider, SIGNAL(valueChanged(int)), m_player, SLOT(seek(int)) );
      
  m_displayTimer->stop();
}


void K3bAudioPlayerWidget::slotEnded()
{
  slotStopped();
}


void K3bAudioPlayerWidget::slotUpdateCurrentTime( int time )
{
  m_labelCurrentTime->setText( QTime().addSecs(time).toString() );
}


void K3bAudioPlayerWidget::slotUpdateLength( long time )
{
  m_labelOverallTime->setText( QTime().addSecs(time).toString() );
}


void K3bAudioPlayerWidget::slotUpdateFilename( const QString& filename )
{
  int pos = filename.findRev("/");
  if( pos < 0 ) // no slash
    m_filename = filename;
  else
    m_filename = filename.mid(pos+1);
}


void K3bAudioPlayerWidget::slotUpdateSlider()
{
  if( m_player ) {
    // we need to set the length here because sometimes it is not ready in the beginning (??)
    if( !m_bLengthReady && m_player->length() > 0 ) {
      slotUpdateLength( m_player->length() );
      m_seekSlider->setMaxValue( m_player->length() );
      m_bLengthReady = true;
    }

    // we need to disconnect here to avoid recursive value setting
    m_seekSlider->disconnect(m_player);
    m_seekSlider->setValue( m_player->position() );
    connect( m_seekSlider, SIGNAL(valueChanged(int)), m_player, SLOT(seek(int)) );
  }
  else {
    qDebug("(K3bAudioPlayerWidget) player is NULL" );
  }
}


#include "k3baudioplayerwidget.moc"
