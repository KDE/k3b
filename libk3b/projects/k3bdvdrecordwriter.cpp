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

#include "k3bdvdrecordwriter.h"

#include <k3bcore.h>
#include <k3bexternalbinmanager.h>
#include <k3bprocess.h>
#include <k3bdevice.h>
#include <k3bdevicemanager.h>
#include <k3bglobals.h>
#include <k3bglobalsettings.h>

#include <klocale.h>


K3bDvdrecordWriter::K3bDvdrecordWriter( K3bDevice::Device* dev, QObject* parent, const char* name )
  : K3bCdrecordWriter( dev, parent, name )
{
}


K3bDvdrecordWriter::~K3bDvdrecordWriter()
{
}

void K3bDvdrecordWriter::prepareProcess()
{
  if( m_process ) delete m_process;  // kdelibs want this!
  m_process = new K3bProcess();
  m_process->setRunPrivileged(true);
  m_process->setSplitStdout(true);
  connect( m_process, SIGNAL(stdoutLine(const QString&)), this, SLOT(slotStdLine(const QString&)) );
  connect( m_process, SIGNAL(stderrLine(const QString&)), this, SLOT(slotStdLine(const QString&)) );
  connect( m_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(slotProcessExited(int, QProcess::ExitStatus)) );
  connect( m_process, SIGNAL(wroteStdin(K3Process*)), this, SIGNAL(dataWritten()) );

//   if( k3bcore->externalBinManager()->binObject("cdrecord")->hasFeature( "dvd-patch" ) )
//     m_cdrecordBinObject = k3bcore->externalBinManager()->binObject("cdrecord");
//   else
    m_cdrecordBinObject = k3bcore->externalBinManager()->binObject("dvdrecord");

  if( !m_cdrecordBinObject )
    return;

  *m_process << m_cdrecordBinObject->path;

  // display progress
  *m_process << "-v";
    
  if( m_cdrecordBinObject->hasFeature( "delay") )
    *m_process << "-delay" << "0";
  else if( m_cdrecordBinObject->hasFeature( "gracetime") )
    *m_process << "gracetime=2";  // 2 is the lowest allowed value (Joerg, why do you do this to us?)
   
  // Again we assume the device to be set!
  *m_process << QString("dev=%1").arg(K3b::externalBinDeviceParameter(burnDevice(), m_cdrecordBinObject));
  *m_process << QString("speed=%1").arg(burnSpeed());
  
  // DVDs are only written in DAO mode (and Packet, but we do not support that since it does not
  //                                    make much sense here)  
  *m_process << "-dao";
  setWritingMode( K3b::WRITING_MODE_DAO ); // just to make sure the CdrecordWriter emits the correct messages
    
  if( simulate() )
    *m_process << "-dummy";
    
  if( burnproof() ) {
    if( burnDevice()->burnproof() ) {
      // with cdrecord 1.11a02 burnproof was renamed to burnfree
      // what about dvdrecord??
      if( m_cdrecordBinObject->version < K3bVersion( "1.11a02" ) )
	*m_process << "driveropts=burnproof";
      else
	*m_process << "driveropts=burnfree";
    }
    else
      emit infoMessage( i18n("Writer does not support buffer underrun free recording (BURNPROOF)"), INFO );
  }
  
  if( k3bcore->globalSettings()->ejectMedia() )
    *m_process << "-eject";

  bool manualBufferSize = k3bcore->globalSettings()->manualBufferSize();
  if( manualBufferSize ) {
    *m_process << QString("fs=%1m").arg( k3bcore->globalSettings()->writingBuffer() );
  }
    
  bool overburn = k3bcore->globalSettings()->overburn();
  if( overburn )
    if( m_cdrecordBinObject->hasFeature("overburn") )
      *m_process << "-overburn";
    else
      emit infoMessage( i18n("Cdrecord %1 does not support overburning.",m_cdrecordBinObject->version), INFO );
    
  // additional user parameters from config
  const QStringList& params = m_cdrecordBinObject->userParameters();
  for( QStringList::const_iterator it = params.begin(); it != params.end(); ++it )
    *m_process << *it;

  // add the user parameters
  for( QStringList::const_iterator it = m_arguments.begin(); it != m_arguments.end(); ++it )
    *m_process << *it;
}

#include "k3bdvdrecordwriter.moc"

