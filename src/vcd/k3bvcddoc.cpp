/***************************************************************************
                             k3b -  description
                             -------------------
    copyright            : (C) 2002 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
                          k3bvcddoc.cpp  -  description
                             -------------------
    begin                : Mon Nov 4 2002
    copyright            : (C) 2002 by Christian Kvasny
    email                : chris@ckvsoft.at
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "../k3b.h"
#include "../tools/k3bglobals.h"
#include "k3bvcddoc.h"
#include "k3bvcdview.h"
#include "k3bvcdtrack.h"
#include "k3bvcdburndialog.h"
#include "k3bvcdjob.h"
#include "../tools/kstringlistdialog.h"

// QT-includes
#include <qstring.h>
#include <qstringlist.h>
#include <qfile.h>
#include <qdatastream.h>
#include <qdom.h>
#include <qdatetime.h>
#include <qtimer.h>
#include <qtextstream.h>

// KDE-includes
#include <kprocess.h>
#include <kurl.h>
#include <kapplication.h>
#include <kmessagebox.h>
#include <kconfig.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kio/global.h>
#include <kdebug.h>


bool desperate_mode = false;
bool preserve_header = false;
bool print_progress = true;
bool aspect_correction  = false;
byte forced_sequence_header = 0;

K3bVcdDoc::K3bVcdDoc( QObject* parent )
  : K3bDoc( parent )
{
  m_tracks = 0L;
  m_vcdOptions = 0L;
  
  m_docType = VCD;
  m_vcdType = NONE;

  m_urlAddingTimer = new QTimer( this );
  connect( m_urlAddingTimer, SIGNAL(timeout()), this, SLOT(slotWorkUrlQueue()) );
}

K3bVcdDoc::~K3bVcdDoc()
{
  if( m_tracks ) {
    m_tracks->setAutoDelete( true );
    delete m_tracks;
  }

  if ( m_vcdOptions )
    delete m_vcdOptions;

}

bool K3bVcdDoc::newDocument()
{
  if( m_tracks )
    m_tracks->setAutoDelete( true );

  delete m_tracks;

  m_tracks = new QPtrList<K3bVcdTrack>;
  m_tracks->setAutoDelete( false );

  m_vcdOptions = new K3bVcdOptions();

  return K3bDoc::newDocument();
}

unsigned long long K3bVcdDoc::calcTotalSize() const
{
  unsigned long long sum = 0;
  if ( m_tracks ) {
    for ( K3bVcdTrack* track = m_tracks->first(); track; track = m_tracks->next() ) {
      sum += track->size();
    }
  }
  return sum;
}

unsigned long long K3bVcdDoc::size() const
{
  return calcTotalSize();
}

unsigned long long K3bVcdDoc::length() const
{
  return size() / 2352;
}

void K3bVcdDoc::addUrl( const KURL& url )
{
  addTrack( url, m_tracks->count() );
}

void K3bVcdDoc::addUrls( const KURL::List& urls )
{
  addTracks( urls, m_tracks->count() );
}

void K3bVcdDoc::addTracks( const KURL::List& urls, uint position )
{
  for( KURL::List::ConstIterator it = urls.begin(); it != urls.end(); it++ ) {
    urlsToAdd.enqueue( new PrivateUrlToAdd( *it, position++ ) );
  }

  m_urlAddingTimer->start(0);
}

void K3bVcdDoc::slotWorkUrlQueue()
{
  if( !urlsToAdd.isEmpty() ) {
    PrivateUrlToAdd* item = urlsToAdd.dequeue();
    lastAddedPosition = item->position;

    // append at the end by default
    if( lastAddedPosition > m_tracks->count() )
      lastAddedPosition = m_tracks->count();

    if( !item->url.isLocalFile() ) {
      kdDebug() << item->url.path() << " no local file" << endl;
      return;
    }

    if( !QFile::exists( item->url.path() ) ) {
      m_notFoundFiles.append( item->url.path() );
      return;
    }

    if( K3bVcdTrack* newTrack = createTrack( item->url ) )
      addTrack( newTrack, lastAddedPosition );

    delete item;

    emit newTracks();
  }
  else {
    m_urlAddingTimer->stop();

    emit newTracks();

    informAboutNotFoundFiles();
  }
}


K3bVcdTrack* K3bVcdDoc::createTrack( const KURL& url )
{
  char filename[255];
  strcpy(filename,QFile::encodeName(url.path()));
  mpeg* Mpeg = new mpeg(filename,0);

  if ( Mpeg ) {
    int mpegVersion = Mpeg->MpegVersion();
    // no mpeg audio files at this time!!
    if (mpegVersion > 0 && Mpeg->has_video()) {
      if (vcdType() == NONE) {
        m_urlAddingTimer->stop();
        setVcdType(vcdTypes(mpegVersion));
        KMessageBox::information(kapp->mainWidget(),"(" + url.path() + ")\n" +
          i18n("K3b will create a (S)VCD image from the given MPEG files, but these files must already be in (S)VCD format. K3b performs no resample on MPEG files yet. This looks like an MPEG%1 file and K3b set the type to %2.").arg(mpegVersion).arg((mpegVersion==1)?"VCD 2.0": "SVCD"),
          i18n("Set Type to %1 (MPEG%2)").arg((mpegVersion==1)?"VCD 2.0":"SVCD").arg(mpegVersion) );
        m_urlAddingTimer->start(0);
      }
      
      if (vcdType() != vcdTypes(mpegVersion)) {
        KMessageBox::error( kapp->mainWidget(), "(" + url.path() + ")\n" +
          i18n("You can't mix MPEG1 and MPEG2 video files.\nPlease start a new Project for this filetype.\nResample not implemented in K3b yet :("),
          i18n("Wrong File Type for this Project") );

        delete Mpeg;
        return 0;
      }
      K3bVcdTrack* newTrack =  new K3bVcdTrack( m_tracks, url.path() );
      char HMS[30];
      QString mt;
      Mpeg->SecsToHMS(HMS, Mpeg->Video->duration);
      newTrack->setMpegType(Mpeg->MpegType);
      newTrack->setMpegVideoVersion(Mpeg->mpeg_version);

      mt.append(i18n(" MPEG%1").arg(mpegVersion));

      newTrack->setMpegDisplaySize(QString(" %1 x %2").arg(Mpeg->Video->hsize).arg(Mpeg->Video->vsize));
      if (Mpeg->DExt){
        newTrack->setMpegDExt(true);
        newTrack->setMpegFormat(Mpeg->DExt->video_format);
        switch (Mpeg->DExt->video_format) {
          case 0 : mt.append(i18n("  Component")); break;
          case 1 : mt.append("  PAL"); break;
          case 2 : mt.append("  NTSC"); break;
          case 3 : mt.append("  SECAM"); break;
          case 4 : mt.append("  MAC"); break;
          case 5 : mt.append(i18n("  Unspecified")); break;
        }
        if ((Mpeg->DExt->h_display_size != Mpeg->Video->hsize) || (Mpeg->DExt->v_display_size != Mpeg->Video->vsize))
          newTrack->setMpegDisplaySize(QString(" %1 x %2").arg(Mpeg->DExt->h_display_size).arg(Mpeg->DExt->v_display_size));
      }

      newTrack->setMpegSize(QString(" %1 x %2").arg(Mpeg->Video->hsize).arg(Mpeg->Video->vsize));
      newTrack->setMpegFps(QString(" %1").arg(Mpeg->Video->frame_rate));
      newTrack->setMpegMbps(QString(" %1").arg(Mpeg->Video->bitrate/2500.0));

      newTrack->setMpegDuration(HMS);
      newTrack->setMpegVersion(mt);

      newTrack->setMpegAspectRatio(Mpeg->Video->aspect_ratio);

      if (Mpeg->SExt){
        newTrack->setMpegSExt(true);
        newTrack->setMpegProgressive(Mpeg->SExt->progressive);
        newTrack->setMpegChromaFormat(Mpeg->SExt->chroma_format);
      }
      // audio
      if (Mpeg->has_audio()) {
        Mpeg->SecsToHMS(HMS, Mpeg->Audio->duration);
        newTrack->setHasAudio(true);
        newTrack->setMpegAudioCopyright(Mpeg->Audio->copyright);
        newTrack->setMpegAudioOriginal(Mpeg->Audio->original);
        newTrack->setMpegAudioEmphasis(Mpeg->Audio->emphasis_index);
        newTrack->setMpegAudioType(Mpeg->Audio->mpeg_ver);
        newTrack->setMpegAudioLayer(Mpeg->Audio->layer);
        newTrack->setMpegAudioDuration(HMS);
        newTrack->setMpegAudioKbps(QString("%1").arg(Mpeg->Audio->bitrate));
        newTrack->setMpegAudioHz(QString("%1").arg(Mpeg->Audio->sampling_rate));
        newTrack->setMpegAudioFrame(QString("%1").arg(Mpeg->Audio->frame_length));
        newTrack->setMpegAudioMode(Mpeg->Audio->mode);
        newTrack->setMpegAudioModeExt(Mpeg->Audio->modext);
      }

      // for debuging
      Mpeg->PrintInfos();
      delete Mpeg;
      return newTrack;
    }
    else {
      KMessageBox::error( kapp->mainWidget(), "(" + url.path() + ")\n" +
        i18n("Only MPEG1 and MPEG2 video files are supported."),
        i18n("Wrong File Format") );
    }
  }
  return 0;
}


void K3bVcdDoc::addTrack(const KURL& url, uint position )
{
  urlsToAdd.enqueue( new PrivateUrlToAdd( url, position ) );

  m_urlAddingTimer->start(0);
}


void K3bVcdDoc::addTrack( K3bVcdTrack* track, uint position )
{
  if( m_tracks->count() >= 98 ) {
    kdDebug() << "(K3bVcdDoc) VCD Green Book (Red Book) only allows 98 tracks." << endl;
    // TODO: show some messagebox
    delete track;
    return;
  }

  lastAddedPosition = position;

  if( !m_tracks->insert( position, track ) ) {
    lastAddedPosition = m_tracks->count();
    m_tracks->insert( m_tracks->count(), track );
  }

  emit newTracks();

  setModified( true );
}


void K3bVcdDoc::removeTrack( K3bVcdTrack* track )
{
  if( !track ) {
    return;
  }

  // set the current item to track
  if( m_tracks->findRef( track ) >= 0 ) {
    // take the current item
    track = m_tracks->take();

    // emit signal before deleting the track to avoid crashes
    // when the view tries to call some of the tracks' methods
    emit newTracks();

    delete track;
    kdDebug() << QString("(K3bVcdDoc) removeTrack count = %1").arg(numOfTracks()) << endl;
    if (numOfTracks() == 0)
      this->setVcdType(NONE);
  }
}

void K3bVcdDoc::moveTrack( const K3bVcdTrack* track, const K3bVcdTrack* after )
{
  if( track == after )
    return;

  // set the current item to track
  m_tracks->findRef( track );
  // take the current item
  track = m_tracks->take();

  // if after == 0 findRef returnes -1
  int pos = m_tracks->findRef( after );
  m_tracks->insert( pos+1, track );
}


K3bView* K3bVcdDoc::newView( QWidget* parent )
{
  return new K3bVcdView( this, parent );
}


QString K3bVcdDoc::documentType() const
{
  return "k3b_vcd_project";
}

void K3bVcdDoc::addView(K3bView* view)
{
  K3bDoc::addView( view );
}

K3bBurnJob* K3bVcdDoc::newBurnJob()
{
  return new K3bVcdJob( this );
}

void K3bVcdDoc::informAboutNotFoundFiles()
{
  if( !m_notFoundFiles.isEmpty() ) {
    KStringListDialog d( m_notFoundFiles, i18n("Not found"), i18n("Could not find the following files:"),
      true, k3bMain(), "notFoundFilesInfoDialog" );
    d.exec();

    m_notFoundFiles.clear();
  }
}

void K3bVcdDoc::setVcdType( int type )
{
  m_vcdType = type;
}

void K3bVcdDoc::loadDefaultSettings()
{
  KConfig* c = k3bMain()->config();

  c->setGroup( "Videocd settings" );

  setDummy( c->readBoolEntry( "dummy_mode", false ) );
  // setDao( c->readBoolEntry( "dao", true ) );
  setDao( true );
  setOnTheFly( c->readBoolEntry( "on_the_fly", false ) );
  setBurnproof( c->readBoolEntry( "burnproof", true ) );
  setOnlyCreateImage( c->readBoolEntry( "only_create_image", false ) );
  setDeleteImage( c->readBoolEntry( "remove_image", true ) );  

}


bool K3bVcdDoc::loadDocumentData( QDomElement* root )
{
  newDocument();

  QDomNodeList nodes = root->childNodes();

  if( nodes.length() < 3 )
    return false;

  if( nodes.item(0).nodeName() != "general" )
    return false;
  if( !readGeneralDocumentData( nodes.item(0).toElement() ) )
    return false;
    
  if( nodes.item(1).nodeName() != "vcd" )
    return false;

  if( nodes.item(2).nodeName() != "contents" )
    return false;

 
  // vcd Label
  QDomNodeList vcdNodes = nodes.item(1).childNodes();
  vcdOptions()->setAlbumId( vcdNodes.item(0).toElement().text() );
  vcdOptions()->setVolumeId( vcdNodes.item(1).toElement().text() );
  setVcdType( vcdNodes.item(2).toElement().text().toInt() );
  
  // vcd Tracks
  QDomNodeList trackNodes = nodes.item(2).childNodes();

  for( uint i = 0; i< trackNodes.length(); i++ ) {

    // check if url is available
    QDomElement trackElem = trackNodes.item(i).toElement();
    QString url = trackElem.attributeNode( "url" ).value();
    if( !QFile::exists( url ) )
      m_notFoundFiles.append( url );
    else {
      KURL k;
      k.setPath( url );
      if( K3bVcdTrack* track = createTrack( k ) ) {
        QDomNodeList trackNodes = trackElem.childNodes();
        addTrack( track, m_tracks->count() );
      }
    }
  }

  emit newTracks();

  informAboutNotFoundFiles();

  if ( m_notFoundFiles.isEmpty() )
    setModified(false); 

  return true;
}


bool K3bVcdDoc::saveDocumentData( QDomElement* docElem)
{
  QDomDocument doc = docElem->ownerDocument();
  saveGeneralDocumentData( docElem );

  // save Vcd Label
  QDomElement vcdMain = doc.createElement( "vcd" );
  QDomElement vcdElem = doc.createElement( "albumId" );
  vcdElem.appendChild( doc.createTextNode( ( vcdOptions()->albumId() )) );
  vcdMain.appendChild( vcdElem );

  vcdElem = doc.createElement( "volumeId" );
  vcdElem.appendChild( doc.createTextNode( (vcdOptions()->volumeId() )) );
  vcdMain.appendChild( vcdElem );

  vcdElem = doc.createElement( "vcdType" );
  vcdElem.appendChild( doc.createTextNode( (QString("%1").arg(vcdType()) )) );
  vcdMain.appendChild( vcdElem );

  docElem->appendChild( vcdMain );
  
  // save the tracks
  // -------------------------------------------------------------
  QDomElement contentsElem = doc.createElement( "contents" );

  for( K3bVcdTrack* track = first(); track != 0; track = next() ) {

    QDomElement trackElem = doc.createElement( "track" );
    trackElem.setAttribute( "url", KIO::decodeFileName(track->absPath()) );

    contentsElem.appendChild( trackElem );
  }
  // -------------------------------------------------------------

  docElem->appendChild( contentsElem );

  return true;
}


#include "k3bvcddoc.moc"
