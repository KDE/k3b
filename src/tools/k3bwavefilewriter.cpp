/***************************************************************************
                          k3bwavefilewriter.cpp  -  description
                             -------------------
    begin                : Wed Feb 27 2002
    copyright            : (C) 2002 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "k3bwavefilewriter.h"

K3bWaveFileWriter::K3bWaveFileWriter()
  : m_outputStream( &m_outputFile )
{
  m_dataWritten = 0;
}


K3bWaveFileWriter::~K3bWaveFileWriter()
{
  close();
}


bool K3bWaveFileWriter::open( const QString& filename )
{
  close();

  m_dataWritten = 0;

  m_outputFile.setName( filename );

  if( m_outputFile.open( IO_ReadWrite ) ) {
    m_filename = filename;

    writeEmptyHeader();

    return true;
  }
  else {
    return false;
  }
}


void K3bWaveFileWriter::close()
{
  if( isOpen() ) {
    if( m_dataWritten ) {
      // update wave header
      updateHeader();

      m_outputFile.close();
    }
    else {
      m_outputFile.close();
      m_outputFile.remove();
    }
  }

  m_filename = QString::null;
}


bool K3bWaveFileWriter::isOpen()
{
  return m_outputFile.isOpen();
}


const QString& K3bWaveFileWriter::filename() const 
{
  return m_filename;
}


void K3bWaveFileWriter::write( const char* data, int len, Endianess e )
{
  if( isOpen() ) {
    if( e == LittleEndian ) {
      m_outputStream.writeRawBytes( data, len );
    }
    else {
      if( len % 2 > 0 ) {
	qDebug("(K3bWaveFileWriter) data length is not a multible of 2! Cannot write data.");
	return;
      }

      // we need to swap the bytes
      char buffer[len];
      for( int i = 0; i < len-1; i+=2 ) {
	buffer[i] = data[i+1];
	buffer[i+1] = data[i];
      }
      m_outputStream.writeRawBytes( buffer, len );
    }

    m_dataWritten += len;
  }
}


void K3bWaveFileWriter::writeEmptyHeader()
{
  static char riffHeader[] =
  {
    0x52, 0x49, 0x46, 0x46, // 0  "RIFF"
    0x00, 0x00, 0x00, 0x00, // 4  wavSize
    0x57, 0x41, 0x56, 0x45, // 8  "WAVE"
    0x66, 0x6d, 0x74, 0x20, // 12 "fmt "
    0x10, 0x00, 0x00, 0x00, // 16
    0x01, 0x00, 0x02, 0x00, // 20
    0x44, 0xac, 0x00, 0x00, // 24
    0x10, 0xb1, 0x02, 0x00, // 28
    0x04, 0x00, 0x10, 0x00, // 32
    0x64, 0x61, 0x74, 0x61, // 36 "data"
    0x00, 0x00, 0x00, 0x00  // 40 byteCount
  };

  m_outputStream.writeRawBytes( riffHeader, 44 );
}


void K3bWaveFileWriter::updateHeader()
{
  if( isOpen() ) {

    m_outputFile.flush();

    char c[4];
    Q_INT32 wavSize(m_dataWritten + 44 - 8);

    // jump to the wavSize position in the header

    m_outputFile.at( 4 );
    c[0] = (wavSize   >> 0 ) & 0xff;
    c[1] = (wavSize   >> 8 ) & 0xff;
    c[2] = (wavSize   >> 16) & 0xff;
    c[3] = (wavSize   >> 24) & 0xff;
    m_outputStream.writeRawBytes( c, 4 );

    m_outputFile.at( 40 );
    c[0] = (m_dataWritten   >> 0 ) & 0xff;
    c[1] = (m_dataWritten   >> 8 ) & 0xff;
    c[2] = (m_dataWritten   >> 16) & 0xff;
    c[3] = (m_dataWritten   >> 24) & 0xff;
    m_outputStream.writeRawBytes( c, 4 );

    // jump back to the end
    m_outputFile.at( m_outputFile.size() );
  }
}
