/***************************************************************************
                          k3bdvdcopy.cpp  -  description
                             -------------------
    begin                : Sun Mar 3 2002
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

#include "k3bdvdcopy.h"
#include "k3bdvdcontent.h"
#include "../tools/k3bexternalbinmanager.h"
#include "../k3b.h"
#include "k3bdvdrippingprocess.h"
#include "../device/k3bdevicemanager.h"
#include "../device/k3bdevice.h"

#include <qstring.h>
#include <qdatastream.h>

#include <kprocess.h>
#include <klocale.h>

K3bDvdCopy::K3bDvdCopy(const QString& device, const QString& directory, const QString& vob, const QString& tmp, const QValueList<K3bDvdContent> &titles, QWidget *parent ) : K3bJob( ) {
    m_device = device;
    m_directory = directory;
    m_dirvob = vob;
    m_dirtmp = tmp;
    m_ripTitles = titles;
    m_parent = parent;
    m_ripProcess = new K3bDvdRippingProcess( m_parent );
}

K3bDvdCopy::~K3bDvdCopy(){
    //delete m_ripProcess;
}

void K3bDvdCopy::start(){
    m_ripProcess->setDvdTitle( m_ripTitles );
    m_ripProcess->setDevice( m_device );
    m_ripProcess->setDirectories( m_directory, m_dirvob, m_dirtmp );
    //m_ripProcess->setJob( m_ripJob );
    connect( m_ripProcess, SIGNAL( interrupted() ), m_parent, SLOT( slotRipJobDeleted() ) );
    connect( m_ripProcess, SIGNAL( finished( bool ) ), this, SLOT( ripFinished( bool ) ) );
    connect( m_ripProcess, SIGNAL( progressPercent( int ) ), this, SLOT( slotPercent( int ) ) );
    m_ripProcess->start();
    emit started();
    emit newTask( i18n("Copy DVD.")  );
    qDebug("(K3bDvdCopy) Starting rip.");
}

void K3bDvdCopy::ripFinished( bool result ){
    qDebug("(K3bDvdCopy) Send finished.");
    emit finished( result );
}

void K3bDvdCopy::cancel( ){
    m_ripProcess->cancel();
}

void K3bDvdCopy::slotPercent( int i ){
    emit percent( i );
}

void K3bDvdCopy::setDvdTitle( const QValueList<K3bDvdContent> &titles ){
    m_ripTitles = titles;
}

#include "k3bdvdcopy.moc"
