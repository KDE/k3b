/*
 *
 * $Id$
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


#include "k3binteractiondialog.h"
#include "k3btitlelabel.h"
#include "k3bstdguiitems.h"
#include "k3bpushbutton.h"
#include "k3bthemedheader.h"
#include "k3bthememanager.h"
#include <k3bapplication.h>
#include <k3bmultichoicedialog.h>

#include <qlabel.h>
#include <qtoolbutton.h>
#include <qlayout.h>
#include <qtooltip.h>

#include <qstring.h>
#include <qpoint.h>
#include <qfont.h>
#include <q3popupmenu.h>
#include <qeventloop.h>
#include <qapplication.h>
#include <qtimer.h>

#include <QGridLayout>
#include <QKeyEvent>
#include <QEvent>
#include <Q3HBoxLayout>
#include <QDialogButtonBox>

#include <klocale.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <kstdguiitem.h>
#include <kpushbutton.h>
#include <kconfig.h>
#include <kiconloader.h>
#include <kglobalsettings.h>


K3bInteractionDialog::K3bInteractionDialog( QWidget* parent,
					    const QString& title,
					    const QString& subTitle,
					    int buttonMask,
					    int defaultButton,
					    const QString& configGroup )
  : KDialog( parent ),
    m_mainWidget(0),
    m_defaultButton(defaultButton),
    m_configGroup(configGroup),
    m_exitLoopOnHide(true),
    m_inLoop(false),
    m_inToggleMode(false),
    m_delayedInit(false)
{
  installEventFilter( this );
  setButtons(KDialog::None);

  mainGrid = new QGridLayout;
  QWidget *widget = new QWidget(this);
  setMainWidget( widget );

  mainGrid->setSpacing( spacingHint() );
  mainGrid->setMargin( marginHint() );

  // header
  // ---------------------------------------------------------------------------------------------------
  m_dialogHeader = new K3bThemedHeader( mainWidget() );
  mainGrid->addMultiCellWidget( m_dialogHeader, 0, 0, 0, 2 );


  // settings buttons
  // ---------------------------------------------------------------------------------------------------
  if( !m_configGroup.isEmpty() ) {
    Q3HBoxLayout* layout2 = new Q3HBoxLayout( 0, 0, spacingHint(), "layout2");
    m_buttonLoadSettings = new QToolButton( /*i18n("User Defaults"), */mainWidget() );
    m_buttonLoadSettings->setIconSet( SmallIconSet( "document-revert" ) );
    m_buttonLoadSettings->setPopupMode( QToolButton::MenuButtonPopup );
    Q3PopupMenu* userDefaultsPopup = new Q3PopupMenu( m_buttonLoadSettings );
    userDefaultsPopup->insertItem( i18n("Load default settings"), this, SLOT(slotLoadK3bDefaults()) );
    userDefaultsPopup->insertItem( i18n("Load saved settings"), this, SLOT(slotLoadUserDefaults()) );
    userDefaultsPopup->insertItem( i18n("Load last used settings"), this, SLOT(slotLoadLastSettings()) );
    m_buttonLoadSettings->setMenu( userDefaultsPopup );
    layout2->addWidget( m_buttonLoadSettings );

    m_buttonSaveSettings = new QToolButton( /*i18n("Save User Defaults"), */mainWidget(), "m_buttonSaveSettings" );
    m_buttonSaveSettings->setIconSet( SmallIconSet( "document-save" ) );
    layout2->addWidget( m_buttonSaveSettings );

    mainGrid->addLayout( layout2, 2, 0 );
  }

  QSpacerItem* spacer = new QSpacerItem( 10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum );
  mainGrid->addItem( spacer, 2, 1 );


  // action buttons
  // ---------------------------------------------------------------------------------------------------
  //Q3HBoxLayout* layout5 = new Q3HBoxLayout( 0, 0, spacingHint(), "layout5");
  QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
  if( buttonMask & START_BUTTON ) {
    KGuiItem startItem = KStandardGuiItem::ok();
    m_buttonStart = new KPushButton( startItem, mainWidget() );
    m_buttonStart->setObjectName( "m_buttonStart" );
    // refine the button text
    setButtonText( START_BUTTON,
                   i18n("Start"),
                   i18n("Start the task") );
    QFont fnt( m_buttonStart->font() );
    fnt.setBold(true);
    m_buttonStart->setFont( fnt );
    buttonBox->addButton(m_buttonStart,QDialogButtonBox::ActionRole); 
  }
  if( buttonMask & SAVE_BUTTON ) {
    m_buttonSave = new KPushButton( KStandardGuiItem::save(), mainWidget() );
    buttonBox->addButton(m_buttonSave,QDialogButtonBox::AcceptRole);
  }
  else
    m_buttonSave = 0;
  if( buttonMask & CANCEL_BUTTON ) {
    m_buttonCancel = new KPushButton( KConfigGroup( k3bcore->config(), "General Options" )
                                      .readEntry( "keep action dialogs open", false )
                                      ? KStandardGuiItem::close()
                                      : KStandardGuiItem::cancel(),
                                      mainWidget());
   buttonBox->addButton(m_buttonCancel,QDialogButtonBox::RejectRole);
  }
  else
    m_buttonCancel = 0;

  mainGrid->addWidget(buttonBox,2,2);
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
  if( KPushButton* b = getButton( m_defaultButton ) )
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
    connect( m_buttonSaveSettings, SIGNAL(clicked()),
	     this, SLOT(slotSaveUserDefaults()) );
  }
}


void K3bInteractionDialog::initToolTipsAndWhatsThis()
{
  if( !m_configGroup.isEmpty() ) {
    // ToolTips
    // -------------------------------------------------------------------------
    m_buttonLoadSettings->setToolTip( i18n("Load default or saved settings") );
    m_buttonSaveSettings->setToolTip( i18n("Save current settings to reuse them later") );

    // What's This info
    // -------------------------------------------------------------------------
    m_buttonLoadSettings->setWhatsThis( i18n("<p>Load a set of settings either from the default K3b settings, "
						"settings saved before, or the last used ones.") );
    m_buttonSaveSettings->setWhatsThis( i18n("<p>Saves the current settings of the action dialog."
						"<p>These settings can be loaded with the <em>Load saved settings</em> "
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
    QWidget *widget = new QWidget(this);
    setMainWidget( widget );
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
  loadUserDefaults( c );
}

void K3bInteractionDialog::slotSaveUserDefaults()
{
  KConfigGroup c( k3bcore->config(), m_configGroup );
  saveUserDefaults( c );
}


void K3bInteractionDialog::slotLoadLastSettings()
{
  KConfigGroup c( k3bcore->config(), "last used " + m_configGroup );
  loadUserDefaults( c );
}


void K3bInteractionDialog::saveLastSettings()
{
  KConfigGroup c( k3bcore->config(), "last used " + m_configGroup );
  saveUserDefaults( c );
}


void K3bInteractionDialog::slotStartClickedInternal()
{
  saveLastSettings();

  KConfigGroup c( k3bcore->config(), "General Options" );
  if( !c.readEntry( "action dialog startup settings", 0 ) ) {
    // first time saving last used settings
    switch( K3bMultiChoiceDialog::choose( i18n("Action Dialog Settings"),
					  i18n("<p>K3b handles three sets of settings in action dialogs: "
					       "the defaults, the saved settings, and the last used settings. "
					       "Please choose which of these sets should be loaded if an action "
					       "dialog is opened again."
					       "<p><em>Be aware that this choice can always be changed from the K3b "
					       "configuration dialog.</em>"),
					  QMessageBox::Question,
					  this,
					  3,
					  KGuiItem(i18n("Default Settings")),
					  KGuiItem(i18n("Saved Settings")),
					  KGuiItem(i18n("Last Used Settings")) ) ) {
    case 1:
      c.writeEntry( "action dialog startup settings", int(LOAD_K3B_DEFAULTS) );
      break;
    case 2:
      c.writeEntry( "action dialog startup settings", int(LOAD_SAVED_SETTINGS) );
      break;
    case 3:
      c.writeEntry( "action dialog startup settings", int(LOAD_LAST_SETTINGS) );
      break;
    }
  }

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

  // reset all other default buttons
  if( KPushButton* b = getButton( START_BUTTON ) )
    b->setDefault( true );
  if( KPushButton* b = getButton( SAVE_BUTTON ) )
    b->setDefault( true );
  if( KPushButton* b = getButton( CANCEL_BUTTON ) )
    b->setDefault( true );

  // set the selected default
  if( KPushButton* b = getButton( button ) )
    b->setDefault( true );
}


bool K3bInteractionDialog::eventFilter( QObject* o, QEvent* ev )
{
  if( dynamic_cast<K3bInteractionDialog*>(o) == this &&
      ev->type() == QEvent::KeyPress ) {

    QKeyEvent* kev = dynamic_cast<QKeyEvent*>(ev);

    switch ( kev->key() ) {
    case Qt::Key_Enter:
    case Qt::Key_Return:
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

    case Qt::Key_Escape:
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


KPushButton* K3bInteractionDialog::getButton( int button )
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


void K3bInteractionDialog::setButtonGui( int button,
					 const KGuiItem& item )
{
  if( KPushButton* b = getButton( button ) )
    b->setGuiItem( item );
}


void K3bInteractionDialog::setButtonText( int button,
					  const QString& text,
					  const QString& tooltip,
					  const QString& whatsthis )
{
  if( KPushButton* b = getButton( button ) ) {
    b->setText( text );
    QToolTip::remove( b );
    b->setToolTip( tooltip );
    b->setWhatsThis( whatsthis );
  }
}


void K3bInteractionDialog::setButtonEnabled( int button, bool enabled )
{
  if( KPushButton* b = getButton( button ) ) {
    b->setEnabled( enabled );
    // make sure the correct button is selected as default again
    setDefaultButton( m_defaultButton );
  }
}


void K3bInteractionDialog::setButtonShown( int button, bool shown )
{
  if( KPushButton* b = getButton( button ) ) {
    b->setShown( shown );
    // make sure the correct button is selected as default again
    setDefaultButton( m_defaultButton );
  }
}


void K3bInteractionDialog::setStartButtonText( const QString& text,
					       const QString& tooltip,
					       const QString& whatsthis )
{
  if( m_buttonStart ) {
    m_buttonStart->setText( text );
    QToolTip::remove( m_buttonStart );
    m_buttonStart->setToolTip( tooltip );
    m_buttonStart->setWhatsThis( whatsthis );
  }
}


void K3bInteractionDialog::setCancelButtonText( const QString& text,
						const QString& tooltip,
						const QString& whatsthis )
{
  if( m_buttonCancel ) {
    m_buttonCancel->setText( text );
    QToolTip::remove( m_buttonCancel );
    m_buttonCancel->setToolTip( tooltip );
    m_buttonCancel->setWhatsThis( whatsthis );
  }
}


void K3bInteractionDialog::setSaveButtonText( const QString& text,
					      const QString& tooltip,
					      const QString& whatsthis )
{
  if( m_buttonSave ) {
    m_buttonSave->setText( text );
    QToolTip::remove( m_buttonSave );
    m_buttonSave->setToolTip( tooltip );
    m_buttonSave->setWhatsThis( whatsthis );
  }
}


void K3bInteractionDialog::saveUserDefaults( KConfigGroup& )
{
}


void K3bInteractionDialog::loadUserDefaults( const KConfigGroup& )
{
}


void K3bInteractionDialog::loadK3bDefaults()
{
}


void K3bInteractionDialog::loadStartupSettings()
{
  KConfigGroup c( k3bcore->config(), "General Options" );

  // earlier K3b versions loaded the saved settings
  // so that is what we do as a default
  int i = c.readEntry( "action dialog startup settings", int(LOAD_SAVED_SETTINGS) );
  switch( i ) {
  case LOAD_K3B_DEFAULTS:
    slotLoadK3bDefaults();
    break;
  case LOAD_SAVED_SETTINGS:
    slotLoadUserDefaults();
    break;
  case LOAD_LAST_SETTINGS:
    slotLoadLastSettings();
    break;
  }
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
    kError() << "(K3bInteractionDialog::exec) Recursive call detected." << endl;
    return -1;
  }
  //FIXME kde4
#if 0
  bool destructiveClose = testWFlags( WDestructiveClose );
  clearWFlags( WDestructiveClose );

  bool wasShowModal = testWFlags( WShowModal );
  setWFlags( WShowModal );
#endif
  setResult( 0 );

  loadStartupSettings();
  show();
  if( m_delayedInit )
    QTimer::singleShot( 0, this, SLOT(slotDelayedInit()) );
  else
    init();
  m_inLoop = true;
  enterLoop();
#if 0
  if( !wasShowModal )
    clearWFlags( WShowModal );

  int res = result();

  if( destructiveClose )
    delete this;
  return res;
#endif
  return 0;
}

void K3bInteractionDialog::enterLoop()
{
    QEventLoop eventLoop;
    connect(this, SIGNAL(leaveModality()),
        &eventLoop, SLOT(quit()));
    eventLoop.exec(QEventLoop::ExcludeUserInputEvents);
}

void K3bInteractionDialog::hide()
{
  if( isHidden() )
    return;

  KDialog::hide();
  if( m_inLoop && m_exitLoopOnHide ) {
    m_inLoop = false;
    emit leaveModality();
  }
}


bool K3bInteractionDialog::close( bool alsoDelete )
{
  if( m_inLoop && !m_exitLoopOnHide ) {
    m_inLoop = false;
    emit leaveModality();
  }
  return KDialog::close( alsoDelete );
}


void K3bInteractionDialog::done( int r )
{
  if( m_inLoop && !m_exitLoopOnHide ) {
    m_inLoop = false;
    emit leaveModality();
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
