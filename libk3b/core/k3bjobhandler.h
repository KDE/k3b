/* 
 *
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

#ifndef _K3B_JOB_HANDLER_H_
#define _K3B_JOB_HANDLER_H_


#include <k3bdiskinfo.h>
#include <k3bdevice.h>


/**
 * See @p K3bJobProgressDialog as an example for the usage of
 * the K3bJobHandler interface.
 */
class K3bJobHandler
{
 public:
  K3bJobHandler() {}
  virtual ~K3bJobHandler() {}

  /**
   * \return true if the handler itself is also a job
   */
  virtual bool isJob() const { return false; }

  /**
   * @return K3bDevice::MediaType on success,
   *         0 if forced (no media info available),
   *         and -1 on error (canceled)
   */
  virtual int waitForMedia( K3bDevice::Device*,
			    int mediaState = K3bDevice::STATE_EMPTY,
			    int mediaType = K3bDevice::MEDIA_WRITABLE_CD,
			    const QString& message = QString::null ) = 0;

  // FIXME: use KGuiItem  
  virtual bool questionYesNo( const QString& text,
			      const QString& caption = QString::null,
			      const QString& yesText = QString::null,
			      const QString& noText = QString::null ) = 0;

  /**
   * Use this if you need the user to do something before the job is able to continue.
   * In all other cases an infoMessage should be used.
   */
  virtual void blockingInformation( const QString& text,
				    const QString& caption = QString::null ) = 0;

};

#endif
