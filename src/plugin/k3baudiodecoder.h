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

#ifndef _K3B_AUDIO_DECODER_H_
#define _K3B_AUDIO_DECODER_H_


#include <k3bplugin.h>
#include <k3bpluginfactory.h>
#include <k3bmsf.h>

#include <kurl.h>



/**
 * PluginFactory that needs to be subclassed in order to create an
 * audio decoder.
 */
class K3bAudioDecoderFactory : public K3bPluginFactory
{
  Q_OBJECT

 public:
  K3bAudioDecoderFactory( QObject* parent = 0, const char* name = 0 )
    : K3bPluginFactory( parent, name ) {
  }

  virtual ~K3bAudioDecoderFactory() {
  }

  QString group() const { return "AudioDecoder"; }

  virtual bool canDecode( const KURL& filename ) = 0;
};



/**
 * Abstract streaming class for all the audio input.
 * Has to output data in the following format:
 * MSBLeft LSBLeft MSBRight LSBRight (big endian byte order)
 **/
class K3bAudioDecoder : public K3bPlugin
{
  Q_OBJECT

 public:
  K3bAudioDecoder( QObject* parent = 0, const char* name = 0 );
  virtual ~K3bAudioDecoder();


  /**
   * Set the file to decode. Be aware that one cannot rely 
   * on the file length until analyseFile() has been called.
   */
  void setFilename( const QString& );

  /**
   * Since this may take a while depending on the filetype it is best
   * to run it in a seperate thread.
   */
  bool analyseFile();

  /**
   * initialze the decoding.
   */
  bool initDecoder();

  /**
   * This should at least support "Title" and "Artist"
   * The default implementation uses KFileMetaInfo
   */ 
  virtual QString metaInfo( const QString& );

  /**
   * returnes the filetype of the decoded file
   */
  virtual QString fileType() const { return QString::null; }
  virtual QStringList supportedTechnicalInfos() const { return QStringList(); }

  /**
   * Technical info about the file. Be aware that one cannot rely 
   * on the technical infos until analyseFile() has been called.
   */
  virtual QString technicalInfo( const QString& ) const { return QString::null; }

  /**
   * returnes -1 on error, 0 when finished, length of data otherwise
   * takes care of padding
   * calls decodeInternal() to actually decode data
   *
   * Fill the data buffer with maximal maxLen bytes.
   */
  int decode( char* data, int maxLen );

  /**
   * cleanup after decoding like closing files.
   */
  virtual void cleanup();

  /**
   * Seek to the position pos.
   * Decoding is started new. That means that the data will be padded to
   * length() - pos.
   * returnes true on success;
   */
  bool seek( const K3b::Msf& pos );

  /**
   * Be aware that one cannot rely 
   * on the file length until analyseFile() has been called.
   */
  virtual K3b::Msf length() const { return m_length; }

  const QString& filename() const { return m_fileName; }

 protected:
  virtual bool initDecoderInternal() = 0;

  /**
   * This method should calculate the length of the file.
   */
  virtual bool analyseFileInternal() = 0;

  /**
   * fill the already allocated data with maximal maxLen bytes of decoded samples.
   * This should calculate the length. May be time consuming.
   */
  virtual int decodeInternal( char* data, int maxLen ) = 0;

  virtual bool seekInternal( const K3b::Msf& ) { return false; }

  void setLength( const K3b::Msf& len ) { m_length = len; }

 private:
  QString m_fileName;
  K3b::Msf m_length;

  class Private;
  Private* d;
};


#endif
