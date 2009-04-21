/*
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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


#include "k3baudioplayer.h"
#include "k3bmsf.h"

#include <qlabel.h>
#include <qtoolbutton.h>
#include <qlayout.h>
#include <qtimer.h>
#include <qdatetime.h>
#include <qfont.h>
#include <qslider.h>
#include <q3listview.h>
#include <qfile.h>
#include <qpalette.h>
#include <q3header.h>
#include <qevent.h>
#include <q3dragobject.h>
#include <QList>
//Added by qt3to4:
#include <QDropEvent>
#include <QGridLayout>
#include <QFrame>
#include <k3urldrag.h>

#include <kiconloader.h>
#include <klocale.h>
#include <kurl.h>
#include <kaction.h>
#include <ksqueezedtextlabel.h>

#include <string.h>

#ifdef WITH_ARTS
#include <arts/artsflow.h>
#endif

#include <kdebug.h>

using namespace std;

K3b::PlayListViewItem::PlayListViewItem( const QString& filename, Q3ListView* parent )
  : K3ListViewItem( parent ), m_filename( filename )
{
  m_length = 0;
  m_bActive = false;
}


K3b::PlayListViewItem::PlayListViewItem( const QString& filename, Q3ListView* parent, Q3ListViewItem* after )
  : K3ListViewItem( parent, after ), m_filename( filename )
{
  m_length = 0;
  m_bActive = false;
}


K3b::PlayListViewItem::~PlayListViewItem()
{
}


QString K3b::PlayListViewItem::text( int c ) const
{
  switch( c ) {
  case 0:
    {
      int pos = m_filename.lastIndexOf('/');
      if( pos >= 0 )
	return m_filename.mid(pos+1);
      return m_filename;
    }

  case 1:
    if( m_length > 0 )
      return K3b::Msf(m_length).toString(false);

  default:
    return "";
  }
}


void K3b::PlayListViewItem::paintCell( QPainter* p, const QColorGroup& cg, int c, int w, int a )
{
  if( m_bActive ) {
    // change the color of the text:
    // change roles: Text, HighlightedText, HighLight
    QColorGroup newCg( cg );

    // we assume the user has not configured a very dark color as base color
    newCg.setColor( QColorGroup::Text, Qt::red );
    newCg.setColor( QColorGroup::Highlight, Qt::red );
    newCg.setColor( QColorGroup::HighlightedText, Qt::white );

    K3ListViewItem::paintCell( p, newCg, c, w, a );
  }
  else
    K3ListViewItem::paintCell( p, cg, c, w, a );
}


K3b::PlayListView::PlayListView( QWidget* parent )
  : K3ListView( parent )
{
  addColumn( i18n("Filename") );
  addColumn( i18n("Length") );
  setAllColumnsShowFocus( true );
  setAcceptDrops( true );
  setDropVisualizer( true );
  setDragEnabled(true);
  setItemsMovable( true );
  header()->setClickEnabled( false );
  setSorting( -1 );
}


K3b::PlayListView::~PlayListView()
{
}


bool K3b::PlayListView::acceptDrag( QDropEvent* e ) const
{
  // we accept textdrag (urls) and moved items (supported by K3ListView)
  return K3URLDrag::canDecode(e) || K3ListView::acceptDrag(e);
}


Q3DragObject* K3b::PlayListView::dragObject()
{
  QList<Q3ListViewItem*> list = selectedItems();

  if( list.isEmpty() )
    return 0;

  QListIterator<Q3ListViewItem*> it(list);
  KUrl::List urls;

  for( ; it.current(); ++it )
    urls.append( KUrl( ((K3b::PlayListViewItem*)it.current())->filename() ) );

  return K3URLDrag::newDrag( urls, viewport() );
}


K3b::AudioPlayer::AudioPlayer( QWidget* parent )
  : QWidget( parent )
#ifdef WITH_ARTS
, m_playObject( Arts::PlayObject::null() )
#endif
{
    m_currentItem = 0L;
  // initialize
  // ------------------------------------------------------------------------
  m_labelFilename    = new KSqueezedTextLabelLabel( i18n("no file"), this );
  m_labelFilename->setTextElideMode( Qt::ElideRight );
  m_labelOverallTime = new QLabel( "00:00", this );
  m_labelCurrentTime = new QLabel( "00:00", this );

  m_viewPlayList = new K3b::PlayListView( this );

  m_labelOverallTime->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
  m_labelCurrentTime->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
  m_labelOverallTime->setFrameStyle( QFrame::StyledPanel | QFrame::Plain );
  m_labelCurrentTime->setFrameStyle( QFrame::StyledPanel | QFrame::Plain );
  m_labelFilename->setFrameStyle( QFrame::StyledPanel | QFrame::Plain );
  m_labelOverallTime->setPalette( QPalette( QColor(238, 238, 205) ) );
  m_labelCurrentTime->setPalette( QPalette( QColor(238, 238, 205) ) );
  m_labelFilename->setPalette( QPalette( QColor(238, 238, 205) ) );

  m_buttonPlay = new QToolButton( this );
  m_buttonPause = new QToolButton( this );
  m_buttonStop = new QToolButton( this );
  m_buttonPlay->setIconSet( SmallIconSet("media-playback-start") );
  m_buttonPause->setIconSet( SmallIconSet("media-playback-pause") );
  m_buttonStop->setIconSet( SmallIconSet("media-playback-stop") );
  m_buttonForward = new QToolButton( this );
  m_buttonBack = new QToolButton( this );
  m_buttonForward->setIconSet( SmallIconSet("media-skip-forward") );
  m_buttonBack->setIconSet( SmallIconSet("media-skip-backward") );

  m_seekSlider = new QSlider( QSlider::Horizontal, this );

  m_updateTimer = new QTimer( this );
  // ------------------------------------------------------------------------

  // layout
  // ------------------------------------------------------------------------
  QGridLayout* grid = new QGridLayout( this );
  grid->setSpacing( 2 );
  grid->setMargin( 0 );

  grid->addWidget( m_buttonPlay, 1, 0 );
  grid->addWidget( m_buttonPause, 1, 1 );
  grid->addWidget( m_buttonStop, 1, 2 );
  grid->addColSpacing( 3, 5 );
  grid->addWidget( m_buttonBack, 1, 4 );
  grid->addWidget( m_buttonForward, 1, 5 );

  grid->addMultiCellWidget( m_labelFilename, 0, 0, 0, 6 );

  grid->addMultiCellWidget( m_seekSlider, 1, 1, 6, 8 );

  grid->addWidget( m_labelCurrentTime, 0, 7 );
  grid->addWidget( m_labelOverallTime, 0, 8 );

  grid->addMultiCellWidget( m_viewPlayList, 2, 2, 0, 8 );
  grid->setRowStretch( 2, 1 );
  grid->setColumnStretch( 6, 1 );
  // ------------------------------------------------------------------------


  // actions
  // ------------------------------------------------------------------------
  m_actionRemove = new KAction( i18n( "Remove" ), "editdelete",
				Qt::Key_Delete, this, SLOT(slotRemoveSelected()),
				this, "audioplayer_remove" );
  m_actionClear = new KAction( i18n( "Clear List" ), "editclear",
			       0, this, SLOT(clear()),
			       this, "audioplayer_clear" );

  m_contextMenu = new KActionMenu( this, "audio_player_menu" );
  m_contextMenu->insert(m_actionRemove);
  m_contextMenu->insert(m_actionClear);
  // ------------------------------------------------------------------------


  // connections
  // ------------------------------------------------------------------------
  connect( m_viewPlayList, SIGNAL(contextMenu(K3ListView*, Q3ListViewItem*, const QPoint&)),
	   this, SLOT(slotShowContextMenu(K3ListView*, Q3ListViewItem*, const QPoint&)) );

  connect( m_buttonPlay, SIGNAL(clicked()), this, SLOT(play()) );
  connect( m_buttonStop, SIGNAL(clicked()), this, SLOT(stop()) );
  connect( m_buttonPause, SIGNAL(clicked()), this, SLOT(pause()) );

  connect( m_buttonForward, SIGNAL(clicked()), this, SLOT(forward()) );
  connect( m_buttonBack, SIGNAL(clicked()), this, SLOT(back()) );

  connect( m_seekSlider, SIGNAL(sliderMoved(int)), this, SLOT(seek(int)) );
  connect( m_seekSlider, SIGNAL(valueChanged(int)), this, SLOT(slotUpdateCurrentTime(int)) );

  connect( m_updateTimer, SIGNAL(timeout()), this, SLOT(slotUpdateDisplay()) );
  connect( m_updateTimer, SIGNAL(timeout()), this, SLOT(slotCheckEnd()) );

  connect( m_viewPlayList, SIGNAL(doubleClicked(Q3ListViewItem*)),
	   this, SLOT(slotPlayItem(Q3ListViewItem*)) );
  connect( m_viewPlayList, SIGNAL(dropped(QDropEvent*,Q3ListViewItem*)),
	   this, SLOT(slotDropped(QDropEvent*,Q3ListViewItem*)) );
  // ------------------------------------------------------------------------


  m_bLengthReady = false;
}


K3b::AudioPlayer::~AudioPlayer()
{
  // we remove the reference to the play object
  // if we don't do this it won't be removed and K3b will crash (not sure why)
#ifdef WITH_ARTS
  m_playObject = Arts::PlayObject::null();
#endif
}


int K3b::AudioPlayer::state()
{
#ifdef WITH_ARTS
  if( !m_playObject.isNull() ) {
    switch( m_playObject.state() ) {
    case Arts::posIdle:
      return STOPPED;
    case Arts::posPlaying:
      return PLAYING;
    case Arts::posPaused:
      return PAUSED;
    }
  }
  else if( m_currentItem )
    return STOPPED;
#endif

  return EMPTY;
}


void K3b::AudioPlayer::playFile( const QString& filename )
{
  clear();
  if( QFile::exists( filename ) ) {
    K3b::PlayListViewItem* item = new K3b::PlayListViewItem( filename, m_viewPlayList );
    setCurrentItem( item );
    play();
    emit started( filename );
  }
}


void K3b::AudioPlayer::playFiles( const QStringList& files )
{
  clear();
  QStringList::ConstIterator it = files.begin();
  playFile( *it );
  ++it;

  for( ; it != files.end(); ++it )
    enqueueFile( *it );
}


void K3b::AudioPlayer::enqueueFile( const QString& filename )
{
  if( QFile::exists( filename ) )
    (void)new K3b::PlayListViewItem( filename, m_viewPlayList, m_viewPlayList->lastChild() );
}


void K3b::AudioPlayer::enqueueFiles( const QStringList& files )
{
  for( QStringList::ConstIterator it = files.begin(); it != files.end(); ++it )
    enqueueFile( *it );
}


void K3b::AudioPlayer::play()
{
#ifdef WITH_ARTS
  if( !m_currentItem ) {
    setCurrentItem( m_viewPlayList->firstChild() );
  }

  if( m_currentItem ) {
    if( m_playObject.isNull() ) {
      Arts::PlayObjectFactory factory = Arts::Reference("global:Arts_PlayObjectFactory");
      if( factory.isNull() ) {
	kDebug() << "(K3b::AudioPlayer) could not create PlayObjectFactory. Possibly no artsd running.";
	m_labelFilename->setText( i18n("No running aRtsd found") );
	return;
      }

      m_playObject = factory.createPlayObject( string(QFile::encodeName(m_currentItem->filename()) ) );
      if( m_playObject.isNull() ) {
	kDebug() << "(K3b::AudioPlayer) no aRts module available for: " << m_currentItem->filename();
	m_labelFilename->setText( i18n("Unknown file format") );

	// play the next if there is any
	if( m_currentItem->itemBelow() ) {
	  setCurrentItem( m_currentItem->itemBelow() );
	  play();
	}
	return;
      }
    }
    if( m_playObject.state() != Arts::posPlaying ) {
      m_playObject.play();
      emit started();
      m_updateTimer->start( 1000 );
    }

    slotUpdateFilename();
  }
#endif
}


void K3b::AudioPlayer::slotPlayItem( Q3ListViewItem* item )
{
  setCurrentItem( item );
  play();
}


void K3b::AudioPlayer::stop()
{
#ifdef WITH_ARTS
  if( !m_playObject.isNull() ) {
    m_updateTimer->stop();
    m_playObject.halt();
    m_playObject = Arts::PlayObject::null();
    m_bLengthReady = false;

    emit stopped();
  }
#endif

  m_seekSlider->setValue(0);
  slotUpdateFilename();
  slotUpdateCurrentTime(0);
}


void K3b::AudioPlayer::pause()
{
#ifdef WITH_ARTS
  if( !m_playObject.isNull() ) {
    if( m_playObject.state() == Arts::posPlaying ) {
      m_updateTimer->stop();
      m_playObject.pause();
      emit paused();
    }

    slotUpdateFilename();
  }
#endif
}


void K3b::AudioPlayer::seek( long pos )
{
#ifdef WITH_ARTS
  if( !m_playObject.isNull() ) {
    if( m_playObject.state() != Arts::posIdle ) {
      if( pos < 0 ) {
	m_playObject.seek( Arts::poTime() );
      }
      else if( m_playObject.overallTime().seconds < pos ) {
	m_playObject.seek( m_playObject.overallTime() );
      }
      else if( pos != m_playObject.currentTime().seconds ) {
	m_playObject.seek( Arts::poTime( pos, 0, -1, "" ) );
      }
    }
  }
  else {
    m_seekSlider->setValue(0);
    slotUpdateCurrentTime(0);
  }
#endif
}



void K3b::AudioPlayer::seek( int pos )
{
  seek( (long)pos );
}


void K3b::AudioPlayer::forward()
{
  if( m_currentItem ) {
    if( m_currentItem->itemBelow() ) {
      bool bPlay = false;
      if( state() == PLAYING )
	bPlay = true;

      setCurrentItem( m_currentItem->itemBelow() );

      if( bPlay )
	play();
    }
  }
}


void K3b::AudioPlayer::back()
{
  if( m_currentItem ) {
    if( m_currentItem->itemAbove() ) {
      bool bPlay = false;
      if( state() == PLAYING )
	bPlay = true;

      setCurrentItem( m_currentItem->itemAbove() );

      if( bPlay )
	play();
    }
  }
}


void K3b::AudioPlayer::clear()
{
  setCurrentItem( 0 );
  m_viewPlayList->clear();
}


long K3b::AudioPlayer::length()
{
#ifdef WITH_ARTS
  if( !m_playObject.isNull() ) {
    return m_playObject.overallTime().seconds;
  }
#endif
  return 0;
}


long K3b::AudioPlayer::position()
{
#ifdef WITH_ARTS
  if( !m_playObject.isNull() ) {
    return m_playObject.currentTime().seconds;
  }
#endif
  return 0;
}


// FIXME: let my do some useful stuff!
bool K3b::AudioPlayer::supportsMimetype( const QString& mimetype )
{
  if( mimetype.contains("audio") || mimetype.contains("ogg") )
    return true;
  else
    return false;
}


void K3b::AudioPlayer::slotCheckEnd()
{
#ifdef WITH_ARTS
  if( !m_playObject.isNull() ) {
    if( m_playObject.state() == Arts::posIdle ) {
      if( m_currentItem->nextSibling() ) {
	setCurrentItem( m_currentItem->nextSibling() );
	play();
      }
      else {
	stop();
      }
      emit ended();
    }
  }
#endif
}


void K3b::AudioPlayer::setCurrentItem( Q3ListViewItem* item )
{
  if( item == 0 ) {
    stop();
    m_labelOverallTime->setText("00:00");
    m_labelFilename->setText( i18n("no file") );
    m_currentItem = 0;
  }
  else if( K3b::PlayListViewItem* playItem = dynamic_cast<K3b::PlayListViewItem*>(item) ) {
    if( m_currentItem ) {
      // reset m_currentItem
      m_currentItem->setActive( false );
      stop();
    }
    m_currentItem = playItem;
    m_currentItem->setActive( true );

    // paint the activity changes
    m_viewPlayList->viewport()->update();

    slotUpdateFilename();
  }
}


void K3b::AudioPlayer::slotUpdateCurrentTime( int time )
{
  m_labelCurrentTime->setText( K3b::Msf( time*75 ).toString(false) );
}


void K3b::AudioPlayer::slotUpdateLength( long time )
{
  m_labelOverallTime->setText( K3b::Msf( time*75 ).toString(false) );
}


void K3b::AudioPlayer::slotUpdateFilename()
{
  if( m_currentItem ) {
    QString display = m_currentItem->filename();
    int pos = display.lastIndexOf('/');
    if( pos >= 0 )
      display = display.mid(pos+1);

    switch( state() ) {
    case PLAYING:
      display.prepend( QString("(%1) ").arg(i18n("playing")) );
      break;
    case PAUSED:
      display.prepend( QString("(%1) ").arg(i18n("paused")) );
      break;
    case STOPPED:
      display.prepend( QString("(%1) ").arg(i18n("stopped")) );
      break;
    default:
      break;
    }

    m_labelFilename->setText( display );
  }
}


void K3b::AudioPlayer::slotUpdateDisplay()
{
  if( m_currentItem ) {
    // we need to set the length here because sometimes it is not ready in the beginning (?)
    if( !m_bLengthReady && length() > 0 ) {
      slotUpdateLength( length() );
      m_seekSlider->setMaxValue( length() );
      m_currentItem->setLength( 75 * length() );
      m_bLengthReady = true;

      m_viewPlayList->viewport()->update();
    }

    m_seekSlider->setValue( position() );
  }
}


void K3b::AudioPlayer::slotDropped( QDropEvent* e, Q3ListViewItem* after )
{
  if( !after )
    after = m_viewPlayList->lastChild();

  KUrl::List urls;
  K3URLDrag::decode( e, urls );

  for( KUrl::List::ConstIterator it = urls.begin(); it != urls.end(); ++it ) {
    if( QFile::exists( (*it).toLocalFile() ) ) {
      Q3ListViewItem* newItem = new K3b::PlayListViewItem( (*it).toLocalFile(), m_viewPlayList, after );
      after = newItem;
    }
  }
}


void K3b::AudioPlayer::slotRemoveSelected()
{
  QList<Q3ListViewItem*> selected = m_viewPlayList->selectedItems();
  for( Q3ListViewItem* item = selected.first(); item; item = selected.next() ) {
    if( item == m_currentItem )
      setCurrentItem(0);
    delete item;
  }
}


void K3b::AudioPlayer::slotShowContextMenu( K3ListView*, Q3ListViewItem* item, const QPoint& p )
{
  if( item )
    m_actionRemove->setEnabled( true );
  else
    m_actionRemove->setEnabled( false );

  m_contextMenu->popup(p);
}


#include "k3baudioplayer.moc"
