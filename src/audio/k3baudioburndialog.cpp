/***************************************************************************
                          k3baudioburndialog.cpp  -  description
                             -------------------
    begin                : Sun Apr 1 2001
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

#include "k3baudioburndialog.h"
#include "../k3b.h"
#include "k3baudiodoc.h"
#include "../k3bdevicemanager.h"

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qmultilineedit.h>
#include <qpushbutton.h>
#include <qtabwidget.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qgrid.h>
#include <qtoolbutton.h>
#include <qlist.h>
#include <qstringlist.h>

#include <klocale.h>
#include <kfiledialog.h>
#include <kstddirs.h>

K3bAudioBurnDialog::K3bAudioBurnDialog(K3bAudioDoc* _doc, QWidget *parent, const char *name, bool modal )
	: KDialogBase( KDialogBase::Tabbed, i18n("Write Audio CD"), User1|Ok|Cancel, Ok, parent, name, modal, true, i18n("Write") )
{
	doc = _doc;
	
	setButtonBoxOrientation( Vertical );
	setupBurnTab( addPage( i18n("Burning") ) );
	setupCdTextTab( addPage( i18n("CD-Text") ) );
	
	readSettings();
	
	connect( m_comboWriter, SIGNAL(activated(int)), this, SLOT(slotRefreshWriterSpeeds()) );
}

K3bAudioBurnDialog::~K3bAudioBurnDialog(){
}


void K3bAudioBurnDialog::saveSettings()
{
	// save temp dir
	k3bMain()->config()->setGroup( "General Options" );
	k3bMain()->config()->writeEntry( "Temp Dir", m_editDirectory->text() );
	k3bMain()->config()->sync();

	doc->setDao( m_checkDao->isChecked() );
	doc->setDummy( m_checkSimulate->isChecked() );
	doc->setPadding( m_checkPadding->isChecked() );
	((K3bAudioDoc*)doc)->writeCdText( m_checkCdText->isChecked() );
	
	// -- saving current speed --------------------------------------
	QString _strSpeed = m_comboSpeed->currentText();
	_strSpeed.truncate( _strSpeed.find('x') );
	doc->setSpeed( _strSpeed.toInt() );
	
	// -- saving current device --------------------------------------
	QString _str = m_comboWriter->currentText();
	_str.truncate( _str.find('-') );
	QStringList list = QStringList::split( ',', _str );
	int bus = list[0].toInt();
	int target = list[1].toInt();
	int lun = list[2].toInt();
	
	doc->setBurner( k3bMain()->deviceManager()->deviceByBus( bus, target, lun ) );
	
	// -- save Cd-Text ------------------------------------------------
	((K3bAudioDoc*)doc)->setTitle( m_editTitle->text() );
	((K3bAudioDoc*)doc)->setArtist( m_editArtist->text() );
	((K3bAudioDoc*)doc)->setISRC( m_editISRC->text() );
	((K3bAudioDoc*)doc)->setArranger( m_editArranger->text() );
}


void K3bAudioBurnDialog::slotUser1()
{
	saveSettings();
	done( Burn );
}


int K3bAudioBurnDialog::exec( bool burn )
{
	if( burn )
		actionButton(User1)->show();
	else
		actionButton(User1)->hide();
		
	return QDialog::exec();
}


void K3bAudioBurnDialog::slotOk()
{
	saveSettings();
	done( Saved );
}

void K3bAudioBurnDialog::slotCancel()
{
	done( Canceled );
}


void K3bAudioBurnDialog::readSettings()
{
	// read temp dir
	k3bMain()->config()->setGroup( "General Options" );
	m_editDirectory->setText( k3bMain()->config()->readEntry( "Temp Dir", locateLocal( "appdata", "temp/" ) ) );
	
	m_checkDao->setChecked( doc->dao() );
	m_checkSimulate->setChecked( doc->dummy() );
	m_checkPadding->setChecked( doc->padding() );
	m_checkCdText->setChecked( ((K3bAudioDoc*)doc)->cdText() );

	// -- reading current speed --------------------------------------
	int _index = 0;
	QString _strSpeed = QString::number(doc->speed()) + "x";
	
	for( int i = 0; i < m_comboSpeed->count(); i++ )
		if( m_comboSpeed->text( i ) == _strSpeed )
			_index = i;
			
	m_comboSpeed->setCurrentItem( _index );
	
	// -- read cd-writers ----------------------------------------------
	QList<K3bDevice> _devices = k3bMain()->deviceManager()->burningDevices();
	K3bDevice* _dev = _devices.first();
	while( _dev ) {
		m_comboWriter->insertItem( _dev->device() + " - " + _dev->vendor + " " + _dev->description );
		_dev = _devices.next();
	}
	
	slotRefreshWriterSpeeds();
	
	// read CD-Text ------------------------------------------------------------
	m_editTitle->setText( ((K3bAudioDoc*)doc)->title() );
	m_editArtist->setText( ((K3bAudioDoc*)doc)->artist() );
	m_editISRC->setText( ((K3bAudioDoc*)doc)->isrc() );
	m_editArranger->setText( ((K3bAudioDoc*)doc)->arranger() );
}


void K3bAudioBurnDialog::setupBurnTab( QFrame* frame )
{
    QGridLayout* frameLayout = new QGridLayout( frame );
    frameLayout->setSpacing( spacingHint() );
    frameLayout->setMargin( marginHint() );

    // --- temp group ---------------------------------------------------------
    QGroupBox* m_groupTempDir = new QGroupBox( frame, "m_groupTempDir" );
    m_groupTempDir->setTitle( i18n( "Temp Directory" ) );
    m_groupTempDir->setColumnLayout(0, Qt::Vertical );
    m_groupTempDir->layout()->setSpacing( 0 );
    m_groupTempDir->layout()->setMargin( 0 );
    QGridLayout* m_groupTempDirLayout = new QGridLayout( m_groupTempDir->layout() );
    m_groupTempDirLayout->setAlignment( Qt::AlignTop );
    m_groupTempDirLayout->setSpacing( spacingHint() );
    m_groupTempDirLayout->setMargin( marginHint() );

    QLabel* TextLabel1_3 = new QLabel( m_groupTempDir, "TextLabel1_3" );
    TextLabel1_3->setText( i18n( "Files are buffered in" ) );

    m_groupTempDirLayout->addWidget( TextLabel1_3, 0, 0 );

    QLabel* TextLabel2 = new QLabel( m_groupTempDir, "TextLabel2" );
    TextLabel2->setText( i18n( "Space free on device" ) );

    m_groupTempDirLayout->addWidget( TextLabel2, 2, 0 );

    QLabel* TextLabel4 = new QLabel( m_groupTempDir, "TextLabel4" );
    TextLabel4->setText( i18n( "Size of CD" ) );

    m_groupTempDirLayout->addWidget( TextLabel4, 3, 0 );

    m_labelCdSize = new QLabel( m_groupTempDir, "m_labelCdSize" );
    m_labelCdSize->setText( i18n( "0 MB" ) );
    m_labelCdSize->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );

    m_groupTempDirLayout->addMultiCellWidget( m_labelCdSize, 3, 3, 1, 2 );

    m_labelFreeSpace = new QLabel( m_groupTempDir, "m_labelFreeSpace" );
    m_labelFreeSpace->setText( i18n( "0.0 MB" ) );
    m_labelFreeSpace->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );

    m_groupTempDirLayout->addMultiCellWidget( m_labelFreeSpace, 2, 2, 1, 2 );

    m_editDirectory = new QLineEdit( m_groupTempDir, "m_editDirectory" );

    m_groupTempDirLayout->addMultiCellWidget( m_editDirectory, 1, 1, 0, 1 );

    m_buttonFindDir = new QToolButton( m_groupTempDir, "m_buttonFindDir" );
    m_buttonFindDir->setText( i18n( "..." ) );
    QToolTip::add(  m_buttonFindDir, i18n( "Find directory..." ) );

    m_groupTempDirLayout->addWidget( m_buttonFindDir, 1, 2 );
    // -------------------------------------------------- temp group ---


    // ---- options group ------------------------------------------------
    QGroupBox* m_groupOptions = new QGroupBox( frame, "m_groupOptions" );
    m_groupOptions->setTitle( i18n( "Options" ) );
    m_groupOptions->setColumnLayout(0, Qt::Vertical );
    m_groupOptions->layout()->setSpacing( 0 );
    m_groupOptions->layout()->setMargin( 0 );
    QVBoxLayout* m_groupOptionsLayout = new QVBoxLayout( m_groupOptions->layout() );
    m_groupOptionsLayout->setAlignment( Qt::AlignTop );
    m_groupOptionsLayout->setSpacing( spacingHint() );
    m_groupOptionsLayout->setMargin( marginHint() );

    m_checkCdText = new QCheckBox( m_groupOptions, "m_checkCdText" );
    m_checkCdText->setText( i18n( "Write CD-Text" ) );

    m_checkDao = new QCheckBox( m_groupOptions, "m_checkDao" );
    m_checkDao->setText( i18n( "DiscAtOnce" ) );

    m_checkSimulate = new QCheckBox( m_groupOptions, "m_checkSimulate" );
    m_checkSimulate->setText( i18n( "Simulate Writing" ) );

//    m_checkOnTheFly = new QCheckBox( m_groupOptions, "m_checkOnTheFly" );
//    m_checkOnTheFly->setText( i18n( "Writing on the fly" ) );
//    m_groupOptionsLayout->addWidget( m_checkOnTheFly );

    m_checkPadding = new QCheckBox( m_groupOptions, "m_checkPadding" );
    m_checkPadding->setText( i18n( "Use Padding" ) );

    m_groupOptionsLayout->addWidget( m_checkSimulate );
    m_groupOptionsLayout->addWidget( m_checkCdText );
    m_groupOptionsLayout->addWidget( m_checkPadding );
    m_groupOptionsLayout->addWidget( m_checkDao );
    // --------------------------------------------------- options group ---

    // --- device group ----------------------------------------------------
    QGroupBox* m_groupDevice = new QGroupBox( frame, "m_groupDevice" );
    m_groupDevice->setTitle( i18n( "Burning Device" ) );
    m_groupDevice->setColumnLayout(0, Qt::Vertical );
    m_groupDevice->layout()->setSpacing( 0 );
    m_groupDevice->layout()->setMargin( 0 );
    QGridLayout* m_groupDeviceLayout = new QGridLayout( m_groupDevice->layout() );
    m_groupDeviceLayout->setAlignment( Qt::AlignTop );
    m_groupDeviceLayout->setSpacing( spacingHint() );
    m_groupDeviceLayout->setMargin( marginHint() );

    QLabel* TextLabel1 = new QLabel( m_groupDevice, "TextLabel1" );
    TextLabel1->setText( i18n( "Burning Speed" ) );

    m_groupDeviceLayout->addWidget( TextLabel1, 0, 1 );

    m_comboSpeed = new QComboBox( FALSE, m_groupDevice, "m_comboSpeed" );
    m_comboSpeed->setAutoMask( FALSE );
    m_comboSpeed->setDuplicatesEnabled( FALSE );

    m_groupDeviceLayout->addWidget( m_comboSpeed, 1, 1 );

    m_comboWriter = new QComboBox( FALSE, m_groupDevice, "m_comboWriter" );

    m_groupDeviceLayout->addWidget( m_comboWriter, 1, 0 );

    QLabel* TextLabel1_2 = new QLabel( m_groupDevice, "TextLabel1_2" );
    TextLabel1_2->setText( i18n( "Device" ) );

    m_groupDeviceLayout->addWidget( TextLabel1_2, 0, 0 );
    // --------------------------------------------------------- device group ---


//    // --- actions group --------------------------------------------------------
//    QGroupBox* m_groupActions = new QGroupBox( frame, "m_groupActions" );
//    m_groupActions->setLineWidth( 0 );
//    m_groupActions->setTitle( i18n( "Actions to perform" ) );
//    m_groupActions->setColumnLayout(0, Qt::Vertical );
//    m_groupActions->layout()->setSpacing( 0 );
//    m_groupActions->layout()->setMargin( 0 );
//    QVBoxLayout* m_groupActionsLayout = new QVBoxLayout( m_groupActions->layout() );
//    m_groupActionsLayout->setAlignment( Qt::AlignTop );
//    m_groupActionsLayout->setSpacing( spacingHint() );
//    m_groupActionsLayout->setMargin( marginHint() );
//
//    m_checkBufferFiles = new QCheckBox( m_groupActions, "m_checkBufferFiles" );
//    m_checkBufferFiles->setText( i18n( "Cache Files" ) );
//    m_groupActionsLayout->addWidget( m_checkBufferFiles );
//
//
//    m_checkWrite = new QCheckBox( m_groupActions, "m_checkWrite" );
//    m_checkWrite->setText( i18n( "Write" ) );
//    m_groupActionsLayout->addWidget( m_checkWrite );
//    // ---------------------------------------------------------- actions group ---


//    frameLayout->addMultiCellWidget( m_groupActions, 0, 1, 0, 0 );
    frameLayout->addWidget( m_groupTempDir, 1, 1 );
    frameLayout->addWidget( m_groupOptions, 1, 0 );
    frameLayout->addMultiCellWidget( m_groupDevice, 0, 0, 0, 1 );

    m_groupTempDirLayout->setColStretch( 1 , 1);
    m_groupDeviceLayout->setColStretch( 0 , 1);
	frameLayout->setRowStretch( 1, 1 );
	frameLayout->setColStretch( 1, 1 );

    // tab order

    connect( m_buttonFindDir, SIGNAL(clicked()), this, SLOT(slotFindDir()) );
}


void K3bAudioBurnDialog::setupCdTextTab( QFrame* frame )
{
    QGridLayout* frameLayout = new QGridLayout( frame );
    frameLayout->setSpacing( spacingHint() );
    frameLayout->setMargin( marginHint() );

    QGridLayout* layout1 = new QGridLayout;
    layout1->setSpacing( spacingHint() );
    layout1->setMargin( 0 );

    m_editArranger = new QLineEdit( frame, "m_editArranger" );

    layout1->addWidget( m_editArranger, 2, 1 );

    m_editISRC = new QLineEdit( frame, "m_editISRC" );
    QToolTip::add(  m_editISRC, i18n( "International Standard Recording Code" ) );

    layout1->addWidget( m_editISRC, 3, 1 );

    m_editArtist = new QLineEdit( frame, "m_editArtist" );

    layout1->addWidget( m_editArtist, 1, 1 );

    m_editTitle = new QLineEdit( frame, "m_editTitle" );

    layout1->addWidget( m_editTitle, 0, 1 );

    QLabel* TextLabel5 = new QLabel( frame, "TextLabel5" );
    TextLabel5->setText( i18n( "Arranger" ) );

    layout1->addWidget( TextLabel5, 2, 0 );

    QLabel* TextLabel2 = new QLabel( frame, "TextLabel2" );
    TextLabel2->setText( i18n( "Artist" ) );

    layout1->addWidget( TextLabel2, 1, 0 );

    QLabel* TextLabel7 = new QLabel( frame, "TextLabel7" );
    TextLabel7->setText( i18n( "ISRC" ) );

    layout1->addWidget( TextLabel7, 3, 0 );

    QLabel* TextLabel1 = new QLabel( frame, "TextLabel1" );
    TextLabel1->setText( i18n( "Title" ) );

    layout1->addWidget( TextLabel1, 0, 0 );

    frameLayout->addMultiCellLayout( layout1, 0, 1, 0, 0 );
    QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
    frameLayout->addItem( spacer, 2, 0 );

    QLabel* TextLabel6 = new QLabel( frame, "TextLabel6" );
    TextLabel6->setText( i18n( "Message" ) );
    TextLabel6->setAlignment( int( QLabel::AlignTop | QLabel::AlignLeft ) );

    frameLayout->addWidget( TextLabel6, 0, 2 );

    m_editMessage = new QMultiLineEdit( frame, "m_editMessage" );
    m_editMessage->setWordWrap( QMultiLineEdit::WidgetWidth );

    frameLayout->addMultiCellWidget( m_editMessage, 0, 2, 3, 3 );
    QSpacerItem* spacer_2 = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
    frameLayout->addMultiCell( spacer_2, 1, 2, 2, 2 );

    QFrame* Line1 = new QFrame( frame, "Line1" );
    Line1->setMargin( 0 );
    Line1->setFrameStyle( QFrame::VLine | QFrame::Sunken );

    frameLayout->addMultiCellWidget( Line1, 0, 2, 1, 1 );
}


void K3bAudioBurnDialog::slotRefreshWriterSpeeds()
{
	QString _s = m_comboWriter->currentText();
	_s.truncate(_s.find('-') );
	QStringList list = QStringList::split(  ',', _s );
	int bus = list[0].toInt();
	int target = list[1].toInt();
	int lun = list[2].toInt();
	K3bDevice* _dev =  k3bMain()->deviceManager()->deviceByBus( bus, target, lun );
	
	// add speeds to combobox
	m_comboSpeed->clear();
	m_comboSpeed->insertItem( "1x" );
	int _speed = 2;
	while( _speed <= _dev->maxWriteSpeed ) {
		m_comboSpeed->insertItem( QString( "%1x" ).arg(_speed) );
		_speed+=2;
	}
}


void K3bAudioBurnDialog::slotFindDir()
{
	QString dir = KFileDialog::getExistingDirectory( m_editDirectory->text(), k3bMain(), "Select Temp Directory" );
	if( !dir.isEmpty() ) {
		m_editDirectory->setText( dir );
	}
}
