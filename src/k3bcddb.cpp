/***************************************************************************
                          k3bcddb.cpp  -  description
                             -------------------
    begin                : Sun Oct 7 2001
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

#include <qstring.h>
#include <qvaluelist.h>
#include <qstringlist.h>
#include <qmessagebox.h>

#include <klocale.h>
#include <kconfig.h>

#include "k3bcddb.h"
#include "cddb.h"
#include "k3b.h"

#include "device/k3btoc.h"
#include "device/k3btrack.h"
#include "k3bcddbmultientriesdialog.h"



typedef Q_INT16 size16;
typedef Q_INT32 size32;

/* This is in support for the Mega Hack, if cdparanoia ever is fixed, or we
 * use another ripping library we can remove this.  */
#include <linux/cdrom.h>
#include <sys/ioctl.h>

extern "C" {
#include <cdda_interface.h>
#include <cdda_paranoia.h>
}
#define MAX_IPC_SIZE (1024*32)
#define DEFAULT_CDDB_SERVER "localhost:888"

extern "C" {
    int FixupTOC( cdrom_drive * d, int tracks );
}
int start_of_first_data_as_in_toc;
int hack_track;

/* Mega hack.  This function comes from libcdda_interface, and is called by
 * it.  We need to override it, so we implement it ourself in the hope, that
 * shared lib semantics make the calls in libcdda_interface to FixupTOC end
 * up here, instead of it's own copy.  This usually works.
 * You don't want to know the reason for this.  */
/*
int FixupTOC( cdrom_drive * d, int tracks )
{
    int j;
    for( j = 0; j < tracks; j++ ) {
        if( d->disc_toc[j].dwStartSector < 0 )
            d->disc_toc[j].dwStartSector = 0;
        if( j < tracks - 1 && d->disc_toc[j].dwStartSector > d->disc_toc[j + 1].dwStartSector )
            d->disc_toc[j].dwStartSector = 0;
    }
    long last = d->disc_toc[0].dwStartSector;
    for( j = 1; j < tracks; j++ ) {
        if( d->disc_toc[j].dwStartSector < last )
            d->disc_toc[j].dwStartSector = last;
    }
    start_of_first_data_as_in_toc = -1;
    hack_track = -1;
    if( d->ioctl_fd != -1 ) {
        struct cdrom_multisession ms_str;
        ms_str.addr_format = CDROM_LBA;
        if( ioctl( d->ioctl_fd, CDROMMULTISESSION, &ms_str ) == -1 )
            return -1;
        if( ms_str.addr.lba > 100 ) {
            for( j = tracks - 1; j >= 0; j-- )
                if( j > 0 && !IS_AUDIO( d, j ) && IS_AUDIO( d, j - 1 ) ) {
                    if( d->disc_toc[j].dwStartSector > ms_str.addr.lba - 11400 ) {
*/
                        /* The next two code lines are the purpose of duplicating this
                         * function, all others are an exact copy of paranoias FixupTOC().
                         * The gory details: CD-Extra consist of N audio-tracks in the
                         * first session and one data-track in the next session.  This
                         * means, the first sector of the data track is not right behind
                         * the last sector of the last audio track, so all length
                         * calculation for that last audio track would be wrong.  For this
                         * the start sector of the data track is adjusted (we don't need
                         * the real start sector, as we don't rip that track anyway), so
                         * that the last audio track end in the first session.  All well
                         * and good so far.  BUT: The CDDB disc-id is based on the real
                         * TOC entries so this adjustment would result in a wrong Disc-ID.
                         * We can only solve this conflict, when we save the old
                         * (toc-based) start sector of the data track.  Of course the
                         * correct solution would be, to only adjust the _length_ of the
                         * last audio track, not the start of the next track, but the
                         * internal structures of cdparanoia are as they are, so the
                         * length is only implicitely given.  Bloody sh*.  */

/*                        start_of_first_data_as_in_toc = d->disc_toc[j].dwStartSector;
                        hack_track = j + 1;
                        d->disc_toc[j].dwStartSector = ms_str.addr.lba - 11400;
                    }
                    break;
                }
            return 1;
        }
    }
    return 0;
}
*/
/* libcdda returns for cdda_disc_lastsector() the last sector of the last
 * _audio_ track.  How broken.  For CDDB Disc-ID we need the real last sector
 * to calculate the disc length.  */
long my_last_sector( cdrom_drive * drive )
{
    return cdda_track_lastsector( drive, drive->tracks );
}


K3bCddb::K3bCddb( ) : QObject(0, "cddb") {
    m_useCddb = false;
}

K3bCddb::K3bCddb( bool useCddb, QString server, unsigned int port )
{
    m_useCddb = useCddb;
    if (useCddb){
      m_cddb = new CDDB();
      m_cddbServer = server;
      m_cddbPort = port;
    }
}

K3bCddb::~K3bCddb(  )
{
	if (m_useCddb)
		delete m_cddb;
}

void K3bCddb::setUseCddb(bool useCddb){
    m_useCddb = useCddb;
    if (useCddb){
      m_cddb = new CDDB();
    }
}

unsigned int K3bCddb::get_discid( struct cdrom_drive *drive )
{
    unsigned int id = 0;
    for( int i = 1; i <= drive->tracks; i++ ) {
        unsigned int n = cdda_track_firstsector( drive, i ) + 150;
        if( i == hack_track )
            n = start_of_first_data_as_in_toc + 150;
        n /= 75;
        while( n > 0 ) {
            id += n % 10;
            n /= 10;
        }
    }
    unsigned int l = ( my_last_sector( drive ) );
    l -= cdda_disc_firstsector( drive );
    l /= 75;
    id = ( ( id % 255 ) << 24 ) | ( l << 8 ) | drive->tracks;
    return id;
}

void K3bCddb::updateCD( struct cdrom_drive *drive )
{
    m_discid = get_discid( drive );
    m_tracks = cdda_tracks( drive );
    m_cd_album = i18n( "No Title" );
    m_titles.clear(  );
    m_drive = drive;

    if( m_useCddb ) {
        m_cddb->set_server( m_cddbServer.latin1(  ), m_cddbPort );
        QStringList errorEntries;
        if( m_cddb->queryCD( (*getTrackList( )), errorEntries ) ) {
            readQuery();
            return;
        } else {
            QStringList::Iterator it;
            for( it = errorEntries.begin(); it != errorEntries.end(); ++it ){
                qDebug("(K3bCddb) multiple (wrong) CDDB entry: " + (*it) );
            }
            K3bCddbMultiEntriesDialog *dialog = new K3bCddbMultiEntriesDialog( errorEntries );
            connect( dialog, SIGNAL( chosenId( unsigned int )), this, SLOT( prepareQuery( unsigned int ) ) );
            connect( dialog, SIGNAL( cancelClicked( )), this, SLOT( queryTracks(  ) ) );
            dialog->show();
        }
    } else {
        queryTracks();
    }

}

bool K3bCddb::appendCddbInfo( K3bToc& toc )
{
  if( toc.isEmpty() )
    return false;


  // get the default cddb server
  // The option-dialog is able to store more than one server
  // so why don't we use them?
  // ------------------------------------------------
  KConfig *c = kapp->config();
  c->setGroup("Cddb");
  QString hostString = c->readEntry("cddbServer", "");

  if( hostString.isEmpty() )
    return false;

  int index = hostString.find(":");
  QString server = hostString.left(index);
  unsigned int port = hostString.right(hostString.length()-index-1).toUInt();


  // create the strange list of integers that cddb needs
  // we relly need to get rid of that class!!!
  // ------------------------------------------------
  QValueList<int> qvl;
  
  int i = 0;
  for( K3bTrack* track = toc.first(); track != 0; track = toc.next() ) {
    if( i + 1 != hack_track )
      qvl.append( track->firstSector() + 150 );
    else
      qvl.append( start_of_first_data_as_in_toc + 150 );
    i++;
  }
  qvl.append( toc.firstSector() );
  qvl.append( toc.lastSector() );



  // try to retrieve disc-info via cddb
  // ------------------------------------------------
  bool success;
  CDDB* cddb = new CDDB();
  cddb->set_server( server, port );
  QStringList errorList;
  if( cddb->queryCD( qvl, errorList ) ) {
    toc.setAlbum( cddb->title() );
    toc.setArtist( cddb->artist() );

    for( unsigned int i = 0; i < toc.count(); i++ ) {
      toc.at(i)->setTitle( cddb->track( i ) );
    }

    success = true;
  }
  else
    success = false;

  delete cddb;



  return success;
}

QValueList<int>* K3bCddb::getTrackList( ) {
    QValueList < int > *qvl = new QValueList<int>();
    for( int i = 0; i < m_tracks; i++ ) {
        m_is_audio[i] = cdda_track_audiop( m_drive, i + 1 );
        if( i + 1 != hack_track )
            qvl->append( cdda_track_firstsector( m_drive, i + 1 ) + 150 );
        else
            qvl->append( start_of_first_data_as_in_toc + 150 );
    }
    qvl->append( cdda_disc_firstsector( m_drive ) );
    qvl->append( my_last_sector( m_drive ) );
    return qvl;
}

void K3bCddb::readQuery(){
    m_titles.clear(  );
    //m_based_on_cddb = true;
    m_cd_album = m_cddb->title(  );
    m_cd_artist = m_cddb->artist(  );
    for( int i = 0; i < m_tracks; i++ ) {
        m_titles.append( m_cddb->track( i ) );
    }
    // titlelist updated
    emit updatedCD();
}
// ------------------------  Slots  -------------------------
void K3bCddb::prepareQuery( int unsigned cddbId ){
    if( m_cddb->queryCD( *getTrackList(), cddbId )){
        readQuery();
    } else {
        QMessageBox::critical( 0, i18n("CDDB Error"), i18n("Can't read CDDB data."), i18n("Ok") );
        queryTracks();
    }
}
void K3bCddb::queryTracks(){
    m_titles.clear(  );
    //m_based_on_cddb = false;
    m_cd_album = "";
    m_cd_artist = "";
    m_s_track = "";
    for( int i = 0; i < m_tracks; i++ ) {
        QString num;
        int ti = i + 1;
        QString s;
        num.sprintf( "Track %02d", ti );
        if( cdda_track_audiop( m_drive, ti ) )
            s = num.stripWhiteSpace(); //s_track.arg( num );
        else
            s.sprintf( "data%02d", ti );
        qDebug("Track:" + s + " end");
        m_titles.append( s );
    }
    emit updatedCD();
}

#include "k3bcddb.moc"

