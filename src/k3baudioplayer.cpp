/***************************************************************************
                          k3baudioplayer.cpp  -  description
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

#include "k3baudioplayer.h"
#include "tools/k3bglobals.h"
#include "kcutlabel.h"

#include <qlabel.h>
#include <qtoolbutton.h>
#include <qlayout.h>
#include <qtimer.h>
#include <qdatetime.h>
#include <qfont.h>
#include <qslider.h>
#include <qlistview.h>
#include <qfile.h>
#include <qpalette.h>
#include <qheader.h>
#include <qevent.h>
#include <qdragobject.h>
#include <qptrlist.h>

#include <kiconloader.h>
#include <klocale.h>
#include <kurl.h>
#include <kurldrag.h>
#include <kaction.h>

#include <string.h>

#include <arts/artsflow.h>
#include <kdebug.h>

using namespace std;

K3bPlayListViewItem::K3bPlayListViewItem( const QString& filename, QListView* parent )
  : KListViewItem( parent ), m_filename( filename )
{
  m_length = 0;
  m_bActive = false;
}


K3bPlayListViewItem::K3bPlayListViewItem( const QString& filename, QListView* parent, QListViewItem* after )
  : KListViewItem( parent, after ), m_filename( filename )
{
  m_length = 0;
  m_bActive = false;
}


K3bPlayListViewItem::~K3bPlayListViewItem()
{
}


QString K3bPlayListViewItem::text( int c ) const
{
  switch( c ) {
  case 0:
    {
      int pos = m_filename.findRev("/");
      if( pos >= 0 )
	return m_filename.mid(pos+1);
      return m_filename;
    }

  case 1:
    if( m_length > 0 )
      return K3b::framesToString( m_length );

  default:
    return "";
  }
}


void K3bPlayListViewItem::paintCell( QPainter* p, const QColorGroup& cg, int c, int w, int a )
{
  if( m_bActive ) {
    // change the color of the text:
    // change roles: Text, HighlightedText, HighLight
    QColorGroup newCg( cg );

    // we assume the user has not configured a very dark color as base color
    newCg.setColor( QColorGroup::Text, red );
    newCg.setColor( QColorGroup::Highlight, red );
    newCg.setColor( QColorGroup::HighlightedText, white );

    KListViewItem::paintCell( p, newCg, c, w, a );
  }
  else
    KListViewItem::paintCell( p, cg, c, w, a );
}


K3bPlayListView::K3bPlayListView( QWidget* parent, const char* name )
  : KListView( parent, name )
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


K3bPlayListView::~K3bPlayListView()
{
}


bool K3bPlayListView::acceptDrag( QDropEvent* e ) const
{
  // we accept textdrag (urls) and moved items (supported by KListView)
  return QUriDrag::canDecode(e) || KListView::acceptDrag(e);
}


QDragObject* K3bPlayListView::dragObject()
{
  QPtrList<QListViewItem> list = selectedItems();

  if( list.isEmpty() )
    return 0;

  QPtrListIterator<QListViewItem> it(list);
  KURL::List urls;

  for( ; it.current(); ++it )
    urls.append( KURL( ((K3bPlayListViewItem*)it.current())->filename() ) );

  return KURLDrag::newDrag( urls, viewport() );
}


K3bAudioPlayer::K3bAudioPlayer( QWidget* parent, const char* name )
  : QWidget( parent, name ), m_playObject( Arts::PlayObject::null() )
{
  // initialize
  // ------------------------------------------------------------------------
  m_labelFilename    = new KCutLabel( i18n("no file"), this );
  m_labelOverallTime = new QLabel( "00:00:00", this );
  m_labelCurrentTime = new QLabel( "00:00:00", this );

  m_viewPlayList = new K3bPlayListView( this );

  m_labelOverallTime->setAlignment( AlignHCenter | AlignVCenter );
  m_labelCurrentTime->setAlignment( AlignHCenter | AlignVCenter );
  m_labelOverallTime->setFrameStyle( QFrame::StyledPanel | QFrame::Plain );
  m_labelCurrentTime->setFrameStyle( QFrame::StyledPanel | QFrame::Plain );
  m_labelFilename->setFrameStyle( QFrame::StyledPanel | QFrame::Plain );
  m_labelOverallTime->setPalette( QPalette( QColor(238, 238, 205) ) );
  m_labelCurrentTime->setPalette( QPalette( QColor(238, 238, 205) ) );
  m_labelFilename->setPalette( QPalette( QColor(238, 238, 205) ) );

  m_buttonPlay = new QToolButton( this );
  m_buttonPause = new QToolButton( this );
  m_buttonStop = new QToolButton( this );
  m_buttonPlay->setIconSet( SmallIconSet("1rightarrow") );
  m_buttonPause->setIconSet( SmallIconSet("player_pause") );
  m_buttonStop->setIconSet( SmallIconSet("player_stop") );
  m_buttonForward = new QToolButton( this );
  m_buttonBack = new QToolButton( this );
  m_buttonForward->setIconSet( SmallIconSet("player_end") );
  m_buttonBack->setIconSet( SmallIconSet("player_start") );

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
  grid->setColStretch( 6, 1 );
  // ------------------------------------------------------------------------


  // actions
  // ------------------------------------------------------------------------
  m_actionRemove = new KAction( i18n( "Remove" ), "editdelete",
				Key_Delete, this, SLOT(slotRemoveSelected()), this );
  m_actionClear = new KAction( i18n( "Clear list" ), "editclear",
			       0, this, SLOT(clear()), this );

  m_contextMenu = new KActionMenu( this );
  m_contextMenu->insert(m_actionRemove);
  m_contextMenu->insert(m_actionClear);
  // ------------------------------------------------------------------------


  // connections
  // ------------------------------------------------------------------------
  connect( m_viewPlayList, SIGNAL(contextMenu(KListView*, QListViewItem*, const QPoint&)),
	   this, SLOT(slotShowContextMenu(KListView*, QListViewItem*, const QPoint&)) );

  connect( m_buttonPlay, SIGNAL(clicked()), this, SLOT(play()) );
  connect( m_buttonStop, SIGNAL(clicked()), this, SLOT(stop()) );
  connect( m_buttonPause, SIGNAL(clicked()), this, SLOT(pause()) );

  connect( m_buttonForward, SIGNAL(clicked()), this, SLOT(forward()) );
  connect( m_buttonBack, SIGNAL(clicked()), this, SLOT(back()) );

  connect( m_seekSlider, SIGNAL(valueChanged(int)), this, SLOT(seek(int)) );
  connect( m_seekSlider, SIGNAL(valueChanged(int)), this, SLOT(slotUpdateCurrentTime(int)) );

  connect( m_updateTimer, SIGNAL(timeout()), this, SLOT(slotUpdateDisplay()) );
  connect( m_updateTimer, SIGNAL(timeout()), this, SLOT(slotCheckEnd()) );

  connect( m_viewPlayList, SIGNAL(doubleClicked(QListViewItem*)),
	   this, SLOT(slotPlayItem(QListViewItem*)) );
  connect( m_viewPlayList, SIGNAL(dropped(QDropEvent*,QListViewItem*)),
	   this, SLOT(slotDropped(QDropEvent*,QListViewItem*)) );
  // ------------------------------------------------------------------------


  m_bLengthReady = false;
}


K3bAudioPlayer::~K3bAudioPlayer()
{
  // we remove the reference to the play object
  // if we don't do this it won't be removed and K3b will crash (not sure why)
  m_playObject = Arts::PlayObject::null();
}


int K3bAudioPlayer::state()
{
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

  return EMPTY;
}


void K3bAudioPlayer::playFile( const QString& filename )
{
  clear();
  if( QFile::exists( filename ) ) {
    K3bPlayListViewItem* item = new K3bPlayListViewItem( filename, m_viewPlayList );
    setCurrentItem( item );
    play();
    emit started( filename );
  }
}


void K3bAudioPlayer::playFiles( const QStringList& files )
{
  clear();
  QStringList::ConstIterator it = files.begin();
  playFile( *it );
  ++it;

  for( ; it != files.end(); ++it )
    enqueueFile( *it );
}


void K3bAudioPlayer::enqueueFile( const QString& filename )
{
  if( QFile::exists( filename ) )
    (void)new K3bPlayListViewItem( filename, m_viewPlayList, m_viewPlayList->lastChild() );
}


void K3bAudioPlayer::enqueueFiles( const QStringList& files )
{
  for( QStringList::ConstIterator it = files.begin(); it != files.end(); ++it )
    enqueueFile( *it );
}


void K3bAudioPlayer::play()
{
  if( !m_currentItem ) {
    setCurrentItem( m_viewPlayList->firstChild() );
  }

  if( m_currentItem ) {
    if( m_playObject.isNull() ) {
      Arts::PlayObjectFactory factory = Arts::Reference("global:Arts_PlayObjectFactory");
      if( factory.isNull() ) {
	kdDebug() << "(K3bAudioPlayer) could not create PlayObjectFactory. Possibly no artsd running." << endl;
	m_labelFilename->setText( i18n("No running aRtsd found") );
	return;
      }

      m_playObject = factory.createPlayObject( string(QFile::encodeName(m_currentItem->filename()) ) );
      if( m_playObject.isNull() ) {
	kdDebug() << "(K3bAudioPlayer) no aRts module available for: " << m_currentItem->filename() << endl;
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
}


void K3bAudioPlayer::slotPlayItem( QListViewItem* item )
{
  setCurrentItem( item );
  play();
}


void K3bAudioPlayer::stop()
{
  if( !m_playObject.isNull() ) {
    m_updateTimer->stop();
    m_playObject.halt();
    m_playObject = Arts::PlayObject::null();
    m_bLengthReady = false;

    slotUpdateFilename();
    m_seekSlider->setValue(0);
    emit stopped();
  }
}


void K3bAudioPlayer::pause()
{
  if( !m_playObject.isNull() ) {
    if( m_playObject.state() == Arts::posPlaying ) {
      m_updateTimer->stop();
      m_playObject.pause();
      emit paused();
    }

    slotUpdateFilename();
  }
}


void K3bAudioPlayer::seek( long pos )
{
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
}



void K3bAudioPlayer::seek( int pos )
{
  seek( (long)pos );
}


void K3bAudioPlayer::forward()
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


void K3bAudioPlayer::back()
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


void K3bAudioPlayer::clear()
{
  setCurrentItem( 0 );
  m_viewPlayList->clear();
}


long K3bAudioPlayer::length()
{
  if( !m_playObject.isNull() ) {
    return m_playObject.overallTime().seconds;
  }
  return 0;
}


long K3bAudioPlayer::position()
{
  if( !m_playObject.isNull() ) {
    return m_playObject.currentTime().seconds;
  }
  return 0;
}


// FIXME: let my do some useful stuff!
bool K3bAudioPlayer::supportsMimetype( const QString& mimetype )
{
  if( mimetype.contains("audio") || mimetype.contains("ogg") )
    return true;
  else
    return false;
}


void K3bAudioPlayer::slotCheckEnd()
{
  if( !m_playObject.isNull() ) {
    if( m_playObject.state() == Arts::posIdle ) {
      if( m_currentItem->nextSibling() ) {
	setCurrentItem( m_currentItem->nextSibling() );
	play();
      }
      else {
	m_updateTimer->stop();
	seek(0);
      }
      emit ended();
    }
  }
}


void K3bAudioPlayer::setCurrentItem( QListViewItem* item )
{
  if( item == 0 ) {
    stop();
    m_labelOverallTime->setText("00:00:00");
    m_labelFilename->setText( i18n("no file") );
    m_currentItem = 0;
  }
  else if( K3bPlayListViewItem* playItem = dynamic_cast<K3bPlayListViewItem*>(item) ) {
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


void K3bAudioPlayer::slotUpdateCurrentTime( int time )
{
  m_labelCurrentTime->setText( K3b::framesToString( time*75 ) );
}


void K3bAudioPlayer::slotUpdateLength( long time )
{
  m_labelOverallTime->setText( K3b::framesToString( time*75 ) );
}


void K3bAudioPlayer::slotUpdateFilename()
{
  if( m_currentItem ) {
    QString display = m_currentItem->filename();
    int pos = display.findRev("/");
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


void K3bAudioPlayer::slotUpdateDisplay()
{
  if( m_currentItem ) {
    // we need to set the length here because sometimes it is not ready in the beginning (??)
    if( !m_bLengthReady && length() > 0 ) {
      slotUpdateLength( length() );
      m_seekSlider->setMaxValue( length() );
      m_currentItem->setLength( 75 * length() );
      m_bLengthReady = true;

      m_viewPlayList->viewport()->update();
    }

    // we need to disconnect here to avoid recursive value setting
    m_seekSlider->disconnect( this, SLOT(seek(int)) );
    m_seekSlider->setValue( position() );
    connect( m_seekSlider, SIGNAL(valueChanged(int)), this, SLOT(seek(int)) );
  }
}


void K3bAudioPlayer::slotDropped( QDropEvent* e, QListViewItem* after )
{
  if( !after )
    after = m_viewPlayList->lastChild();

  KURL::List urls;
  KURLDrag::decode( e, urls );

  for( KURL::List::ConstIterator it = urls.begin(); it != urls.end(); ++it ) {
    if( QFile::exists( (*it).path() ) ) {
      QListViewItem* newItem = new K3bPlayListViewItem( (*it).path(), m_viewPlayList, after );
      after = newItem;
    }
  }
}


void K3bAudioPlayer::slotRemoveSelected()
{
  QPtrList<QListViewItem> selected = m_viewPlayList->selectedItems();
  for( QListViewItem* item = selected.first(); item; item = selected.next() ) {
    if( item == m_currentItem )
      setCurrentItem(0);
    delete item;
  }
}


void K3bAudioPlayer::slotShowContextMenu( KListView*lstView, QListViewItem* item, const QPoint& p )
{
  if( item )
    m_actionRemove->setEnabled( true );
  else
    m_actionRemove->setEnabled( false );

  m_contextMenu->popup(p);
}


#include "k3baudioplayer.moc"
