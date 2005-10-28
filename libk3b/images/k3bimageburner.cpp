/* 
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bimageburner.h"
#include "k3bimagesource.h"


class K3bImageBurner::Private
{
public:
  Private()
    : canceled(false) {
  }

  bool canceled;
};


K3bImageBurner::K3bImageBurner( K3bJobHandler* hdl, QObject* parent )
  : K3bBurnJob( hdl, parent ),
    K3bImageSink(),
    m_burnDevice(0),
    m_burnSpeed(0),
    m_writingMode( K3b::WRITING_MODE_AUTO ),
    m_writingApp( K3b::DEFAULT ),
    m_simulate( false ),
    m_closeMedium( true )
{
  d = new Private();
}


K3bImageBurner::~K3bImageBurner()
{
  delete d;
}


void K3bImageBurner::start()
{
  jobStarted();
  d->canceled = false;

  // connect source
  source()->disconnect( this );
  connect( source(), SIGNAL(tocReady(bool)), SLOT(slotTocReady(bool)) );
  connect( source(), SIGNAL(infoMessage(const QString&, int)), SIGNAL(infoMessage(const QString&, int)) );

  // FIXME: emit some newTask
  emit newTask( "Doing stuff with the toc (I am a temp message, REPLACEME)" );

  // we start by determining the toc
  source()->determineToc();
}


void K3bImageBurner::cancel()
{
  if( active() ) {
    d->canceled = true;

    // first we cancel the source
    source()->cancel();

    emit canceled();

    // now cancel the job for real
    cancelInternal();
  }
}


void K3bImageBurner::slotTocReady( bool success )
{
  if( success ) {
    // start the burning already!
    startInternal();
  }
  else if( !canceled() ) {
    // FIXME: should the source emit a proper error message or we?
    jobFinished( false );
  }
}


bool K3bImageBurner::canceled() const
{
  return d->canceled;
}

#include "k3bimageburner.moc"
