/* 
 *
 * $Id$
 * Copyright (C) 2003 Thomas Froescher <tfroescher@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bdvdcopy.h"
#include "k3bdvdcontent.h"
#include <k3bexternalbinmanager.h>
#include "k3bdvdrippingprocess.h"
#include <k3bdevicemanager.h>
#include <k3bdevice.h>

#include <qstring.h>
#include <qdatastream.h>
#include <qdatetime.h>
#include <qwidget.h>
#include <kprocess.h>
#include <klocale.h>

K3bDvdCopy::K3bDvdCopy( K3bJobHandler* hdl, const QString& device, const QString& directory, const QString& vob, const QString& tmp, const QValueList<K3bDvdContent> &titles, QObject *parent )
  : K3bJob( hdl, parent ) {
    m_device = device;
    m_directory = directory;
    m_dirvob = vob;
    m_dirtmp = tmp;
    m_ripTitles = titles;
    //    m_parent = parent;
    m_successfulStarted = true;
    m_ripProcess = new K3bDvdRippingProcess( this, this );
}

K3bDvdCopy::~K3bDvdCopy(){
    //delete m_ripProcess;
}

void K3bDvdCopy::start(){
    m_timeEstimated = QTime::currentTime();
    m_timeDataRate = QTime::currentTime();
    m_ripProcess->setDvdTitle( m_ripTitles );
    m_ripProcess->setDevice( m_device );
    m_ripProcess->setDirectories( m_directory, m_dirvob, m_dirtmp );
    m_ripProcess->setRipSize( m_ripSize );
    m_ripProcess->setAngle( m_angle );
    //m_ripProcess->setJob( m_ripJob );
    //    connect( m_ripProcess, SIGNAL( canceled() ), m_parent, SLOT( slotRipJobDeleted() ) );
    connect( m_ripProcess, SIGNAL( infoMessage(const QString&, int) ), this, SIGNAL( infoMessage(const QString&, int) ) );
    connect( m_ripProcess, SIGNAL( newSubTask(const QString&) ), this, SIGNAL( newSubTask(const QString&) ) );
    connect( m_ripProcess, SIGNAL( finished( bool ) ), this, SLOT( ripFinished( bool ) ) );
    connect( m_ripProcess, SIGNAL( percent( int ) ), this, SLOT( slotPercent( int ) ) );
    connect( m_ripProcess, SIGNAL( rippedBytesPerPercent( unsigned long ) ), this, SLOT( slotDataRate( unsigned long ) ) );

    m_ripProcess->start();
    if( m_successfulStarted ) {
        emit started();
        emit newTask( i18n("Ripping Video DVD")  );
        kdDebug() << "(K3bDvdCopy) Starting rip." << endl;
    } else {
        kdDebug() << "(K3bDvdCopy) Start ripping failed." << endl;
    }
}

void K3bDvdCopy::ripFinished( bool result ){
    m_successfulStarted = result; // if start failed it returns immedatitely finsihed with false;
    m_preProcessingFailed = m_ripProcess->isInitFailed();
    kdDebug() << "(K3bDvdCopy) Send finished. Status: " << result << ", Initstatus: " << m_preProcessingFailed << endl;
    emit finished( result );
}

void K3bDvdCopy::cancel( ){
    m_ripProcess->cancel();
}

void K3bDvdCopy::slotPercent( unsigned int i ){
    QTime current = QTime::currentTime();
    int s = m_timeEstimated.msecsTo( current );
    m_timeEstimated = current;
    unsigned int diff = (unsigned int) (( 100 - i ) * s/1000);
    emit estimatedTime( diff );
    emit percent( (int) i );
}

void K3bDvdCopy::slotDataRate( unsigned long l){
    QTime current = QTime::currentTime();
    int s = m_timeDataRate.msecsTo( current );
    m_timeDataRate = current;
    float diff = (float) ( (float) l / s *1000 )/1024/1024;
    emit dataRate( diff );
}

void K3bDvdCopy::setDvdTitle( const QValueList<K3bDvdContent> &titles ){
    m_ripTitles = titles;
}
void K3bDvdCopy::setSettings( double size, const QString& angle ){
    m_angle = angle;
    m_ripSize = size;
}

QString K3bDvdCopy::jobDescription() const
{
  return i18n("Ripping Video DVD");
}

QString K3bDvdCopy::jobDetails() const
{
  return i18n("1 title", "%n titles", m_ripTitles.count());  
}

#include "k3bdvdcopy.moc"
