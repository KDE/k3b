/* 
 * FLAC decoder module for K3b.
 * Based on the Ogg Vorbis module for same.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2003 John Steele Scott <toojays@toojays.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bflacdecoder.h"

#include <qbuffer.h>
#include <qfile.h>
#include <qstringlist.h>

#include <kurl.h>
#include <kdebug.h>
#include <klocale.h>
#include <kinstance.h>

#include <string.h>
#include <math.h>
#include <FLAC++/metadata.h>
#include <FLAC++/decoder.h>



class K3bFLACDecoder::Private
  : public FLAC::Decoder::SeekableStream
{
public:
  void open(QFile* f) {
    file = f;	
    file->open(IO_ReadOnly);
    
    internalBuffer->flush();
    
    set_metadata_respond(FLAC__METADATA_TYPE_STREAMINFO);
    set_metadata_respond(FLAC__METADATA_TYPE_VORBIS_COMMENT);

    init();
    process_until_end_of_metadata();
  }

  void cleanup() {
    file->close();
    finish();
    delete comments;
    comments = 0;
  }

  Private(QFile* f)
    : FLAC::Decoder::SeekableStream(),
      comments(0) {
    internalBuffer = new QBuffer();
    internalBuffer->open(IO_ReadWrite);

    open(f);
  }
  

  ~Private() {
    cleanup();
    delete internalBuffer;
  }
  
  bool seekToFrame(int frame);

  QFile* file;
  QBuffer* internalBuffer;
  FLAC::Metadata::VorbisComment* comments;
  unsigned rate;
  unsigned channels;
  unsigned bitsPerSample;
  unsigned maxFramesize;
  unsigned maxBlocksize;
  unsigned minFramesize;
  unsigned minBlocksize;
  FLAC__uint64 samples;
  
protected:
  virtual FLAC__SeekableStreamDecoderReadStatus read_callback(FLAC__byte buffer[], unsigned *bytes);
  virtual FLAC__SeekableStreamDecoderSeekStatus seek_callback(FLAC__uint64 absolute_byte_offset);
  virtual FLAC__SeekableStreamDecoderTellStatus tell_callback(FLAC__uint64 *absolute_byte_offset);
  virtual FLAC__SeekableStreamDecoderLengthStatus length_callback(FLAC__uint64 *stream_length);
  virtual bool eof_callback();
  virtual void error_callback(FLAC__StreamDecoderErrorStatus){};
  virtual void metadata_callback(const ::FLAC__StreamMetadata *metadata);
  virtual ::FLAC__StreamDecoderWriteStatus write_callback(const ::FLAC__Frame *frame, const FLAC__int32 * const buffer[]);
};

bool K3bFLACDecoder::Private::seekToFrame(int frame) {
  FLAC__uint64 sample = frame * rate / 75;
  return seek_absolute(sample);
}

bool K3bFLACDecoder::Private::eof_callback() {
  return file->atEnd();
}

FLAC__SeekableStreamDecoderReadStatus K3bFLACDecoder::Private::read_callback(FLAC__byte buffer[],       								    unsigned *bytes) {
  long retval =  file->readBlock((char *)buffer, (*bytes));
  if(-1 == retval) {
    return FLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_ERROR;
  } else {
    (*bytes) = retval;
    return FLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_OK;
  }
}

FLAC__SeekableStreamDecoderSeekStatus 
K3bFLACDecoder::Private::seek_callback(FLAC__uint64 absolute_byte_offset) {
  if(file->at(absolute_byte_offset) == FALSE)
    return FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_ERROR;
  else
    return FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_OK;
}

FLAC__SeekableStreamDecoderTellStatus 
K3bFLACDecoder::Private::tell_callback(FLAC__uint64 *absolute_byte_offset) {
  (*absolute_byte_offset) = file->at();
  return FLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_OK;
}

FLAC__SeekableStreamDecoderLengthStatus 
K3bFLACDecoder::Private::length_callback(FLAC__uint64 *stream_length) {
  (*stream_length) = file->size();
  return FLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_OK;
}


void K3bFLACDecoder::Private::metadata_callback(const FLAC__StreamMetadata *metadata) {
  switch (metadata->type) {
  case FLAC__METADATA_TYPE_STREAMINFO:
    channels = metadata->data.stream_info.channels;
    rate = metadata->data.stream_info.sample_rate;
    bitsPerSample = metadata->data.stream_info.bits_per_sample;
    samples = metadata->data.stream_info.total_samples;
    maxFramesize = metadata->data.stream_info.max_framesize;
    minFramesize = metadata->data.stream_info.min_framesize;
    maxBlocksize = metadata->data.stream_info.max_blocksize;
    minBlocksize = metadata->data.stream_info.min_blocksize;
    break;
  case FLAC__METADATA_TYPE_VORBIS_COMMENT:
    comments = new FLAC::Metadata::VorbisComment((FLAC__StreamMetadata *)metadata, true);
    break;
  default:
    break;
  }
}

FLAC__StreamDecoderWriteStatus K3bFLACDecoder::Private::write_callback(const FLAC__Frame *frame, const FLAC__int32 * const buffer[]) {
  unsigned i, j;
  // Note that in canDecode we made sure that the input is 1-16 bit stereo or mono.
  unsigned samples = frame->header.blocksize;

  for(i=0; i < samples; i++) {
   // in FLAC channel 0 is left, 1 is right
   for(j=0; j < this->channels; j++) {
    FLAC__int32 value = (buffer[j][i])<<(16 - frame->header.bits_per_sample);
    internalBuffer->putch(value >> 8); // msb
    internalBuffer->putch(value & 0xFF); // lsb
   }
  }

  // Rewind the buffer so the decode method will take data from the beginning.
  internalBuffer->at(0);
  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

K3bFLACDecoder::K3bFLACDecoder( QObject* parent, const char* name )
  : K3bAudioDecoder( parent, name )
{
  d = 0;
}


K3bFLACDecoder::~K3bFLACDecoder()
{
  delete d;
}

void K3bFLACDecoder::cleanup()
{
  if (d) {
    d->cleanup();
    d->open(new QFile(filename()));
  }
  else
    d = new Private(new QFile(filename()));
}

bool K3bFLACDecoder::analyseFileInternal( K3b::Msf& frames, int& samplerate, int& ch )
{
  cleanup();

  frames = (unsigned long)ceil((d->samples * 75.0))/d->rate;
  samplerate = d->rate;
  ch = d->channels;

  return true;
}


bool K3bFLACDecoder::initDecoderInternal()
{
  cleanup();

  return true;
}


int K3bFLACDecoder::decodeInternal( char* _data, int maxLen )
{
  int bytesToCopy;
  int bytesCopied;
  int bytesAvailable;

  if(d->internalBuffer->size() == 0) {
    // want more data
    switch(d->get_state()) {
    case FLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM:
      d->finish();
      break;
    case FLAC__SEEKABLE_STREAM_DECODER_OK:
      if(! d->process_single())
	return -1;
      break;
    default:
      return -1;
    }
  }
  
  bytesAvailable = d->internalBuffer->size() - d->internalBuffer->at();
  bytesToCopy = QMIN(maxLen, bytesAvailable);
  bytesCopied = (int)d->internalBuffer->readBlock(_data, bytesToCopy);

  if(bytesCopied == bytesAvailable) {
    // reset the buffer
    d->internalBuffer->close();
    d->internalBuffer->open(IO_ReadWrite|IO_Truncate);
  }

  return bytesCopied;
}


QString K3bFLACDecoder::metaInfo( const QString& tag )
{
  if(d == 0)
    d = new Private(new QFile(filename()));

  if(d->comments == 0)
    return QString::null;

  for( unsigned int i = 0; i < d->comments->get_num_comments(); i++ ) {
	QString comment = QString::fromUtf8( d->comments->get_comment(i).get_field(),
					     d->comments->get_comment(i).get_field_length() );
	QStringList values = QStringList::split( "=", comment );
	if( values.count() > 1 )
	  if( values[0].lower() == tag.lower() )
	    return values[1];
  }

  return QString::null;
}


bool K3bFLACDecoder::seekInternal( const K3b::Msf& pos )
{
  return d->seekToFrame(pos.totalFrames());
}


QString K3bFLACDecoder::fileType() const
{
  return i18n("FLAC");
}


QStringList K3bFLACDecoder::supportedTechnicalInfos() const
{
  return QStringList::split( ";", 
			     i18n("Channels") + ";" +
			     i18n("Sampling Rate") + ";" +
			     i18n("Sample Size") );
}


QString K3bFLACDecoder::technicalInfo( const QString& info ) const
{
  if( d->comments != 0 ) {
    if( info == i18n("Vendor") )
      return QString::fromUtf8(d->comments->get_vendor_string().get_field());
    else if( info == i18n("Channels") )
      return QString::number(d->channels);
    else if( info == i18n("Sampling Rate") )
      return i18n("%1 Hz").arg(d->rate);
    else if( info == i18n("Samples") )
      return QString::number(d->samples);
    else if( info == i18n("Sample Size") )
      return i18n("%1 bits").arg(d->bitsPerSample);
  }

  return QString::null;
}



K3bFLACDecoderFactory::K3bFLACDecoderFactory( QObject* parent, const char* name )
  : K3bAudioDecoderFactory( parent, name )
{
  s_instance = new KInstance( "k3bflacdecoder" );
}


K3bFLACDecoderFactory::~K3bFLACDecoderFactory()
{
}


K3bPlugin* K3bFLACDecoderFactory::createPluginObject( QObject* parent, 
						      const char* name,
						      const QStringList& )
{
  return new K3bFLACDecoder( parent, name );
}


bool K3bFLACDecoderFactory::canDecode( const KURL& url )
{
  char buf[4];

  QFile file(url.path());
  if(!file.open(IO_ReadOnly)) {
    kdDebug() << "(K3bFLACDecoder) Could not open file " << url.path() << endl;
    return false;
  }

  file.readBlock(buf, 4);
  file.close();

  if(memcmp(buf, "fLaC", 4) != 0) {

    kdDebug() << "(K3bFLACDecoder) " << url.path() << ": not a FLAC file" << endl;
    return false;
  }

  FLAC::Metadata::StreamInfo info = FLAC::Metadata::StreamInfo();
  FLAC::Metadata::get_streaminfo(url.path().ascii(), info);

  if((info.get_channels() <= 2) &&
     (info.get_bits_per_sample() <= 16)) {
    return true;
  } else {
    kdDebug() << "(K3bFLACDecoder) " << url.path() << ": wrong format:" << endl
	      << "                channels:    " 
	      << QString::number(info.get_channels()) << endl
	      << "                samplerate:  "
	      << QString::number(info.get_sample_rate()) << endl
	      << "                bits/sample: "
	      << QString::number(info.get_bits_per_sample()) << endl;
    return false;
  }
}

#include "k3bflacdecoder.moc"
