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

#include "k3bpluginfactory.h"


template <class T>
KInstance* K3bPluginFactory<T>::s_instance = 0;


template <class T>
K3bPluginFactory<T>* K3bPluginFactory<T>::s_self = 0;


template <class T>
KInstance* K3bPluginFactory<T>::instance()
{
  if( !s_instance && s_self )
    s_instance = new KInstance( s_self->m_instanceName );
  return s_instance;
}
