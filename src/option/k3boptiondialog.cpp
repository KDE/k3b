/***************************************************************************
                          k3boptiondialog.cpp  -  description
                             -------------------
    begin                : Tue Apr 17 2001
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

#include "k3boptiondialog.h"
#include "../k3b.h"
#include "k3bcddboptiontab.h"
#include "k3bdeviceoptiontab.h"
#include "k3bburningoptiontab.h"
#include "k3brippingpatternoptiontab.h"

#include <qheader.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qpixmap.h>
#include <qfile.h>
#include <qlist.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qstringlist.h>
#include <qpoint.h>
#include <qxembed.h>
#include <qwidgetstack.h>
#include <qhbox.h>
#include <qvgroupbox.h>

#include <klistview.h>
#include <klistbox.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kconfig.h>
#include <ksimpleconfig.h>
#include <kapp.h>
#include <kmessagebox.h>
#include <kstddirs.h>
#include <kaction.h>
#include <dcopclient.h>
#include <kprocess.h>
#include <klineedit.h>



// TODO: handle the default-settings

K3bOptionDialog::K3bOptionDialog(QWidget *parent, const char *name, bool modal )
  : KDialogBase( IconList, i18n("Options"), Help|Apply|Ok|Cancel|Default, Ok, parent,name, modal, true)
{
  setupBurningPage();
  setupDevicePage();	
  setupProgramsPage();
  setupCddbPage();
  setupRippingPatternPage();
  //	setupPermissionPage();
	
  readPrograms();
  m_cddbOptionTab->readSettings();
  m_deviceOptionTab->readDevices();
  m_burningOptionTab->readSettings();
  m_rippingPatternOptionTab->readSettings();
  //readGeneralRippingOption();
  // if we don't do this the dialog start really huge
  // because of the label in the device-tab
  resize( 800, 700 );
}


K3bOptionDialog::~K3bOptionDialog()
{
}


void K3bOptionDialog::slotOk()
{
  // save and only exit if all was successful
  bool bAccept = true;
	
  bAccept = bAccept && savePrograms();

  slotApply();
	
  if( bAccept )
    accept();
}

void K3bOptionDialog::slotApply()
{
  // save all the shit!
  savePrograms();
  m_cddbOptionTab->apply();
  m_deviceOptionTab->saveDevices();
  m_burningOptionTab->saveSettings();
  m_rippingPatternOptionTab->apply();
  //applyGeneralRippingOption();

  kapp->config()->sync();
}


void K3bOptionDialog::setupProgramsPage()
{
  QFrame* frame = addPage( i18n("Programs"), i18n("Setup external programs"),
			   KGlobal::instance()->iconLoader()->loadIcon( "gear", KIcon::NoGroup, KIcon::SizeMedium ) );
	
  // add all to frame
  QGridLayout* frameLayout = new QGridLayout( frame );
  frameLayout->setSpacing( spacingHint() );
  frameLayout->setMargin( marginHint() );

  m_viewPrograms = new KListView( frame, "m_viewPrograms" );
  m_viewPrograms->addColumn( i18n( "Program" ) );
  m_viewPrograms->addColumn( i18n( "path" ) );
  m_viewPrograms->addColumn( i18n( "additional parameters" ) );
	
  // set the second column renameable
  m_viewPrograms->setItemsRenameable( true );
  m_viewPrograms->setRenameable( 0, false );
  m_viewPrograms->setRenameable( 1, true );
  m_viewPrograms->setRenameable( 2, true );

  frameLayout->addMultiCellWidget( m_viewPrograms, 1, 1, 0, 1 );

  m_buttonSearch = new QPushButton( frame, "m_buttonSearch" );
  m_buttonSearch->setText( i18n( "Search" ) );

  frameLayout->addWidget( m_buttonSearch, 2, 1 );

  m_labelInfo = new QLabel( frame, "m_labelInfo" );
  m_labelInfo->setText( i18n( "Please specify the paths to the external programs that K3b needs to work properly or press \"Search\" to let K3b search for the programs." ) );
  m_labelInfo->setScaledContents( FALSE );
  m_labelInfo->setAlignment( int( QLabel::WordBreak | QLabel::AlignVCenter | QLabel::AlignLeft ) );

  frameLayout->addMultiCellWidget( m_labelInfo, 0, 0, 0, 1 );
  QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
  frameLayout->addItem( spacer, 2, 0 );

  // TODO: connect the search button to a suitable slot
  m_buttonSearch->setDisabled( true );
}


void K3bOptionDialog::readPrograms()
{
  KConfig* config = kapp->config();
  config->setGroup("External Programs");
	
  // clear the view before adding anything!
  m_viewPrograms->clear();
	
  QListViewItem * item = new QListViewItem( m_viewPrograms );
  item->setText( 0, "mkisofs" );
  item->setText( 1, config->readEntry( "mkisofs path", "/usr/bin/mkisofs" ) );
  item->setText( 2, config->readListEntry( "mkisofs parameters" ).join(" ") );

  item = new QListViewItem( m_viewPrograms );
  item->setText( 0, "cdrecord"  );
  item->setText( 1, config->readEntry( "cdrecord path", "/usr/bin/cdrecord" ) );
  item->setText( 2, config->readListEntry( "cdrecord parameters" ).join(" ") );

  //    item = new QListViewItem( m_viewPrograms );
  //    item->setText( 0, "readcd"  );
  //    item->setText( 1,  config->readEntry( "readcd path", "/usr/bin/readcd" ) );
  //    item->setText( 2,  config->readListEntry( "readcd parameters" ).join(" ") );

  item = new QListViewItem( m_viewPrograms );
  item->setText( 0, "mpg123"  );
  item->setText( 1,  config->readEntry( "mpg123 path", "/usr/bin/mpg123" ) );
  item->setText( 2,  config->readListEntry( "mpg123 parameters" ).join(" ") );

  //    item = new QListViewItem( m_viewPrograms );
  //    item->setText( 0, "cdparanoia"  );
  //    item->setText( 1,  config->readEntry( "cdparanoia path", "/usr/bin/cdparanoia" ) );
  //    item->setText( 2,  config->readListEntry( "cdparanoia parameters" ).join(" ") );

  item = new QListViewItem( m_viewPrograms );
  item->setText( 0, "cdrdao"  );
  item->setText( 1,  config->readEntry( "cdrdao path", "/usr/bin/cdrdao" ) );
  item->setText( 2,  config->readListEntry( "cdrdao parameters" ).join(" ") );

  item = new QListViewItem( m_viewPrograms );
  item->setText( 0, "sox"  );
  item->setText( 1,  config->readEntry( "sox path", "/usr/bin/sox" ) );
  item->setText( 2,  config->readListEntry( "sox parameters" ).join(" ") );
}

bool K3bOptionDialog::savePrograms()
{
  KConfig* config = kapp->config();
  config->setGroup( "External Programs" );
	
  // go through all the items in m_viewPrograms and save the values
  // test if the given path exists and show an error dialog if not!
  QString notFound;
	
  QListViewItem* _item = m_viewPrograms->firstChild();
  while( _item ) {
    if( !QFile::exists( _item->text(1) ) )
      notFound.append( _item->text(1) + "\n" );
    else {
      QString entryName = _item->text(0) + " path";
      config->writeEntry( entryName, _item->text(1) );
      entryName = _item->text(0) + " parameters";
      config->writeEntry( entryName, QStringList::split(' ', _item->text(2)) );
    }
    _item = _item->nextSibling();
  }
  config->sync();
	
  if( !notFound.isEmpty() ) {
    KMessageBox::error(	this, i18n("Could not find these programs: \n%s", notFound), i18n("Error") );
    return false;
  }
	
  return true;
}


void K3bOptionDialog::slotDefault()
{
  switch( activePageIndex() )
    {
    case 0: // device page

      break;
    case 1: // programs page
      QListViewItem* _item = m_viewPrograms->firstChild();
      while( _item ) {
	QString entryName = "/usr/bin/" + _item->text(0);
	_item->setText( 1, entryName );
	_item->setText( 2, "" );
	_item = _item->nextSibling();
      }
      break;
    }
}


void K3bOptionDialog::setupBurningPage()
{
  QFrame* frame = addPage( i18n("Burning"), i18n("Burning Settings"),
			   KGlobal::instance()->iconLoader()->loadIcon( "cdwriter_unmount", KIcon::NoGroup, KIcon::SizeMedium ) );
		
  QGridLayout* _frameLayout = new QGridLayout( frame );
  _frameLayout->setSpacing( 0 );
  _frameLayout->setMargin( 0 );

  m_burningOptionTab = new K3bBurningOptionTab( frame );
  _frameLayout->addWidget( m_burningOptionTab, 0, 0 );
}


void K3bOptionDialog::setupCddbPage()
{
  QFrame* frame = addPage( i18n("CDDB"), i18n("Setup the cddb server"),
			   KGlobal::instance()->iconLoader()->loadIcon( "gear", KIcon::NoGroup, KIcon::SizeMedium ) );

  QGridLayout* mainGrid = new QGridLayout( frame );
  mainGrid->setSpacing(0);
  mainGrid->setMargin(0);
  m_cddbOptionTab = new K3bCddbOptionTab(frame, "cddbpage");
  mainGrid->addWidget( m_cddbOptionTab, 0, 0 );
}


void K3bOptionDialog::setupDevicePage()
{
  QFrame* frame = addPage( i18n("Devices"), i18n("Setup CD Devices"),
			   KGlobal::instance()->iconLoader()->loadIcon( "blockdevice", KIcon::NoGroup, KIcon::SizeMedium ) );

  QHBoxLayout* box = new QHBoxLayout( frame );
  box->setSpacing(0);
  box->setMargin(0);
  m_deviceOptionTab = new K3bDeviceOptionTab( frame, "deviceOptionTab" );
  box->addWidget( m_deviceOptionTab );
}

void K3bOptionDialog::setupRippingPatternPage()
{
  QFrame* frame = addPage( i18n("Ripping"), i18n("Setup Ripping Patterns"),
			   KGlobal::instance()->iconLoader()->loadIcon( "blockdevice", KIcon::NoGroup, KIcon::SizeMedium ) );

  QVBoxLayout* box = new QVBoxLayout( frame );
  box->setSpacing( 0 );
  //KDialog::spacingHint() );
  box->setMargin( 0 ); //KDialog::marginHint() );

  m_rippingPatternOptionTab = new K3bRippingPatternOptionTab( frame, "rippingPatternOptionTab" );
  box->addWidget( m_rippingPatternOptionTab );
  QString album("album");
  m_rippingPatternOptionTab->init( album );
}

