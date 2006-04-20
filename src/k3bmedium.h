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

#ifndef _K3B_MEDIUM_H_
#define _K3B_MEDIUM_H_

#include <k3bdiskinfo.h>
#include <k3btoc.h>
#include <k3bcdtext.h>
#include <k3bdevice.h>

class K3bIso9660SimplePrimaryDescriptor;


class K3bMedium
{
 public:
  K3bMedium();
  K3bMedium( K3bDevice::Device* dev );
  ~K3bMedium();

  void setDevice( K3bDevice::Device* dev ) { m_device = dev; reset(); }

  /**
   * Resets everything to default values except the device.
   * This means empty toc, cd text, no writing speeds, and a diskinfo
   * with state UNKNOWN.
   */
  void reset();

  /**
   * Updates the medium information if the device is not null.
   * Do not use this in the GUI thread since it uses blocking
   * K3bdevice methods.
   */
  void update();

  K3bDevice::Device* device() const { return m_device; }

  const K3bDevice::DiskInfo& diskInfo() const { return m_diskInfo; }
  const K3bDevice::Toc& toc() const { return m_toc; }
  const K3bDevice::CdText& cdText() const { return m_cdText; }
  const QValueList<int>& writingSpeeds() const { return m_writingSpeeds; }
  const QString& volumeId() const;

  /**
   * This method tries to make a volume identificator witch uses a reduced character set
   * look more beautiful by, for example, replacing '_' with a space or replacing all upper
   * case words.
   *
   * Volume ids already containing spaces or lower case characters are left unchanged.
   */
  QString beautifiedVolumeId() const;

  enum MediumContent {
    CONTENT_NONE = 0x0,
    CONTENT_AUDIO = 0x1,
    CONTENT_DATA = 0x2,
    CONTENT_VIDEO_CD = 0x4,
    CONTENT_VIDEO_DVD = 0x8,
    CONTENT_ALL = 0xFF
  };

  /**
   * \return a bitwise combination of MediumContent.
   * A VideoCD for example may have the following content: 
   * CONTENT_AUDIO|CONTENT_DATA|CONTENT_VIDEO_CD
   */
  int content() const { return m_content; }

  /**
   * \return The volume descriptor from the ISO9660 filesystem or 0
   *         if the medium does not contain an ISO9660 filesystem.
   */
  const K3bIso9660SimplePrimaryDescriptor* iso9660Descriptor() const { return m_isoDesc; }

  /**
   * \return A short one-liner string representing the medium.
   *         This string may be used for labels or selection boxes.
   * \param useContent if true the content of the CD/DVD will be used, otherwise
   *                   the string will simply be something like "empty DVD-R medium".
   */
  QString shortString( bool useContent = true ) const;

  /**
   * \return A HTML formatted string decribing this medium. This includes the device, the
   *         medium type, the contents type, and some detail information like the number of
   *         tracks.
   *         This string may be used for tooltips or short descriptions.
   */
  QString longString() const;

 private:
  void analyseContent();

  K3bDevice::Device* m_device;

  K3bDevice::DiskInfo m_diskInfo;
  K3bDevice::Toc m_toc;
  K3bDevice::CdText m_cdText;
  QValueList<int> m_writingSpeeds;
  K3bIso9660SimplePrimaryDescriptor* m_isoDesc;

  int m_content;
};

#endif
