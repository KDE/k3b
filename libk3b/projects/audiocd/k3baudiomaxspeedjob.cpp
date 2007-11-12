/* 
 *
 * $Id$
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
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

#include "k3baudiomaxspeedjob.h"
#include "k3baudiotrack.h"
#include "k3baudiodatasource.h"
#include "k3baudiodoc.h"
#include "k3baudiocdtracksource.h"
#include "k3baudiodatasourceiterator.h"

#include <k3bdevice.h>

#include <k3bthread.h>

#include <kdebug.h>
#include <klocale.h>

#include <qdatetime.h>
//Added by qt3to4:
#include <Q3ValueList>


class K3bAudioMaxSpeedJob::WorkThread : public K3bThread
{
public:
  WorkThread( K3bAudioDoc* doc );
  ~WorkThread();

  void run();

  int speedTest( K3bAudioDataSource* source );
  void cancel();
  int maxSpeedByMedia() const;

  int maxSpeed;

private:
  K3bAudioDoc* m_doc;
  bool m_canceled;
  char* m_buffer;
};


K3bAudioMaxSpeedJob::WorkThread::WorkThread( K3bAudioDoc* doc )
  : K3bThread(),
    m_doc(doc),
    m_canceled(false)
{
  m_buffer = new char[2352*10];
}


K3bAudioMaxSpeedJob::WorkThread::~WorkThread()
{
  delete [] m_buffer;
}


void K3bAudioMaxSpeedJob::WorkThread::run()
{
  kDebug() << k_funcinfo << endl;
  m_canceled = false;

  emitStarted();

  K3bAudioDataSourceIterator it( m_doc );

  // count sources for minimal progress info
  int numSources = 0;
  int sourcesDone = 0;
  while( it.current() ) {
    ++numSources;
    it.next();
  }

  bool success = true;
  maxSpeed = 175*1000;
  it.first();

  while( it.current() && !m_canceled ) {
    if( !it.current()->seek(0) ) {
      kDebug() << "(K3bAudioMaxSpeedJob) seek failed." << endl;
      success = false;
      break;
    }
      
    // read some data
    int speed = speedTest( it.current() );

    ++sourcesDone;
    emitPercent( 100*numSources/sourcesDone );

    if( speed < 0 ) {
      success = false;
      break;
    }
    else if( speed > 0 ) {
      // update the max speed
      maxSpeed = qMin( maxSpeed, speed );
    }
      
    it.next();
  }

  if( m_canceled ) {
    emitCanceled();
    success = false;
  }

  if( success )
    kDebug() << "(K3bAudioMaxSpeedJob) max speed: " << maxSpeed << endl;

  emitFinished( success );
}

// returns the amount of data read from this source
int K3bAudioMaxSpeedJob::WorkThread::speedTest( K3bAudioDataSource* source )
{
  //
  // in case of an audio track source we only test when the cd is inserted since asking the user would
  // confuse him a lot.
  //
  // FIXME: there is still the problem of the spin up time.
  //
  if( K3bAudioCdTrackSource* cdts = dynamic_cast<K3bAudioCdTrackSource*>( source ) ) {
    if( K3bDevice::Device* dev = cdts->searchForAudioCD() ) {
      cdts->setDevice( dev );
    }
    else {
      kDebug() << "(K3bAudioMaxSpeedJob) ignoring audio cd track source." << endl;
      return 0;
    }
  }

  QTime t;
  int dataRead = 0;
  int r = 0;

  // start the timer
  t.start();

  // read ten seconds of audio data. This is some value which seemed about right. :)
  while( dataRead < 2352*75*10 && (r = source->read( m_buffer, 2352*10 )) > 0 ) {
    dataRead += r;
  }

  // elapsed millisec
  int usedT = t.elapsed();

  if( r < 0 ) {
    kDebug() << "(K3bAudioMaxSpeedJob) read failure." << endl;
    return -1;
  }

  // KB/sec (add 1 millisecond to avoid division by 0)
  int throughput = (dataRead*1000+usedT)/(usedT+1)/1024;
  kDebug() << "(K3bAudioMaxSpeedJob) throughput: " << throughput 
	    << " (" << dataRead << "/" << usedT << ")" << endl;


  return throughput;
}


void K3bAudioMaxSpeedJob::WorkThread::cancel()
{
  kDebug() << k_funcinfo << endl;
  m_canceled = true;
}


int K3bAudioMaxSpeedJob::WorkThread::maxSpeedByMedia() const
{
  int s = 0;
    
  Q3ValueList<int> speeds = m_doc->burner()->determineSupportedWriteSpeeds();
  // simply use what we have and let the writer decide if the speeds are empty
  if( !speeds.isEmpty() ) {
    // start with the highest speed and go down the list until we are below our max
    Q3ValueListIterator<int> it = speeds.end();
    --it;
    while( *it > maxSpeed && it != speeds.begin() )
      --it;
      
    // this is the first valid speed or the lowest supported one
    s = *it;
    kDebug() << "(K3bAudioMaxSpeedJob) using speed factor: " << (s/175) << endl;
  }

  return s;
}




K3bAudioMaxSpeedJob::K3bAudioMaxSpeedJob( K3bAudioDoc* doc, K3bJobHandler* jh, QObject* parent, const char* name )
  : K3bThreadJob( jh, parent, name )
{
  m_thread = new WorkThread( doc );
  setThread( m_thread );
}


K3bAudioMaxSpeedJob::~K3bAudioMaxSpeedJob()
{
  delete m_thread;
}


int K3bAudioMaxSpeedJob::maxSpeed() const
{
  return m_thread->maxSpeedByMedia();
}
#include "k3baudiomaxspeedjob.moc"
