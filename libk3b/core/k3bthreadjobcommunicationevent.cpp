/*
    SPDX-FileCopyrightText: 2007-2010 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2010 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bthreadjobcommunicationevent.h"

#include <QMutex>

K3b::ThreadJobCommunicationEvent::Data::Data()
    : m_device( 0 ),
      m_wantedMediaState( 0 ),
      m_wantedMediaType( 0 ),
      m_result( 0 )
{
}




K3b::Device::Device* K3b::ThreadJobCommunicationEvent::Data::device() const
{
    return m_device;
}


K3b::Device::MediaStates K3b::ThreadJobCommunicationEvent::Data::wantedMediaState() const
{
    return m_wantedMediaState;
}


K3b::Device::MediaTypes K3b::ThreadJobCommunicationEvent::Data::wantedMediaType() const
{
    return m_wantedMediaType;
}


K3b::Msf K3b::ThreadJobCommunicationEvent::Data::wantedMediaSize() const
{
    return m_wantedMediaSize;
}


QString K3b::ThreadJobCommunicationEvent::Data::message() const
{
    return m_text;
}


QString K3b::ThreadJobCommunicationEvent::Data::text() const
{
    return m_text;
}


QString K3b::ThreadJobCommunicationEvent::Data::caption() const
{
    return m_caption;
}


const KGuiItem& K3b::ThreadJobCommunicationEvent::Data::buttonYes() const
{
    return m_buttonYes;
}


const KGuiItem& K3b::ThreadJobCommunicationEvent::Data::buttonNo() const
{
    return m_buttonNo;
}


int K3b::ThreadJobCommunicationEvent::Data::intResult() const
{
    return m_result;
}


bool K3b::ThreadJobCommunicationEvent::Data::boolResult() const
{
    return ( m_result != 0 );
}


void K3b::ThreadJobCommunicationEvent::Data::wait()
{
    QMutex mutex;
    mutex.lock();
    m_threader.wait( &mutex );
    mutex.unlock();
}


void K3b::ThreadJobCommunicationEvent::Data::done( int result )
{
    m_result = result;
    m_threader.wakeAll();
}


K3b::ThreadJobCommunicationEvent::ThreadJobCommunicationEvent( int type )
    : QEvent( QEvent::User ),
      m_type( type ),
      m_data( new Data() )
{
}


K3b::ThreadJobCommunicationEvent::~ThreadJobCommunicationEvent()
{
    // Do NOT delete m_data here. It is needed after destruction by K3b::ThreadJob
}


int K3b::ThreadJobCommunicationEvent::type() const
{
    return m_type;
}


K3b::ThreadJobCommunicationEvent* K3b::ThreadJobCommunicationEvent::waitForMedium( K3b::Device::Device* device,
                                                                                   Device::MediaStates mediaState,
                                                                                   Device::MediaTypes mediaType,
                                                                                   const K3b::Msf& minMediaSize,
                                                                                   const QString& message )
{
    K3b::ThreadJobCommunicationEvent* event = new K3b::ThreadJobCommunicationEvent( WaitForMedium );
    event->m_data->m_device = device;
    event->m_data->m_wantedMediaState = mediaState;
    event->m_data->m_wantedMediaType = mediaType;
    event->m_data->m_wantedMediaSize = minMediaSize;
    event->m_data->m_text = message;
    return event;
}


K3b::ThreadJobCommunicationEvent* K3b::ThreadJobCommunicationEvent::questionYesNo( const QString& text,
                                                                                   const QString& caption,
                                                                                   const KGuiItem& buttonYes,
                                                                                   const KGuiItem& buttonNo )
{
    K3b::ThreadJobCommunicationEvent* event = new K3b::ThreadJobCommunicationEvent( QuestionYesNo );
    event->m_data->m_text = text;
    event->m_data->m_caption = caption;
    event->m_data->m_buttonYes = buttonYes;
    event->m_data->m_buttonNo = buttonNo;
    return event;
}


K3b::ThreadJobCommunicationEvent* K3b::ThreadJobCommunicationEvent::blockingInformation( const QString& text,
                                                                                         const QString& caption )
{
    K3b::ThreadJobCommunicationEvent* event = new K3b::ThreadJobCommunicationEvent( BlockingInfo );
    event->m_data->m_text = text;
    event->m_data->m_caption = caption;
    return event;
}
