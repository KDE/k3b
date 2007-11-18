/*
 *
 * $Id: k3bprogressinfoevent.h 619556 2007-01-03 17:38:12Z trueg $
 * Copyright (C) 2007 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bthreadjobcommunicationevent.h"

#include <QtCore/QMutex>


K3bThreadJobCommunicationEvent::K3bThreadJobCommunicationEvent( int type )
    : QEvent( QEvent::User ),
      m_type( type ),
      m_device( 0 ),
      m_wantedMediaState( 0 ),
      m_wantedMediaType( 0 ),
      m_result( 0 )
{
}


K3bThreadJobCommunicationEvent::~K3bThreadJobCommunicationEvent()
{
}


int K3bThreadJobCommunicationEvent::type() const
{
    return m_type;
}


K3bDevice::Device* K3bThreadJobCommunicationEvent::device() const
{
    return m_device;
}


int K3bThreadJobCommunicationEvent::wantedMediaState() const
{
    return m_wantedMediaState;
}


int K3bThreadJobCommunicationEvent::wantedMediaType() const
{
    return m_wantedMediaType;
}


QString K3bThreadJobCommunicationEvent::message() const
{
    return m_text;
}


QString K3bThreadJobCommunicationEvent::text() const
{
    return m_text;
}


QString K3bThreadJobCommunicationEvent::caption() const
{
    return m_caption;
}


QString K3bThreadJobCommunicationEvent::yesText() const
{
    return m_yesText;
}


QString K3bThreadJobCommunicationEvent::noText() const
{
    return m_noText;
}


int K3bThreadJobCommunicationEvent::intResult() const
{
    return m_result;
}


bool K3bThreadJobCommunicationEvent::boolResult() const
{
    return ( m_result != 0 );
}


void K3bThreadJobCommunicationEvent::wait()
{
    QMutex mutex;
    mutex.lock();
    m_threader.wait( &mutex );
    mutex.unlock();
}


void K3bThreadJobCommunicationEvent::done( int result )
{
    m_result = result;
    m_threader.wakeAll();
}


K3bThreadJobCommunicationEvent* K3bThreadJobCommunicationEvent::waitForMedium( K3bDevice::Device* device,
                                                                               int mediaState,
                                                                               int mediaType,
                                                                               const QString& message )
{
    K3bThreadJobCommunicationEvent* event = new K3bThreadJobCommunicationEvent( WaitForMedium );
    event->m_device = device;
    event->m_wantedMediaState = mediaState;
    event->m_wantedMediaType = mediaType;
    event->m_text = message;
    return event;
}


K3bThreadJobCommunicationEvent* K3bThreadJobCommunicationEvent::questionYesNo( const QString& text,
                                                                               const QString& caption,
                                                                               const QString& yesText,
                                                                               const QString& noText )
{
    K3bThreadJobCommunicationEvent* event = new K3bThreadJobCommunicationEvent( QuestionYesNo );
    event->m_text = text;
    event->m_caption = caption;
    event->m_yesText = yesText;
    event->m_noText = noText;
    return event;
}


K3bThreadJobCommunicationEvent* K3bThreadJobCommunicationEvent::blockingInformation( const QString& text,
                                                                                     const QString& caption )
{
    K3bThreadJobCommunicationEvent* event = new K3bThreadJobCommunicationEvent( BlockingInfo );
    event->m_text = text;
    event->m_caption = caption;
    return event;
}
