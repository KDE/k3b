/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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

#ifndef K3B_DATA_EVENT_H
#define K3B_DATA_EVENT_H

#include <qevent.h>


/**
 * Custom event class for posting events corresponding to the
 * K3bJob signals. This is useful for a threaded job since
 * in that case it's not possible to emit signals that directly
 * change the GUI (see QThread docu).
 */
class K3bDataEvent : public QCustomEvent
{
 public:
  // make sure we get not in the way of K3bProgressInfoEvent
  static const int EVENT_TYPE = QEvent::User + 100;

  K3bDataEvent( const char* data, int len )
    : QCustomEvent( EVENT_TYPE ),
    m_data(data),
    m_length(len)
    {}

  const char* data() const { return m_data; }
  int length() const { return m_length; }

 private:
  const char* m_data;
  int m_length;
};

#endif
