/***************************************************************************
                          k3bprojectburndialog.cpp  -  description
                             -------------------
    begin                : Thu May 17 2001
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

#include "k3bprojectburndialog.h"
#include "k3b.h"
#include "k3bdoc.h"

#include <qcombobox.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qtoolbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

#include <klocale.h>
#include <kconfig.h>
#include <kstddirs.h>


K3bProjectBurnDialog::K3bProjectBurnDialog(K3bDoc* doc, QWidget *parent, const char *name, bool modal )
	: KDialogBase( KDialogBase::Tabbed, i18n("Write CD"), User1|Ok|Cancel, Ok, parent, name, modal, true, i18n("Write") )
{
	m_doc = doc;
	
	setButtonBoxOrientation( Vertical );
	
    // --- device group ----------------------------------------------------
    m_groupWriter = new QGroupBox( this, "m_groupWriter" );
    m_groupWriter->setTitle( i18n( "Burning Device" ) );
    m_groupWriter->setColumnLayout(0, Qt::Vertical );
    m_groupWriter->layout()->setSpacing( 0 );
    m_groupWriter->layout()->setMargin( 0 );
    QGridLayout* m_groupWriterLayout = new QGridLayout( m_groupWriter->layout() );
    m_groupWriterLayout->setAlignment( Qt::AlignTop );
    m_groupWriterLayout->setSpacing( spacingHint() );
    m_groupWriterLayout->setMargin( marginHint() );

    QLabel* TextLabel1 = new QLabel( m_groupWriter, "TextLabel1" );
    TextLabel1->setText( i18n( "Burning Speed" ) );

    m_groupWriterLayout->addWidget( TextLabel1, 0, 1 );

    m_comboSpeed = new QComboBox( FALSE, m_groupWriter, "m_comboSpeed" );
    m_comboSpeed->setAutoMask( FALSE );
    m_comboSpeed->setDuplicatesEnabled( FALSE );

    m_groupWriterLayout->addWidget( m_comboSpeed, 1, 1 );

    m_comboWriter = new QComboBox( FALSE, m_groupWriter, "m_comboWriter" );

    m_groupWriterLayout->addWidget( m_comboWriter, 1, 0 );

    QLabel* TextLabel1_2 = new QLabel( m_groupWriter, "TextLabel1_2" );
    TextLabel1_2->setText( i18n( "Device" ) );

    m_groupWriterLayout->addWidget( TextLabel1_2, 0, 0 );

    m_groupWriterLayout->setColStretch( 0 , 1);
    // --------------------------------------------------------- device group ---

    connect( m_comboWriter, SIGNAL(activated(int)), this, SLOT(slotRefreshWriterSpeeds()) );
    connect( m_comboWriter, SIGNAL(activated(int)), this, SIGNAL(writerChanged()) );
}

K3bProjectBurnDialog::~K3bProjectBurnDialog(){
}


int K3bProjectBurnDialog::exec( bool burn )
{
	if( burn )
		actionButton(User1)->show();
	else
		actionButton(User1)->hide();
		
	return QDialog::exec();
}


void K3bProjectBurnDialog::slotOk()
{
	saveSettings();
	done( Saved );
}


void K3bProjectBurnDialog::slotCancel()
{
	done( Canceled );
}

void K3bProjectBurnDialog::slotUser1()
{
	saveSettings();
	done( Burn );
}

void K3bProjectBurnDialog::slotRefreshWriterSpeeds()
{
	if( K3bDevice* _dev = writerDevice() ) {
		// add speeds to combobox
		m_comboSpeed->clear();
		m_comboSpeed->insertItem( "1x" );
		int _speed = 2;
		while( _speed <= _dev->maxWriteSpeed ) {
			m_comboSpeed->insertItem( QString( "%1x" ).arg(_speed) );
			_speed+=2;
		}
	}
}

K3bDevice* K3bProjectBurnDialog::writerDevice() const
{
	QString _s = m_comboWriter->currentText();
	_s.truncate(_s.find('-') );
	QStringList list = QStringList::split(  ',', _s );
	int bus = list[0].toInt();
	int target = list[1].toInt();
	int lun = list[2].toInt();
	
	K3bDevice* _dev =  k3bMain()->deviceManager()->deviceByBus( bus, target, lun );
	if( !_dev )
		qDebug( "(K3bProjectBurnDialog) could not find device" );
		
	return _dev;
}

int K3bProjectBurnDialog::writerSpeed() const
{
	QString _strSpeed = m_comboSpeed->currentText();
	_strSpeed.truncate( _strSpeed.find('x') );
	
	return _strSpeed.toInt();
}

void K3bProjectBurnDialog::readSettings()
{
	// -- read cd-writers ----------------------------------------------
	QList<K3bDevice> _devices = k3bMain()->deviceManager()->burningDevices();
	K3bDevice* _dev = _devices.first();
	while( _dev ) {
		m_comboWriter->insertItem( _dev->device() + " - " + _dev->vendor + " " + _dev->description );
		_dev = _devices.next();
	}
	
	slotRefreshWriterSpeeds();
	
	// -- reading current speed --------------------------------------
	int _index = 0;
	QString _strSpeed = QString::number(m_doc->speed()) + "x";
	
	for( int i = 0; i < m_comboSpeed->count(); i++ )
		if( m_comboSpeed->text( i ) == _strSpeed )
			_index = i;
			
	m_comboSpeed->setCurrentItem( _index );
}
