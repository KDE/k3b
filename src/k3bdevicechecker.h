/***************************************************************************
                          k3bdevicechecker.h  -  description
                             -------------------
    begin                : Sun Sep 23 2001
    copyright            : (C) 2001 by Sebastian Trueg
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

#ifndef K3BDEVICECHECKER_H
#define K3BDEVICECHECKER_H

#include <qobject.h>
#include <qlist.h>
#include <qstring.h>


class KProcess;
class ScsiIf;
class K3bDevice;
class K3bScsiBusId;



/**
* Checks a given device for information.
* The following information will be detected:
* <p>
* <li>Scsi ids bus, target and lun</li>
* <li>Read / write speed</li>
* <li>Burnproof capabilites</li>
* <li>Drive information (product, vendor and version)</li>
* @author Thomas Froescher
*/
class K3bDeviceChecker : public QObject 
{
  Q_OBJECT

 public:
  /**
   * Scans the system for the scsi ids of all devices. This information will be added to the
   * current device information.
   * See @ref scanDevice()
   */
  K3bDeviceChecker();

  /**
   * Cleans the device and system scan information.
   */
  ~K3bDeviceChecker();

  /**
   * Gets information about a scanned device.
   */
  K3bDevice *getCurrentDevice();

  /**
   * Scans a device for the read and write speed and burnproof capabilities.
   * @param dev The device to check for further information.
   * @param showErrorMsg The level of the error messages.
   */
  int scanDevice( const char *dev, int showErrorMsg = 1 );

 protected slots:
  void parseCdrecordOutput( KProcess * p, char *output, int len );

 private:
  ScsiIf *m_scsiIf;
  K3bDevice *m_currentDevice;
  QList< K3bScsiBusId > m_scsiBusIds;

  K3bScsiBusId *getScsiIds( QString product );

  /**
   * Gets the scsibus information bus, target and lun.
   */
  int scanbus();

  /**
   * Simply copied from the cdrdao code.
   */
  int getModePage( int pageCode, unsigned char *buf, long bufLen,
		   unsigned char *modePageHeader,
		   unsigned char *blockDesc, int showErrorMsgvoid );

  /**
   * Simply copied from the cdrdao code.
   */
  int sendCmd( const unsigned char *cmd, int cmdLen,
	       const unsigned char *dataOut, int dataOutLen,
	       unsigned char *dataIn, int dataInLen, int showErrorMsg );
};

#endif
