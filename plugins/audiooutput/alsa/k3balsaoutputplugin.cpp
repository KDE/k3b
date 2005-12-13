/* 
 *
 * $Id: k3bartsoutputplugin.cpp 369057 2004-12-07 14:05:11Z trueg $
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
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

#include <config.h>

#include "k3balsaoutputplugin.h"
#include <k3bpluginfactory.h>

#include <kdebug.h>

#include <alsa/asoundlib.h>
#include <alsa/pcm.h>


K_EXPORT_COMPONENT_FACTORY( libk3balsaoutputplugin, K3bPluginFactory<K3bAlsaOutputPlugin>( "k3balsaoutputplugin" ) )


class K3bAlsaOutputPlugin::Private
{
public:
  Private()
    : pcm_playback(0),
      error(false) {
  }

  snd_pcm_t *pcm_playback;

  bool error;  
  QString lastErrorMessage;
};


K3bAlsaOutputPlugin::K3bAlsaOutputPlugin( QObject* parent, const char* name )
  : K3bAudioOutputPlugin( parent, name )
{
  d = new Private;
}


K3bAlsaOutputPlugin::~K3bAlsaOutputPlugin()
{
  cleanup();

  delete d;
}


int K3bAlsaOutputPlugin::write( char* data, int len )
{
  if( d->error )
    return -1;

  int state = snd_pcm_writei( d->pcm_playback, data, snd_pcm_bytes_to_frames( d->pcm_playback, len ) );
  if( state > 0 )
    return snd_pcm_frames_to_bytes( d->pcm_playback, state );
  else
    return -1;
}


void K3bAlsaOutputPlugin::cleanup()
{
  if( d->pcm_playback ) {
    snd_pcm_drain( d->pcm_playback );
    snd_pcm_close( d->pcm_playback );
  }
  d->pcm_playback = 0;
  d->error = false;
}


bool K3bAlsaOutputPlugin::init()
{
  int err = 0;
  if( ( err = snd_pcm_open( &d->pcm_playback, "default", SND_PCM_STREAM_PLAYBACK, 0 ) < 0 ) ) {
    d->lastErrorMessage = i18n("Could not open default alsa audio device (%1).").arg(snd_strerror(err));
    d->error = true;
    return false;
  }

  snd_pcm_hw_params_t* hw_params;

  if( ( err = snd_pcm_hw_params_malloc( &hw_params ) ) < 0 ) {
    d->lastErrorMessage = i18n("Could not allocate hardware parameter structure (%1)").arg(snd_strerror(err));
    d->error = true;
    return false;
  }
                                 
  if( (err = snd_pcm_hw_params_any( d->pcm_playback, hw_params )) < 0) {
    d->lastErrorMessage = i18n("Could not initialize hardware parameter structure (%1).").arg(snd_strerror(err));
    snd_pcm_hw_params_free( hw_params );
    d->error = true;
    return false;
  }
        
  if( (err = snd_pcm_hw_params_set_access( d->pcm_playback, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
    d->lastErrorMessage = i18n("Could not set access type (%1).").arg(snd_strerror(err));
    snd_pcm_hw_params_free( hw_params );
    d->error = true;
    return false;

  }
        
  if( (err = snd_pcm_hw_params_set_format( d->pcm_playback, hw_params, SND_PCM_FORMAT_S16_BE)) < 0) {
    d->lastErrorMessage = i18n("Could not set sample format (%1).").arg(snd_strerror(err));
    snd_pcm_hw_params_free( hw_params );
    d->error = true;
    return false;

  }

  unsigned int rate = 44100;
  if( (err = snd_pcm_hw_params_set_rate_near( d->pcm_playback, hw_params, &rate, 0)) < 0) {
    d->lastErrorMessage = i18n("Could not set sample rate (%1).").arg(snd_strerror(err));
    snd_pcm_hw_params_free( hw_params );
    d->error = true;
    return false;

  }

  if( (err = snd_pcm_hw_params_set_channels( d->pcm_playback, hw_params, 2)) < 0) {
    d->lastErrorMessage = i18n("Could not set channel count (%1).").arg(snd_strerror(err));
    snd_pcm_hw_params_free( hw_params );
    d->error = true;
    return false;

  }
        
  if( (err = snd_pcm_hw_params( d->pcm_playback, hw_params)) < 0) {
    d->lastErrorMessage = i18n("Could not set parameters (%1).").arg(snd_strerror(err));
    snd_pcm_hw_params_free( hw_params );
    d->error = true;
    return false;
  }
        
  snd_pcm_hw_params_free(hw_params);
  
  if( (err = snd_pcm_prepare( d->pcm_playback )) < 0 ) {
    d->lastErrorMessage = i18n("Could not prepare audio interface for use (%1).").arg(snd_strerror(err));
    d->error = true;
    return false;
  }

  d->error = false;
  return true;
}


QString K3bAlsaOutputPlugin::lastErrorMessage() const
{
  return d->lastErrorMessage;
}

