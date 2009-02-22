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


#ifndef _K3B_INTERFACE_H_
#define _K3B_INTERFACE_H_

#include <q3valuelist.h>

#include <kurl.h>

namespace K3b {
    class MainWindow;
}
namespace Device {
  class Device;
}


namespace K3b {
class Interface : public DCOPObject
{
  K_DCOP

 public:
  Interface();

  void setMainWindow( MainWindow* mw ) { m_main = mw; }

 k_dcop:
  /**
   * returns a DCOPRef to a ProjectInterface
   */
  DCOPRef createDataProject();
  DCOPRef createDataCDProject();
  DCOPRef createAudioCDProject();
  DCOPRef createMixedCDProject();
  DCOPRef createVideoCDProject();
  DCOPRef createMovixProject();
  DCOPRef createMovixCDProject();
  DCOPRef createDataDVDProject();
  DCOPRef createVideoDVDProject();
  DCOPRef createMovixDVDProject();

  /**
   * Returns a reference to the currently active project.
   * This is useful to do things like:
   *
   * <pre>k3b --audiocd</pre>
   * and then use dcop on the newly created project via:
   * <pre>dcop $(dcop k3b Interface currentProject) something</pre>
   */
  DCOPRef currentProject();

  DCOPRef openProject( const KUrl& url );

  QList<DCOPRef> projects();

  void copyMedium();
  void copyCd();
  void copyDvd();
  void copyMedium( const KUrl& dev );
  void copyCd( const KUrl& dev );
  void copyDvd( const KUrl& dev );
  void formatMedium();
  void eraseCdrw();
  void formatDvd();
  void burnCdImage( const KUrl& url );
  void burnDvdImage( const KUrl& url );

  /**
   * Open the audio ripping window for the specified device.
   */
  void cddaRip( const KUrl& dev );
  
  /**
   * Add URLs to the current active project.
   * If no project is open a new Audio or Data CD
   * project will be created depending on the type
   * of the first file.
   */
  void addUrls( const KUrl::List& urls );
  void addUrl( const KUrl& url );

  /**
   * @return true if currently some job is running.
   */
  bool blocked() const;

 private:
  MainWindow* m_main;

  Device::Device* m_lastDevice;
};
}

#endif
