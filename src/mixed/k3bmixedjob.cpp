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



K3bMixedJob::K3bMixedJob( K3bMixedDoc* doc, QObject* parent )
  : K3bBurnJob( parent ),
    m_doc( doc )
{
  m_isoImager = new K3bIsoImager( k3bMain()->externalBinManager(), doc->dataDoc(), this );
  connect( m_isoImager, SIGNAL(sizeCalculated(int, int)), this, SLOT(slotSizeCalculationFinished(int, int)) );
  connect( m_isoImager, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
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
  emit infoMessage( "Size calculated: " + QString::number(size), status );
  emit finished( status != ERROR );
}


#include "k3bmixedjob.moc"
