/* 
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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


#ifndef _K3B_PROJECT_INTERFACE_H_
#define _K3B_PROJECT_INTERFACE_H_

#include <dcopobject.h>
#include <kio/global.h>
#include <qstringlist.h>
//Added by qt3to4:
#include <Q3CString>

namespace K3b {
    class Doc;
}

/**
 * Base class for all project interfaces
 */
namespace K3b {
class ProjectInterface : public DCOPObject
{
  K_DCOP

 public:
  ProjectInterface( Doc* );
  virtual ~ProjectInterface();

  // Generate a name for this interface. Automatically used if name=0 is
  // passed to the constructor
  static Q3CString newIfaceName();

 k_dcop:
  virtual void addUrls( const QStringList& urls );
  virtual void addUrl( const QString& url );

  /**
   * Opens the burn dialog
   */
  virtual void burn();

  /**
   * Starts the burning immedeately
   * \return true if the burning could be started. Be aware that the return
   *         value does not say anything about the success of the burning
   *         process.
   */
  virtual bool directBurn();

  virtual void setBurnDevice( const QString& blockdevicename );

  /**
   * \return the length of the project in blocks (frames).
   */
  virtual int length() const;

  /**
   * \return size of the project in bytes.
   */
  virtual KIO::filesize_t size() const;

  virtual const QString& imagePath() const;

  /**
   * \return A string representation of the project type. One of:
   * \li "data" - Data
   * \li "audiocd" - Audio CD
   * \li "mixedcd" - Mixed Mode CD
   * \li "videocd" - Video CD
   * \li "emovix" - eMovix
   * \li "videodvd" - Video DVD
   *
   * Be aware that this is not the same as Doc::documentType for historical reasons.
   */
  virtual QString projectType() const;

 private:
  Doc* m_doc;
};
}

#endif
