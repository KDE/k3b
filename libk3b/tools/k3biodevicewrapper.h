/* 
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_IODEVICE_WRAPPER_H_
#define _K3B_IODEVICE_WRAPPER_H_

#include <qiodevice.h>
#include <qvaluelist.h>

/**
 * This is a wrapper class that allows to just use a portion of the data a QIODevice
 * provides
 */
class K3bIODeviceWrapper : public QIODevice
{
 public:
  /**
   * Creates a new wrapper.
   *
   * \param takeOwnerShip if true the parent iodevice will be deleted along with this one.
   */
  K3bIODeviceWrapper( bool takeOwnerShip = false )
    : m_deleteChilds( takeOwnerShip ),
    m_overallLength( (Offset)0 ) {
    m_isOpen = false;
  }

  /**
   * Creates a new wrapper and already adds one device
   *
   * \param takeOwnerShip if true the parent iodevice will be deleted along with this one.
   */
  K3bIODeviceWrapper( QIODevice* child, Offset start, Offset length, bool takeOwnerShip = false )
    : m_deleteChilds( takeOwnerShip ),
    m_overallLength( (Offset)0 ) {
    m_isOpen = false;
    addWrappedIODevice( child, start, length );
  }

  ~K3bIODeviceWrapper() {
    if( m_deleteChilds )
      for( m_childIt = m_children.begin(); m_childIt != m_children.end(); ++m_childIt )
	delete (*m_childIt).device;
  }

  void addWrappedIODevice( QIODevice* child, Offset start, Offset length ) {
    close();

    m_children.append( ChildDevice( child, start, length ) );
    m_overallLength += length;
  }

  bool open( int mode ) {
    if( !m_children.isEmpty() ) {
      m_childIt = m_children.begin();

      return( (*m_childIt).device->open( mode ) &&
	      (*m_childIt).device->at( (*m_childIt).start ) );
    }
    else
      return false;
  }

  void close() {
    if( m_isOpen ) {
      if( m_childIt != m_children.end() )
	(*m_childIt).device->close();
      m_isOpen = false;
    }
  }

  void flush() {
    if( m_isOpen ) {
      if( m_childIt != m_children.end() )
	(*m_childIt).device->flush();
    }
  }
  
  Offset size() const {
    return m_overallLength;
  }
  
  Offset at() const {
    if( m_isOpen ) {
      Offset pos = 0;
      for( QValueList<ChildDevice>::const_iterator it = m_children.begin(); 
	   it != m_childIt; ++it )
	pos += (*it).length;
      if( m_childIt != m_children.end() )
	return pos + (*m_childIt).device->at() - (*m_childIt).start;
      else
	return pos;
    }
    else
      return (Offset)0;
  }
  
  bool at( Offset pos ) {
    if( m_isOpen ) {
      Offset tmp = 0;
      QValueList<ChildDevice>::const_iterator it;
      for( it = m_children.begin(); it != m_children.end(); ++it ) {
	if( tmp + (*it).length < pos )
	  tmp += (*it).length;
	else
	  break;
      }
      
      if( it == m_children.end() )
	return false;
      else
	return (*it).device->at( pos - tmp + (*it).start );
    }
    else
      return false;
  }
  
  bool atEnd() const {
    if( m_isOpen ) {
      QValueList<ChildDevice>::const_iterator it = m_childIt;
      ++it;
      return ( m_childIt == m_children.end() ||
	       ( it == m_children.end() &&
		 (*m_childIt).device->atEnd() ) );
    }
    else
      return true;
  }

  Q_LONG readBlock( char* data, Q_ULONG maxlen ) {
    if( m_isOpen ) {
      if( m_childIt != m_children.end() ) {
	// read from the current device as much as possible
	Q_ULONG newlen = QMIN( maxlen, (*m_childIt).length - ( (*m_childIt).device->at() - (*m_childIt).start ) );
	if( newlen > 0 )
	  return (*m_childIt).device->readBlock( data, newlen );
	else {
	  // next device
	  m_childIt++;
	  return readBlock( data, maxlen );
	}
      }
      else
	return 0;
    }
    else
      return -1;
  }

  Q_LONG writeBlock( const char*, Q_ULONG ) {
    // no write support
    return -1;
  }

  int getch() {
    if( !atEnd() ) {
      if( (*m_childIt).device->atEnd() )
	++m_childIt;
      return (*m_childIt).device->getch();
    }
    else
      return -1;
  }

  int putch( int ) {
    return -1;
  }

  int ungetch( int ch ) {
    return -1;
  }

 private:
  class ChildDevice {
  public:
    ChildDevice() {}
    ChildDevice( QIODevice* _device,
		 Offset _start,
		 Offset _length )
      : device(_device),
      start(_start),
      length(_length) {}

    QIODevice* device;
    Offset start;
    Offset length;
  };

  QValueList<ChildDevice> m_children;
  QValueList<ChildDevice>::const_iterator m_childIt;

  bool m_isOpen;
  bool m_deleteChilds;

  Offset m_overallLength;
};

#endif
