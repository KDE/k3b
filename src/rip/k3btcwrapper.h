/* 
 *
 * $Id$
 * Copyright (C) 2003 Thomas Froscher <tfroescher@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef K3BTCWRAPPER_H
#define K3BTCWRAPPER_H

#include <qobject.h>
#include <qstring.h>
#include <qstringlist.h>


class KProcess;
class K3bDvdContent;
namespace K3bDevice {
  class Device;
}

/**
  *@author Sebastian Trueg
  */

class K3bTcWrapper : public QObject
{
  Q_OBJECT

 public: 
  K3bTcWrapper( QObject* parent = 0 );
  ~K3bTcWrapper();
  /* Returns true if transcode tools installed
   */
  static bool supportDvd();
  void checkDvdContent( K3bDevice::Device* );
  void isDvdInsert( K3bDevice::Device* device );
  const QValueList<K3bDvdContent>& getDvdTitles() const;

private slots:
  void slotParseTcprobeOutput( KProcess *p, char *text, int index);
  void slotParseTcprobeError( KProcess *p, char *text, int index);
  //void slotParseTcprobeOutput( const QString &text );
  void slotTcprobeExited( KProcess* );

signals:
  void notSupportedDisc();
  void successfulDvdCheck( bool );
  void tcprobeTitleParsed( int );

 private:
  QString m_errorBuffer;
  QString m_outputBuffer;
  typedef QValueList<K3bDvdContent> DvdTitle;
  DvdTitle m_dvdTitles;
  bool m_firstProbeDone;
  // check only one title in runTcProbe, for testing if dvd is inserted
  bool m_runTcProbeCheckOnly;
  int m_currentTitle;
  int m_allTitle;
  int m_allAngle;
  K3bDevice::Device*  m_device;
  K3bDvdContent parseTcprobe();
  void runTcprobe();
};

#endif
