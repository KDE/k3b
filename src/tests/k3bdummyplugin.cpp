/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bdummyplugin.h"

// the k3b stuff we need
#include <k3bcore.h>
#include <device/k3bdevicemanager.h>
#include <device/k3bdevice.h>

#include <kdebug.h>
#include <kaction.h>
#include <kinstance.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kfiledialog.h>

#include <qptrlist.h>
#include <qfile.h>
#include <qtextstream.h>


K3bDummyPlugin::K3bDummyPlugin( QObject* parent, const char* name )
  : KParts::Plugin( parent, name )
{
  (void) new KAction( "&Test KParts plugin stuff (plugin)",
		      0, 0,
		      this, SLOT(slotDoDummyStuff()),
		      actionCollection(), "k3bdummyplugin" );
}


K3bDummyPlugin::~K3bDummyPlugin()
{
}


void K3bDummyPlugin::slotDoDummyStuff()
{
  QString filename = KFileDialog::getSaveFileName();
  if( !filename.isEmpty() ) {

    QFile f( filename );
    if( !f.open( IO_WriteOnly ) ) {
      KMessageBox::error( 0, i18n("Could not open file %1.").arg(filename) );
      return;
    }

    QTextStream s( &f );

    QPtrList<K3bCdDevice::CdDevice>& devices = k3bcore->deviceManager()->allDevices();
    for( QPtrListIterator<K3bCdDevice::CdDevice> it( devices ); it.current(); ++it ) {
      K3bCdDevice::CdDevice* dev = it.current();

      s << dev->vendor() << " " << dev->description() << endl
	<< "----------------------------------" << endl;
    }
  }
}



KPluginFactory::KPluginFactory( QObject* parent, const char* name )
  : KLibFactory( parent, name )
{
  s_instance = new KInstance("KPluginFactory");
}

QObject* KPluginFactory::createObject( QObject* parent, const char* name, const char*, const QStringList & )
{
  return new K3bDummyPlugin( parent, name );
}


K_EXPORT_COMPONENT_FACTORY( libk3bdummyplugin, KPluginFactory )

KInstance* KPluginFactory::s_instance = 0L;

#include "k3bdummyplugin.moc"
