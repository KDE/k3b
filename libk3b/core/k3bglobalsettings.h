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

#ifndef _K3B_GLOBAL_SETTINGS_H_
#define _K3B_GLOBAL_SETTINGS_H_
#include "k3b_export.h"
class KConfig;

/**
 * Some global settings used throughout K3b.
 */
class LIBK3B_EXPORT K3bGlobalSettings
{
 public:
  K3bGlobalSettings();

  /**
   * This method takes care of settings the config group
   */
  void readSettings( KConfig* );

  /**
   * This method takes care of settings the config group
   */
  void saveSettings( KConfig* );

  bool ejectMedia() const { return m_eject; }
  bool burnfree() const { return m_burnfree; }
  bool overburn() const { return m_overburn; }
  bool useManualBufferSize() const { return m_useManualBufferSize; }
  int bufferSize() const { return m_bufferSize; }

  /**
   * If force is set to true K3b will continue in certain "unsafe" situations.
   * The most common being a medium not suitable for the writer in terms of
   * writing speed.
   * Compare cdrecord's parameter -force and -ignsize
   */
  bool force() const { return m_force; }

  void setEjectMedia( bool b ) { m_eject = b; }
  void setBurnfree( bool b ) { m_burnfree = b; }
  void setOverburn( bool b ) { m_overburn = b; }
  void setUseManualBufferSize( bool b ) { m_useManualBufferSize = b; }
  void setBufferSize( int size ) { m_bufferSize = size; }
  void setForce( bool b ) { m_force = b; }

 private:
  bool m_eject;
  bool m_burnfree;
  bool m_overburn;
  bool m_useManualBufferSize;
  int m_bufferSize;
  bool m_force;
};


#endif
