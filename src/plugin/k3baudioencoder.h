/* 
 *
 * $Id$
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

#ifndef _K3B_AUDIO_ENCODER_H_
#define _K3B_AUDIO_ENCODER_H_

#include <k3bplugin.h>
#include <k3bpluginfactory.h>

#include <k3bmsf.h>


/**
 * PluginFactory that needs to be subclassed in order to create an
 * audio encoder.
 */
class K3bAudioEncoderFactory : public K3bPluginFactory
{
  Q_OBJECT

 public:
  K3bAudioEncoderFactory( QObject* parent = 0, const char* name = 0 )
    : K3bPluginFactory( parent, name ) {
  }

  virtual ~K3bAudioEncoderFactory() {
  }

  QString group() const { return "AudioEncoder"; }

  /**
   * This should return the fileextension used by the filetype written in the
   * encoder.
   */
  virtual QString extension() const = 0;

  /**
   * The filetype as presented to the user.
   */
  virtual QString fileTypeComment() const = 0;

  /**
   * Determine the filesize of the encoded file (~)
   * default implementation returnes -1 (unknown)
   */
  virtual long long fileSize( const K3b::Msf& ) const { return -1; }
};



class K3bAudioEncoder : public K3bPlugin
{
  Q_OBJECT

 public:
  K3bAudioEncoder( QObject* parent = 0, const char* name = 0 );
  virtual ~K3bAudioEncoder();

  /**
   * The default implementation openes the file for writing with 
   * writeData. Normally this does not need to be reimplemented.
   */
  virtual bool openFile( const QString& filename );

  /**
   * The default implementation returnes true if openFile (default implementation) has been
   * successfully called. Normally this does not need to be reimplemented.
   */
  virtual bool isOpen() const;

  /**
   * The default implementation closes the file opened by openFile
   * (default implementation) 
   * Normally this does not need to be reimplemented.
   */
  virtual void closeFile();

  /**
   * The default implementation returnes the filename set in openFile
   * or QString::null if no file has been opened.
   * Normally this does not need to be reimplemented.
   */
  virtual const QString& filename() const;

  /**
   * K3b uses the following types:
   * Title, Artist, Comment, Year, Albumtitle, Albumartist
   * Calling this method does only make sense after successfully
   * calling openFile and before calling encode.
   * This calls setMetaDataInternal.
   */
  void setMetaData( const QString&, const QString& );

  /**
   * Returnes the amount of actually written bytes or -1 if an error
   * occured.
   */
  long encode( const char*, Q_ULONG len );

 protected:
  /**
   * Called by the default implementation of openFile
   * This calls initEncoderInternal.
   */
  bool initEncoder();

  /**
   * Called by the deafult implementation of openFile
   * This calls finishEncoderInternal.
   */
  void finishEncoder();

  /**
   * Use this to write the data to the file when
   * using the default implementation of openFile
   * Returnes the number of bytes actually written.
   */
  Q_LONG writeData( const char*, Q_ULONG len );

  /**
   * initzialize the decoder structures.
   * default implementation does nothing
   * this may already write data.
   */
  virtual bool initEncoderInternal();

  /**
   * reimplement this if the encoder needs to do some
   * finishing touch.
   */
  virtual void finishEncoderInternal();

  /**
   * encode the data and write it with writeData (when using
   * the default)
   * The data will always be 16bit 44100 Hz stereo little endian samples.
   * Should return the amount of actually written bytes (may be 0) and -1
   * on error.
   */
  virtual long encodeInternal( const char*, Q_ULONG len ) = 0;

  /**
   * default implementation does nothing
   * this may already write data.
   */
  virtual void setMetaDataInternal( const QString&, const QString& );

 private:
  class Private;
  Private* d;
};

#endif
