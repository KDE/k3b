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

#include <qheader.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qpixmap.h>
#include <qfile.h>

#include <klistview.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kconfig.h>
#include <kapp.h>
#include <kmessagebox.h>


K3bOptionDialog::K3bOptionDialog(QWidget *parent, const char *name, bool modal )
	: KDialogBase( IconList, i18n("Options"), Help|Apply|Ok|Cancel, Ok, parent,name, modal, true)
{

	setupProgramsPage();
	readPrograms();
}


K3bOptionDialog::~K3bOptionDialog()
{
}


void K3bOptionDialog::slotOk()
{
	// save and only exit if all was successful
	bool bAccept = true;
	
	bAccept = bAccept && savePrograms();

	if( bAccept )
		accept();
}

void K3bOptionDialog::slotApply()
{
	// save all the shit!
	savePrograms();
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

    // set the second column renameable
	m_viewPrograms->setItemsRenameable( true );
	m_viewPrograms->setRenameable( 0, false );
	m_viewPrograms->setRenameable( 1, true );

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

    item = new QListViewItem( m_viewPrograms );
    item->setText( 0, "cdrecord"  );
    item->setText( 1, config->readEntry( "cdrecord path", "/usr/bin/cdrecord" ) );

    item = new QListViewItem( m_viewPrograms );
    item->setText( 0, "readcd"  );
    item->setText( 1,  config->readEntry( "readcd path", "/usr/bin/readcd" ) );

    item = new QListViewItem( m_viewPrograms );
    item->setText( 0, "mpg123"  );
    item->setText( 1,  config->readEntry( "mpg123 path", "/usr/bin/mpg123" ) );

    item = new QListViewItem( m_viewPrograms );
    item->setText( 0, "cdda2wav"  );
    item->setText( 1,  config->readEntry( "cdda2wav path", "/usr/bin/cdda2wav" ) );

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
