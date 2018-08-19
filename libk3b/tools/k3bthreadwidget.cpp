/*
 *
 * Copyright (C) 2005-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bthreadwidget.h"
#include "k3bdeviceselectiondialog.h"
#include "k3bdevice.h"

#include <QCoreApplication>
#include <QEvent>
#include <QMutex>
#include <QWaitCondition>


class K3b::ThreadWidget::Data
{
public:
    int id;
    void* data;
    QWaitCondition con;
};


class K3b::ThreadWidget::DeviceSelectionEvent : public QEvent
{
public:
    DeviceSelectionEvent( QWidget* parent, const QString& text, int id )
        : QEvent( QEvent::User ),
          m_parent(parent),
          m_text(text),
          m_id(id) {
    }

    QWidget* parent() const { return m_parent; }
    const QString& text() const { return m_text; }
    int id() const { return m_id; }

private:
    QWidget* m_parent;
    QString m_text;
    int m_id;
};


K3b::ThreadWidget* K3b::ThreadWidget::s_instance = 0;


K3b::ThreadWidget::ThreadWidget()
    : QObject(),
      m_idCounter(1)
{
    s_instance = this;
}


K3b::ThreadWidget::~ThreadWidget()
{
    qDeleteAll( m_dataMap );
    s_instance = 0;
}


int K3b::ThreadWidget::getNewId()
{
    // create new data
    Data* data = new Data;
    data->id = m_idCounter++;
    data->data = 0;
    m_dataMap.insert( data->id, data );
    return data->id;
}


void K3b::ThreadWidget::clearId( int id )
{
    m_dataMap.remove( id );
}


K3b::ThreadWidget::Data* K3b::ThreadWidget::data( int id )
{
    return m_dataMap[id];
}


K3b::ThreadWidget* K3b::ThreadWidget::instance()
{
    if( !s_instance )
        s_instance = new K3b::ThreadWidget();
    return s_instance;
}


// static
K3b::Device::Device* K3b::ThreadWidget::selectDevice( QWidget* parent,
                                                  const QString& text )
{
    // request a new data set
    Data* data = K3b::ThreadWidget::instance()->data( K3b::ThreadWidget::instance()->getNewId() );

    // inform the instance about the request
    QCoreApplication::postEvent( K3b::ThreadWidget::instance(),
                             new K3b::ThreadWidget::DeviceSelectionEvent( parent, text, data->id ) );

    // wait for the result to be ready
    QMutex mutex;
    mutex.lock();
    data->con.wait( &mutex );
    mutex.unlock();

    K3b::Device::Device* dev = static_cast<K3b::Device::Device*>( data->data );

    // delete the no longer needed data
    K3b::ThreadWidget::instance()->clearId( data->id );

    return dev;
}


void K3b::ThreadWidget::customEvent( QEvent* e )
{
    if( DeviceSelectionEvent* dse = dynamic_cast<DeviceSelectionEvent*>(e) ) {
        // create dialog
        K3b::Device::Device* dev = K3b::DeviceSelectionDialog::selectDevice( dse->parent(), dse->text() );

        // return it to the thread
        Data* dat = data( dse->id() );
        dat->data = static_cast<void*>( dev );

        // wake the thread
        dat->con.wakeAll();
    }
}


