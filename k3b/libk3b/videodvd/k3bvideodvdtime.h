/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_VIDEODVD_TIME_H_
#define _K3B_VIDEODVD_TIME_H_

#include <k3b_export.h>

#include <qstring.h>

/**
 * This should not be confused with K3b::Msf
 */
namespace K3bVideoDVD
{
  class LIBK3B_EXPORT Time
    {
    public:
      Time();
      Time( unsigned short hour,
	    unsigned short min,
	    unsigned short sec,
	    unsigned short frame );

      unsigned short hour() const { return m_hour; }
      unsigned short minute() const { return m_minute; }
      unsigned short second() const { return m_second; }
      unsigned short frame() const { return m_frame; }

      double totalSeconds() const;
      unsigned int totalFrames() const;

      // FIXME: is this useful?
      double frameRate() const;

      QString toString( bool includeFrames = true ) const;

    private:
      void makeValid();

      unsigned int m_hour;
      unsigned int m_minute;
      unsigned int m_second;
      unsigned int m_frame;
    };
  }
#endif
