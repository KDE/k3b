/*
 *
 * $Id: k3bapplication.cpp 567271 2006-07-28 13:19:18Z trueg $
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

#include "k3blsofwrapperdialog.h"
#include "k3blsofwrapper.h"
#include <k3brichtextlabel.h>

#include <k3bdevice.h>

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>

#include <qpushbutton.h>
//Added by qt3to4:
#include <Q3ValueList>

#include <sys/types.h>
#include <signal.h>


static QString joinProcessNames( const Q3ValueList<K3bLsofWrapper::Process>& apps )
{
  QStringList l;
  for( Q3ValueList<K3bLsofWrapper::Process>::const_iterator it = apps.begin();
       it != apps.end(); ++it )
    l.append( (*it).name );
  return l.join( ", " );
}


K3bLsofWrapperDialog::K3bLsofWrapperDialog( QWidget* parent )
  : KDialog( parent)
{
  setCaption(i18n("Device in use"));
  setButtons (Close|User1|User2);
  setDefaultButton(Close);
  setModal(true);
  setButtonText(User1,i18n("Quit the other applications"));
  setButtonText(User2,i18n("Check again"));
  setButtonText( Close, i18n("Continue") );

  m_label = new K3bRichTextLabel( this );
  setMainWidget( m_label );

  connect( this, SIGNAL(user1Clicked()), SLOT(slotQuitOtherApps()) );
  connect( this, SIGNAL(user2Clicked()), SLOT(slotCheckDevice()) );
}


K3bLsofWrapperDialog::~K3bLsofWrapperDialog()
{
}


bool K3bLsofWrapperDialog::slotCheckDevice()
{
  K3bLsofWrapper lsof;
  if( lsof.checkDevice( m_device ) ) {
    const Q3ValueList<K3bLsofWrapper::Process>& apps = lsof.usingApplications();
    if( apps.count() > 0 ) {
      m_label->setText( i18n("<p>Device <b>'%1'</b> is already in use by other applications "
			     "(<em>%2</em>)."
			     "<p>It is highly recommended to quit those before continuing. "
			     "Otherwise K3b might not be able to fully access the device."
			     "<p><em>Hint: Sometimes shutting down an application does not "
			     "happen instantly. In that case you might have to use the '%3' "
			     "button.")
			.arg( m_device->vendor() + " - " + m_device->description() )
			.arg( joinProcessNames(apps) )
			.arg( i18n( "Check again" ) ) );
      return true;
    }
  }

  // once no apps are running we can close the dialog
  close();

  return false;
}


void K3bLsofWrapperDialog::slotQuitOtherApps()
{
  K3bLsofWrapper lsof;
  if( lsof.checkDevice( m_device ) ) {
    const Q3ValueList<K3bLsofWrapper::Process>& apps = lsof.usingApplications();
    if( apps.count() > 0 ) {
      if( KMessageBox::warningYesNo( this,
				     i18n("<p>Do you really want K3b to kill the following processes: <em>")
					  + joinProcessNames(apps) ) == KMessageBox::Yes ) {
	for( Q3ValueList<K3bLsofWrapper::Process>::const_iterator it = apps.begin();
	     it != apps.end(); ++it )
	  ::kill( (*it).pid, SIGTERM );
      }
      else
	return;
    }
  }

  // after quitting the other applications recheck for running ones
  slotCheckDevice();
}


void K3bLsofWrapperDialog::checkDevice( K3bDevice::Device* dev, QWidget* parent )
{
  K3bLsofWrapperDialog dlg( parent );
  dlg.m_device = dev;
  if( dlg.slotCheckDevice() )
    dlg.exec();
}

#include "k3blsofwrapperdialog.moc"
