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
#include <k3bcore.h>

#include <kdebug.h>
#include <kcombobox.h>
#include <klocale.h>
#include <kconfig.h>
#include <kdialog.h>

#include <qlayout.h>
#include <qlabel.h>

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

  bool swap;
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

// from xine-lib
static int resume(snd_pcm_t *pcm)
{
  int res;
  while ((res = snd_pcm_resume(pcm)) == -EAGAIN)
    sleep(1);
  if (! res)
    return 0;
  return snd_pcm_prepare(pcm);
}


int K3bAlsaOutputPlugin::write( char* data, int len )
{
  if( d->error )
    return -1;

  snd_pcm_state_t state = snd_pcm_state( d->pcm_playback );

  if( state == SND_PCM_STATE_SUSPENDED ) {
    int r = resume( d->pcm_playback );
    if( r < 0 ) {
      kdDebug() << "(K3bAlsaOutputPlugin) resume failed: " << snd_strerror(r) << endl;
      d->lastErrorMessage = i18n("Internal Alsa problem: %1").arg(snd_strerror(r));
      return -1;
    }
    state = snd_pcm_state( d->pcm_playback );
  }

  if( state == SND_PCM_STATE_XRUN ) {
    kdDebug() << "(K3bAlsaOutputPlugin) preparing..." << endl;
    int r = snd_pcm_prepare( d->pcm_playback );
    if( r < 0 ) {
      kdDebug() << "(K3bAlsaOutputPlugin) failed preparation: " << snd_strerror(r) << endl;
      d->lastErrorMessage = i18n("Internal Alsa problem: %1").arg(snd_strerror(r));
      return -1;
    }
  }

  char* buffer = data;
  if( d->swap ) {
    buffer = new char[len];
    for( int i = 0; i < len-1; i+=2 ) {
      buffer[i] = data[i+1];
      buffer[i+1] = data[i];
    }
  }

  int written = 0;
  while( written < len ) {
    if( state == SND_PCM_STATE_RUNNING ) {
      int wait_result = snd_pcm_wait( d->pcm_playback, 1000000 );
      if( wait_result < 0 ) {
	kdDebug() << "(K3bAlsaOutputPlugin) wait failed: " << snd_strerror(wait_result) << endl;
	d->lastErrorMessage = i18n("Internal Alsa problem: %1").arg(snd_strerror(wait_result));
	return -1;
      }
    }

    if( state != SND_PCM_STATE_PREPARED &&
	state != SND_PCM_STATE_DRAINING &&
	state != SND_PCM_STATE_RUNNING ) {
      kdDebug() << "(K3bAlsaOutputPlugin) invalid state: " << state << endl;
      d->lastErrorMessage = i18n("Internal Alsa problem: %1").arg("invalid state");
      return -1;
    }
    
    snd_pcm_sframes_t frames = snd_pcm_writei( d->pcm_playback, 
					       buffer+written, 
					       snd_pcm_bytes_to_frames( d->pcm_playback, len-written ) );
    if( frames > 0 )
      written += snd_pcm_frames_to_bytes( d->pcm_playback, frames );
    else {
      kdDebug() << "(K3bAlsaOutputPlugin) write failed: " << snd_strerror(frames) << endl;
      d->lastErrorMessage = i18n("Internal Alsa problem: %1").arg(snd_strerror(frames));
      return -1;
    }
  }

  return written;
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
  cleanup();

  KConfigGroup c( k3bcore->config(), "Alsa Output Plugin" );
  QString alsaDevice = c.readEntry( "output device", "default" );

  int err = snd_pcm_open( &d->pcm_playback, alsaDevice.local8Bit(), SND_PCM_STREAM_PLAYBACK, 0 );
  if( err < 0 ) {
    d->lastErrorMessage = i18n("Could not open alsa audio device '%1' (%2).").arg(alsaDevice).arg(snd_strerror(err));
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
    if( (err = snd_pcm_hw_params_set_format( d->pcm_playback, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
      d->lastErrorMessage = i18n("Could not set sample format (%1).").arg(snd_strerror(err));
      snd_pcm_hw_params_free( hw_params );
      d->error = true;
      return false;
    }
    else
      d->swap = true;
  }
  else
    d->swap = false;

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
  
//   if( (err = snd_pcm_prepare( d->pcm_playback )) < 0 ) {
//     d->lastErrorMessage = i18n("Could not prepare audio interface for use (%1).").arg(snd_strerror(err));
//     d->error = true;
//     return false;
//   }

  d->error = false;
  return true;
}


QString K3bAlsaOutputPlugin::lastErrorMessage() const
{
  return d->lastErrorMessage;
}


K3bPluginConfigWidget* K3bAlsaOutputPlugin::createConfigWidget( QWidget* parent, 
								const char* name ) const
{
  return new K3bAlsaOutputPluginConfigWidget( parent, name );
}



K3bAlsaOutputPluginConfigWidget::K3bAlsaOutputPluginConfigWidget( QWidget* parent, const char* name )
  : K3bPluginConfigWidget( parent, name )
{
  QHBoxLayout* l = new QHBoxLayout( this );
  l->setSpacing( KDialog::spacingHint() );
  l->setAutoAdd( true );

  (void)new QLabel( i18n("Alsa device:"), this );

  m_comboDevice = new KComboBox( this );
  m_comboDevice->setEditable( true );
  // enable completion
  m_comboDevice->completionObject();

  // FIXME: initialize the list of devices
  m_comboDevice->insertItem( "default" );
}


K3bAlsaOutputPluginConfigWidget::~K3bAlsaOutputPluginConfigWidget()
{
}


void K3bAlsaOutputPluginConfigWidget::loadConfig()
{
  KConfigGroup c( k3bcore->config(), "Alsa Output Plugin" );

  m_comboDevice->setCurrentText( c.readEntry( "output device", "default" ) );
}


void K3bAlsaOutputPluginConfigWidget::saveConfig()
{
  KConfigGroup c( k3bcore->config(), "Alsa Output Plugin" );

  c.writeEntry( "output device", m_comboDevice->currentText() );
}


#include "k3balsaoutputplugin.moc"
