/* 
 *
 * $Id: $
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef K3B_AUDIO_MODULE_FACTORY
#define K3B_AUDIO_MODULE_FACTORY


#include <qobject.h>
#include <qptrlist.h>

class K3bAudioTrack;
class K3bAudioModule;
class KURL;

class K3bAudioModuleFactory : public QObject
{
  Q_OBJECT

 public:
  ~K3bAudioModuleFactory();

  /** returns NULL if no Module for that type of file is available.
      for now these are static but in the future there could be some plugin
      mode with dynamicly loaded audiomodules */
  K3bAudioModule* createModule( K3bAudioTrack* );
  bool moduleAvailable( const KURL& url );

  static K3bAudioModuleFactory* self();

 private:
  K3bAudioModuleFactory();

  QPtrList<K3bAudioModule> m_modules;
};

#endif
