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

#ifndef _K3B_SIMPLE_JOB_HANDLER_H_
#define _K3B_SIMPLE_JOB_HANDLER_H_

#include <k3b_export.h>

#include <qobject.h>
#include <k3bjobhandler.h>


/**
 * This is a simplified job handler which just consumes the
 * job handler calls without doing anything.
 * Use it for very simple jobs that don't need the job handler
 * methods.
 */
class LIBK3B_EXPORT K3bSimpleJobHandler : public QObject, public K3bJobHandler
{
  Q_OBJECT

 public:
  K3bSimpleJobHandler( QObject* parent = 0 );
  ~K3bSimpleJobHandler();

  /*
   * \return 0
   */
  int waitForMedia( K3bDevice::Device*,
		    int mediaState = K3bDevice::STATE_EMPTY,
		    int mediaType = K3bDevice::MEDIA_WRITABLE_CD,
		    const QString& message = QString::null );
  /**
   * \return true
   */
  bool questionYesNo( const QString& text,
		      const QString& caption = QString::null,
		      const QString& yesText = QString::null,
		      const QString& noText = QString::null );

  /**
   * Does nothing
   */
  void blockingInformation( const QString& text,
			    const QString& caption = QString::null );
};

#endif
