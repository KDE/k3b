/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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


#include "k3binteractiondialog.h"
#include "k3btitlelabel.h"
#include "kcutlabel.h"
#include "k3bstdguiitems.h"
#include "k3bpushbutton.h"
#include "k3bthemedheader.h"
#include "k3bthememanager.h"
#include <k3bapplication.h>
#include <k3btoolbutton.h>

#include <qlabel.h>
#include <qpushbutton.h>
#include <qtoolbutton.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qstring.h>
#include <qpoint.h>
#include <qfont.h>
#include <qpopupmenu.h>
#include <qeventloop.h>
#include <qapplication.h>
#include <qtimer.h>

#include <klocale.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <kstdguiitem.h>
#include <kpushbutton.h>
#include <kconfig.h>
#include <kiconloader.h>


K3bInteractionDialog::K3bInteractionDialog( QWidget* parent,
					    const char* name,
					    const QString& title,
					    const QString& subTitle,
					    int buttonMask,
					    int defaultButton,
					    const QString& configGroup,
					    bool modal,
					    WFlags fl )
  : KDialog( parent, name, modal, fl ),
    m_mainWidget(0),
    m_defaultButton(defaultButton),
    m_configGroup(configGroup),
    m_exitLoopOnHide(true),
    m_inLoop(false),
    m_inToggleMode(false),
    m_delayedInit(false)
{
  installEventFilter( this );

  mainGrid = new QGridLayout( this );
  mainGrid->setSpacing( spacingHint() );
  mainGrid->setMargin( marginHint() );

  // header
  // ---------------------------------------------------------------------------------------------------
  m_dialogHeader = new K3bThemedHeader( this );
  mainGrid->addMultiCellWidget( m_dialogHeader, 0, 0, 0, 2 );


  // settings buttons
  // ---------------------------------------------------------------------------------------------------
  if( !m_configGroup.isEmpty() ) {
    QHBoxLayout* layout2 = new QHBoxLayout( 0, 0, spacingHint(), "layout2");
    m_buttonK3bDefaults = new QToolButton( /*i18n("K3b Defaults"), */this, "m_buttonK3bDefaults" );
    ((QToolButton*)m_buttonK3bDefaults)->setIconSet( SmallIconSet( "revert" ) );
    layout2->addWidget( m_buttonK3bDefaults );

    m_buttonUserDefaults = new K3bToolButton( /*i18n("User Defaults"), */this );
    ((K3bToolButton*)m_buttonUserDefaults)->setIconSet( SmallIconSet( "revert" ) );
    QPopupMenu* userDefaultsPopup = new QPopupMenu( m_buttonUserDefaults );
    userDefaultsPopup->insertItem( i18n("Default Settings"), this, SLOT(slotLoadUserDefaults()) );
    userDefaultsPopup->insertItem( i18n("Last Used Settings"), this, SLOT(slotLoadLastSettings()) );
    ((QToolButton*)m_buttonUserDefaults)->setPopup( userDefaultsPopup );
    ((QToolButton*)m_buttonUserDefaults)->setPopupDelay( QApplication::startDragTime() );
    layout2->addWidget( m_buttonUserDefaults );

    m_buttonSaveUserDefaults = new QToolButton( /*i18n("Save User Defaults"), */this, "m_buttonSaveUserDefaults" );
    ((QToolButton*)m_buttonSaveUserDefaults)->setIconSet( SmallIconSet( "filesave" ) );
    layout2->addWidget( m_buttonSaveUserDefaults );

    mainGrid->addLayout( layout2, 2, 0 );
  }

  QSpacerItem* spacer = new QSpacerItem( 10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum );
  mainGrid->addItem( spacer, 2, 1 );


  // action buttons
  // ---------------------------------------------------------------------------------------------------
  QHBoxLayout* layout5 = new QHBoxLayout( 0, 0, spacingHint(), "layout5");

  if( buttonMask & CANCEL_BUTTON ) {
    m_buttonCancel = new KPushButton( KStdGuiItem::close(), this, "m_buttonCancel" );
    layout5->addWidget( m_buttonCancel );
  }
  else
    m_buttonCancel = 0;
  if( buttonMask & SAVE_BUTTON ) {
    m_buttonSave = new KPushButton( KStdGuiItem::save(), this, "m_buttonSave" );
    layout5->addWidget( m_buttonSave );
  }
  else
    m_buttonSave = 0;
  if( buttonMask & START_BUTTON ) {
    KGuiItem startItem = KStdGuiItem::ok();
    startItem.setText( i18n("Start") );
    m_buttonStart = new KPushButton( startItem, this, "m_buttonStart" );
    QFont fnt( m_buttonStart->font() );
    fnt.setBold(true);
    m_buttonStart->setFont( fnt );
    layout5->addWidget( m_buttonStart );
  }
  else
    m_buttonStart = 0;

  mainGrid->addLayout( layout5, 2, 2 );

  mainGrid->setRowStretch( 1, 1 );

  setTitle( title, subTitle );

  initConnections();
  initToolTipsAndWhatsThis();

  setDefaultButton( START_BUTTON );
}

K3bInteractionDialog::~K3bInteractionDialog()
{
}


void K3bInteractionDialog::show()
{
  KDialog::show();
  if( QPushButton* b = getButton( m_defaultButton ) )
    b->setFocus();
}


QSize K3bInteractionDialog::sizeHint() const
{
  QSize s = KDialog::sizeHint();
  // I want the dialogs to look good.
  // That means their height should never outgrow their width
  if( s.height() > s.width() )
    s.setWidth( s.height() );

  return s;
}


void K3bInteractionDialog::initConnections()
{
  if( m_buttonStart ) {
    connect( m_buttonStart, SIGNAL(clicked()),
	     this, SLOT(slotStartClickedInternal()) );
  }
  if( m_buttonSave ) {
//     connect( m_buttonSave, SIGNAL(clicked()),
// 	     this, SLOT(slotSaveLastSettings()) );
    connect( m_buttonSave, SIGNAL(clicked()),
	     this, SLOT(slotSaveClicked()) );
  }
  if( m_buttonCancel )
    connect( m_buttonCancel, SIGNAL(clicked()),
	     this, SLOT(slotCancelClicked()) );

  if( !m_configGroup.isEmpty() ) {
    connect( m_buttonK3bDefaults, SIGNAL(clicked()),
	     this, SLOT(slotLoadK3bDefaults()) );
    connect( m_buttonUserDefaults, SIGNAL(clicked()),
	     this, SLOT(slotLoadUserDefaults()) );
    connect( m_buttonSaveUserDefaults, SIGNAL(clicked()),
	     this, SLOT(slotSaveUserDefaults()) );
  }
}


void K3bInteractionDialog::initToolTipsAndWhatsThis()
{
  if( !m_configGroup.isEmpty() ) {
    // ToolTips
    // -------------------------------------------------------------------------
    QToolTip::add( m_buttonK3bDefaults, i18n("Load K3b default settings") );
    QToolTip::add( m_buttonUserDefaults, i18n("Load user default settings") );
    QToolTip::add( m_buttonSaveUserDefaults, i18n("Save user default settings for new projects") );

    // What's This info
    // -------------------------------------------------------------------------
    QWhatsThis::add( m_buttonK3bDefaults, i18n("<p>This sets all options back to K3b defaults.") );
    QWhatsThis::add( m_buttonUserDefaults, i18n("<p>This loads the settings saved with the <em>Save User Defaults</em> "
						"button.") );
    QWhatsThis::add( m_buttonSaveUserDefaults, i18n("<p>Saves the current settings as the default for all new projects."
						    "<p>These settings can also be loaded with the <em>User Defaults</em> "
						    "button."
						    "<p><b>The K3b defaults are not overwritten by this.</b>") );
  }
}


void K3bInteractionDialog::setTitle( const QString& title, const QString& subTitle )
{
  m_dialogHeader->setTitle( title, subTitle );

  setCaption( title );
}


void K3bInteractionDialog::setMainWidget( QWidget* w )
{
  w->reparent( this, QPoint(0,0) );
  mainGrid->addMultiCellWidget( w, 1, 1, 0, 2 );
  m_mainWidget = w;
}

QWidget* K3bInteractionDialog::mainWidget()
{
  if( !m_mainWidget ) {
    setMainWidget( new QWidget( this ) );
  }
  return m_mainWidget;
}

void K3bInteractionDialog::slotLoadK3bDefaults()
{
  loadK3bDefaults();
}

void K3bInteractionDialog::slotLoadUserDefaults()
{
  KConfigGroup c( k3bcore->config(), m_configGroup );
  loadUserDefaults( &c );
}

void K3bInteractionDialog::slotSaveUserDefaults()
{
  KConfigGroup c( k3bcore->config(), m_configGroup );
  saveUserDefaults( &c );
}


void K3bInteractionDialog::slotLoadLastSettings()
{
  KConfigGroup c( k3bcore->config(), "last used " + m_configGroup );
  loadUserDefaults( &c );
}


void K3bInteractionDialog::saveLastSettings()
{
  KConfigGroup c( k3bcore->config(), "last used " + m_configGroup );
  saveUserDefaults( &c );
}


void K3bInteractionDialog::slotStartClickedInternal()
{
  saveLastSettings();
  slotStartClicked();
}


void K3bInteractionDialog::slotStartClicked()
{
  emit started();
}

void K3bInteractionDialog::slotCancelClicked()
{
  emit canceled();
  close( false );
}

void K3bInteractionDialog::slotSaveClicked()
{
  emit saved();
}


void K3bInteractionDialog::setDefaultButton( int button )
{
  m_defaultButton = button;
  if( QPushButton* b = getButton( button ) )
    b->setDefault( true ); 
}


bool K3bInteractionDialog::eventFilter( QObject* o, QEvent* ev )
{
  if( dynamic_cast<K3bInteractionDialog*>(o) == this &&
      ev->type() == QEvent::KeyPress ) {

    QKeyEvent* kev = dynamic_cast<QKeyEvent*>(ev);

    switch ( kev->key() ) {
    case Key_Enter:
    case Key_Return:
      // if the process finished this closes the dialog
      if( m_defaultButton == START_BUTTON ) {
	if( m_buttonStart->isEnabled() )
	  slotStartClickedInternal();
      }
      else if( m_defaultButton == CANCEL_BUTTON ) {
	if( m_buttonCancel->isEnabled() )
	  slotCancelClicked();
      }
      else if( m_defaultButton == SAVE_BUTTON ) {
	if( m_buttonSave->isEnabled() )
	  slotSaveClicked();
      }
      return true;

    case Key_Escape:
      // simulate button clicks
      if( m_buttonCancel ) {
	if( m_buttonCancel->isEnabled() )
	  slotCancelClicked();
      }
      return true;
    }
  }

  return KDialog::eventFilter( o, ev );
}


QPushButton* K3bInteractionDialog::getButton( int button )
{
  switch( button ) {
  case START_BUTTON:
    return m_buttonStart;
  case SAVE_BUTTON:
    return m_buttonSave;
  case CANCEL_BUTTON:
    return m_buttonCancel;
  default:
    return 0;
  }
}


void K3bInteractionDialog::setButtonText( int button,
					  const QString& text, 
					  const QString& tooltip, 
					  const QString& whatsthis )
{
  if( QPushButton* b = getButton( button ) ) {
    b->setText( text );
    QToolTip::remove( b );
    QWhatsThis::remove( b );
    QToolTip::add( b, tooltip );
    QWhatsThis::add( b, whatsthis );
  }
}


void K3bInteractionDialog::setButtonEnabled( int button, bool enabled )
{
  if( QPushButton* b = getButton( button ) ) {
    b->setEnabled( enabled );
  }
}


void K3bInteractionDialog::setButtonShown( int button, bool shown )
{
  if( QPushButton* b = getButton( button ) ) {
    b->setShown( shown );
  }
}


void K3bInteractionDialog::setStartButtonText( const QString& text,
					       const QString& tooltip,
					       const QString& whatsthis )
{
  if( m_buttonStart ) {
    m_buttonStart->setText( text );
    QToolTip::remove( m_buttonStart );
    QWhatsThis::remove( m_buttonStart );
    QToolTip::add( m_buttonStart, tooltip );
    QWhatsThis::add( m_buttonStart, whatsthis );
  }
}


void K3bInteractionDialog::setCancelButtonText( const QString& text,
						const QString& tooltip,
						const QString& whatsthis )
{
  if( m_buttonCancel ) {
    m_buttonCancel->setText( text );
    QToolTip::remove( m_buttonCancel );
    QWhatsThis::remove( m_buttonCancel );
    QToolTip::add( m_buttonCancel, tooltip );
    QWhatsThis::add( m_buttonCancel, whatsthis );
  }
}


void K3bInteractionDialog::setSaveButtonText( const QString& text,
					      const QString& tooltip,
					      const QString& whatsthis )
{
  if( m_buttonSave ) {
    m_buttonSave->setText( text );
    QToolTip::remove( m_buttonSave );
    QWhatsThis::remove( m_buttonSave );
    QToolTip::add( m_buttonSave, tooltip );
    QWhatsThis::add( m_buttonSave, whatsthis );
  }
}


void K3bInteractionDialog::saveUserDefaults( KConfigBase* )
{
}


void K3bInteractionDialog::loadUserDefaults( KConfigBase* )
{
}


void K3bInteractionDialog::loadK3bDefaults()
{
}


int K3bInteractionDialog::exec()
{
  return exec( true );
}


int K3bInteractionDialog::exec( bool returnOnHide )
{
  m_exitLoopOnHide = returnOnHide;

  // the following code is mainly taken from QDialog::exec

  if( m_inLoop ) {
    kdError() << "(K3bInteractionDialog::exec) Recursive call detected." << endl;
    return -1;
  }
  
  bool destructiveClose = testWFlags( WDestructiveClose );
  clearWFlags( WDestructiveClose );
  
  bool wasShowModal = testWFlags( WShowModal );
  setWFlags( WShowModal );
  setResult( 0 );

  slotLoadUserDefaults();
  show();
  if( m_delayedInit )
    QTimer::singleShot( 0, this, SLOT(slotDelayedInit()) );
  else
    init();
  
  m_inLoop = true;
  QApplication::eventLoop()->enterLoop();
  
  if( !wasShowModal )
    clearWFlags( WShowModal );
  
  int res = result();
  
  if( destructiveClose )
    delete this;
  
  return res;
}


void K3bInteractionDialog::hide()
{
  if( isHidden() )
    return;
  
  KDialog::hide();
  
  if( m_inLoop && m_exitLoopOnHide ) {
    m_inLoop = false;
    QApplication::eventLoop()->exitLoop();
  }
}


bool K3bInteractionDialog::close( bool alsoDelete )
{
  if( m_inLoop && !m_exitLoopOnHide ) {
    m_inLoop = false;
    QApplication::eventLoop()->exitLoop();
  }

  return KDialog::close( alsoDelete );
}


void K3bInteractionDialog::done( int r )
{
  if( m_inLoop && !m_exitLoopOnHide ) {
    m_inLoop = false;
    QApplication::eventLoop()->exitLoop();
  }

  return KDialog::done( r );
}


void K3bInteractionDialog::slotToggleAll()
{
  if( !m_inToggleMode ) {
    m_inToggleMode = true;
    toggleAll();
    m_inToggleMode = false;
  }
}


void K3bInteractionDialog::toggleAll()
{
}


void K3bInteractionDialog::slotDelayedInit()
{
  init();
}

#include "k3binteractiondialog.moc"
