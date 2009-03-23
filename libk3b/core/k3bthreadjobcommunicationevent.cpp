/*
 *
 * Copyright (C) 2007-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bthreadjobcommunicationevent.h"

#include <QtCore/QMutex>


K3b::ThreadJobCommunicationEvent::ThreadJobCommunicationEvent( int type )
    : QEvent( QEvent::User ),
      m_type( type ),
      m_device( 0 ),
      m_wantedMediaState( 0 ),
      m_wantedMediaType( 0 ),
      m_result( 0 )
{
}


K3b::ThreadJobCommunicationEvent::~ThreadJobCommunicationEvent()
{
}


int K3b::ThreadJobCommunicationEvent::type() const
{
    return m_type;
}


K3b::Device::Device* K3b::ThreadJobCommunicationEvent::device() const
{
    return m_device;
}


K3b::Device::MediaStates K3b::ThreadJobCommunicationEvent::wantedMediaState() const
{
    return m_wantedMediaState;
}


K3b::Device::MediaTypes K3b::ThreadJobCommunicationEvent::wantedMediaType() const
{
    return m_wantedMediaType;
}


QString K3b::ThreadJobCommunicationEvent::message() const
{
    return m_text;
}


QString K3b::ThreadJobCommunicationEvent::text() const
{
    return m_text;
}


QString K3b::ThreadJobCommunicationEvent::caption() const
{
    return m_caption;
}


QString K3b::ThreadJobCommunicationEvent::yesText() const
{
    return m_yesText;
}


QString K3b::ThreadJobCommunicationEvent::noText() const
{
    return m_noText;
}


int K3b::ThreadJobCommunicationEvent::intResult() const
{
    return m_result;
}


bool K3b::ThreadJobCommunicationEvent::boolResult() const
{
    return ( m_result != 0 );
}


void K3b::ThreadJobCommunicationEvent::wait()
{
    QMutex mutex;
    mutex.lock();
    m_threader.wait( &mutex );
    mutex.unlock();
}


void K3b::ThreadJobCommunicationEvent::done( int result )
{
    m_result = result;
    m_threader.wakeAll();
}


K3b::ThreadJobCommunicationEvent* K3b::ThreadJobCommunicationEvent::waitForMedium( K3b::Device::Device* device,
                                                                                   Device::MediaStates mediaState,
                                                                                   Device::MediaTypes mediaType,
                                                                                   const QString& message )
{
    K3b::ThreadJobCommunicationEvent* event = new K3b::ThreadJobCommunicationEvent( WaitForMedium );
    event->m_device = device;
    event->m_wantedMediaState = mediaState;
    event->m_wantedMediaType = mediaType;
    event->m_text = message;
    return event;
}


K3b::ThreadJobCommunicationEvent* K3b::ThreadJobCommunicationEvent::questionYesNo( const QString& text,
                                                                                   const QString& caption,
                                                                                   const QString& yesText,
                                                                                   const QString& noText )
{
    K3b::ThreadJobCommunicationEvent* event = new K3b::ThreadJobCommunicationEvent( QuestionYesNo );
    event->m_text = text;
    event->m_caption = caption;
    event->m_yesText = yesText;
    event->m_noText = noText;
    return event;
}


K3b::ThreadJobCommunicationEvent* K3b::ThreadJobCommunicationEvent::blockingInformation( const QString& text,
                                                                                         const QString& caption )
{
    K3b::ThreadJobCommunicationEvent* event = new K3b::ThreadJobCommunicationEvent( BlockingInfo );
    event->m_text = text;
    event->m_caption = caption;
    return event;
}
