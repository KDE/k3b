/* 
 *
 * $Id: k3bartsoutputplugin.cpp 369057 2004-12-07 14:05:11Z trueg $
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
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

  unsigned int sampleRate;
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
    snd_pcm_sframes_t frames = snd_pcm_writei( d->pcm_playback, 
					       buffer+written, 
					       snd_pcm_bytes_to_frames( d->pcm_playback, len-written ) );

    if( frames < 0 ) {
      if( !recoverFromError( frames ) ) {
	d->error = true;
	return -1;
      }
    }
    else {
      written += snd_pcm_frames_to_bytes( d->pcm_playback, frames );
    }
  }

  return written;
}


bool K3bAlsaOutputPlugin::recoverFromError( int err )
{
  if( err == -EPIPE ) {
    err = snd_pcm_prepare( d->pcm_playback );
    if( err < 0 ) {
      d->lastErrorMessage = i18n("Internal Alsa problem: %1").arg(snd_strerror(err));
      return false;
    }
  }
  else if( err == -ESTRPIPE ) {
    while( ( err = snd_pcm_resume( d->pcm_playback ) ) == -EAGAIN )
      sleep( 1 );

    if (err < 0) {
      // unable to wake up pcm device, restart it
      err = snd_pcm_prepare( d->pcm_playback );
      if( err < 0 ) {
	d->lastErrorMessage = i18n("Internal Alsa problem: %1").arg(snd_strerror(err));
	return false;
      }
    }

    return true;
  }

  return false;
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

  if( !setupHwParams() ) {
    d->error = true;
    return false;
  }  

  d->error = false;
  return true;
}


bool K3bAlsaOutputPlugin::setupHwParams()
{
  snd_pcm_hw_params_t* hw_params;
  int err = 0;

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

  d->sampleRate = 44100;
  if( (err = snd_pcm_hw_params_set_rate_near( d->pcm_playback, hw_params, &d->sampleRate, 0)) < 0) {
    d->lastErrorMessage = i18n("Could not set sample rate (%1).").arg(snd_strerror(err));
    snd_pcm_hw_params_free( hw_params );
    d->error = true;
    return false;
  }

  kdDebug() << "(K3bAlsaOutputPlugin) samplerate set to " << d->sampleRate << endl;

  if( (err = snd_pcm_hw_params_set_channels( d->pcm_playback, hw_params, 2)) < 0) {
    d->lastErrorMessage = i18n("Could not set channel count (%1).").arg(snd_strerror(err));
    snd_pcm_hw_params_free( hw_params );
    d->error = true;
    return false;
  }

  if( (err = snd_pcm_hw_params( d->pcm_playback, hw_params )) < 0) {
    d->lastErrorMessage = i18n("Could not set parameters (%1).").arg(snd_strerror(err));
    snd_pcm_hw_params_free( hw_params );
    d->error = true;
    return false;
  }
        
  snd_pcm_hw_params_free(hw_params);

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
