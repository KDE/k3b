/***************************************************************************
                          k3bmixedjob.cpp  -  Job that creates a mixed mode cd
                             -------------------
    begin                : Fri Aug 23 2002
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


#include "k3bmixedjob.h"
#include "k3bmixeddoc.h"

#include "../data/k3bdatadoc.h"
#include "../data/k3bisoimager.h"

#include "../k3b.h"

#include <qfile.h>
#include <qdatastream.h>

#include <kdebug.h>
#include <klocale.h>


K3bMixedJob::K3bMixedJob( K3bMixedDoc* doc, QObject* parent )
  : K3bBurnJob( parent ),
    m_doc( doc )
{
  m_isoImager = new K3bIsoImager( k3bMain()->externalBinManager(), doc->dataDoc(), this );
  connect( m_isoImager, SIGNAL(sizeCalculated(int, int)), this, SLOT(slotSizeCalculationFinished(int, int)) );
  connect( m_isoImager, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
  connect( m_isoImager, SIGNAL(data(char*, int)), this, SLOT(slotReceivedIsoImagerData(char*, int)) );
  connect( m_isoImager, SIGNAL(percent(int)), this, SIGNAL(subPercent(int)) );
  connect( m_isoImager, SIGNAL(finished(bool)), this, SLOT(slotIsoImagerFinished(bool)) );
}


K3bMixedJob::~K3bMixedJob()
{
}


K3bDoc* K3bMixedJob::doc() const
{
  return m_doc;
}


void K3bMixedJob::start()
{
  m_isoImager->calculateSize();
}


void K3bMixedJob::cancel()
{
}


void K3bMixedJob::slotSizeCalculationFinished( int status, int size )
{
  emit infoMessage( i18n("Size calculated: %1 (%2 Bytes)").arg(size).arg(size*2048), status );
  if( status != ERROR ) {
    m_isoImageFile = new QFile( "/home/trueg/tmp/image.iso" );
    m_isoImageFile->open( IO_WriteOnly );
    m_isoImageFileStream = new QDataStream( m_isoImageFile );
    m_isoImager->start();    
  }
  else {
    emit finished(false);
  }
}


void K3bMixedJob::slotReceivedIsoImagerData( char* data, int len )
{
  m_isoImageFileStream->writeRawBytes( data, len );
  m_isoImager->resume();
}


void K3bMixedJob::slotIsoImagerFinished( bool success )
{
  m_isoImageFile->close();
  emit infoMessage( i18n("Size of ISO image: %1").arg(m_isoImageFile->size() ), INFO );
  emit finished( success );
}


#include "k3bmixedjob.moc"
