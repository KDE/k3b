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

#include "k3bdebuggerplugin.h"

// the k3b stuff we need
#include <k3bcore.h>
#include <device/k3bdevicemanager.h>
#include <device/k3bdevice.h>
#include <k3binteractiondialog.h>

#include <kdebug.h>
#include <kaction.h>
#include <kinstance.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kfiledialog.h>
#include <ktextbrowser.h>
#include <kgenericfactory.h>

#include <qptrlist.h>
#include <qfile.h>
#include <qtextstream.h>


K3bDebuggerPlugin::K3bDebuggerPlugin( QObject* parent, 
				      const char* name,
				      const QStringList& )
  : KParts::Plugin( parent, name )
{
  (void) new KAction( i18n("&Create debugging output (plugin)"),
		      0, 0,
		      this, SLOT(slotDoDebuggerStuff()),
		      actionCollection(), "k3bdebuggerplugin" );
}


K3bDebuggerPlugin::~K3bDebuggerPlugin()
{
}


void K3bDebuggerPlugin::slotDoDebuggerStuff()
{
  //
  // We create a html page with as much info about the system as possible
  // and show it in a dialog with the possibility to send it as mail or save it
  //

  QString info;
  QTextStream s( &info, IO_WriteOnly );

  s << "<html>" << endl
    << "<body>" << endl
    << "<h1>K3b Debugging Information</h1>" << endl
    << "<h2>Detected Devices</h2>" << endl;

  for( QPtrListIterator<K3bCdDevice::CdDevice> it( k3bcore->deviceManager()->allDevices() );
       it.current(); ++it ) {
    K3bCdDevice::CdDevice* dev = it.current();

    s << "<table>" << endl
      << "<tr><td span=2>" 
      << dev->vendor() << " " 
      << dev->description() << " " 
      << dev->version() << " (" << dev->blockDeviceName() << ")" 
      << "</td></tr>" << endl;

    s << "<tr><td>Mountdevice:</td><td>" << dev->mountDevice() << "</td></tr>" << endl
      << "<tr><td>Mountpoint:</td><td>" << dev->mountPoint() << "</td></tr>" << endl;

    s << "</table>" << endl;
  }

  s << "</body>" << endl
    << "</html>" << endl;


  KDialogBase dlg( 0, "infodlg", true,
		   i18n("K3b Debugging Info"),
		   KDialogBase::Ok );
  KTextBrowser* tb = new KTextBrowser( &dlg );
  tb->setText( info );
  dlg.setMainWidget( tb );

  dlg.exec();
}


K_EXPORT_COMPONENT_FACTORY( libk3bdebuggerplugin, KGenericFactory<K3bDebuggerPlugin> )


#include "k3bdebuggerplugin.moc"
