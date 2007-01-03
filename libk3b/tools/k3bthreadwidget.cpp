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

#include "k3bthreadwidget.h"
#include "k3bdeviceselectiondialog.h"
#include <k3bdevice.h>

#include <qevent.h>
#include <qapplication.h>
#include <qwaitcondition.h>


class K3bThreadWidget::Data
{
public:
  int id;
  void* data;
  QWaitCondition con;
};


class K3bThreadWidget::DeviceSelectionEvent : public QCustomEvent
{
public:
  DeviceSelectionEvent( QWidget* parent, const QString& text, int id )
    : QCustomEvent( QEvent::User + 22 ),
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


K3bThreadWidget* K3bThreadWidget::s_instance = 0;


K3bThreadWidget::K3bThreadWidget()
  : QObject(),
    m_idCounter(1)
{
  m_dataMap.setAutoDelete(true);
  s_instance = this;
}


K3bThreadWidget::~K3bThreadWidget()
{
  s_instance = 0;
}


int K3bThreadWidget::getNewId()
{
  // create new data
  Data* data = new Data;
  data->id = m_idCounter++;
  data->data = 0;
  m_dataMap.insert( data->id, data );
  return data->id;
}


void K3bThreadWidget::clearId( int id )
{
  m_dataMap.remove( id );
}


K3bThreadWidget::Data* K3bThreadWidget::data( int id )
{
  return m_dataMap[id];
}


K3bThreadWidget* K3bThreadWidget::instance()
{
  if( !s_instance )
    s_instance = new K3bThreadWidget();
  return s_instance;
}


// static
K3bDevice::Device* K3bThreadWidget::selectDevice( QWidget* parent, 
						  const QString& text )
{
  // request a new data set
  Data* data = K3bThreadWidget::instance()->data( K3bThreadWidget::instance()->getNewId() );

  // inform the instance about the request
  QApplication::postEvent( K3bThreadWidget::instance(),
			   new K3bThreadWidget::DeviceSelectionEvent( parent, text, data->id ) );

  // wait for the result to be ready
  data->con.wait();

  K3bDevice::Device* dev = static_cast<K3bDevice::Device*>( data->data );

  // delete the no longer needed data
  K3bThreadWidget::instance()->clearId( data->id );

  return dev;
}


void K3bThreadWidget::customEvent( QCustomEvent* e )
{
  if( DeviceSelectionEvent* dse = dynamic_cast<DeviceSelectionEvent*>(e) ) {
    // create dialog
    K3bDevice::Device* dev = K3bDeviceSelectionDialog::selectDevice( dse->parent(), dse->text() );

    // return it to the thread
    Data* dat = data( dse->id() );
    dat->data = static_cast<void*>( dev );

    // wake the thread
    dat->con.wakeAll();
  }
}

#include "k3bthreadwidget.moc"
