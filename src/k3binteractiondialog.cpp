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

#include "k3bthememanager.h"
#include <k3bapplication.h>

#include <qlabel.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qstring.h>
#include <qpoint.h>
#include <qfont.h>
#include <qpopupmenu.h>
#include <qeventloop.h>
#include <qapplication.h>

#include <klocale.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <kstdguiitem.h>
#include <kpushbutton.h>
#include <kconfig.h>



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
    m_inLoop(false)
{
  mainGrid = new QGridLayout( this );
  mainGrid->setSpacing( spacingHint() );
  mainGrid->setMargin( marginHint() );

  // header
  // ---------------------------------------------------------------------------------------------------
  QFrame* headerFrame = K3bStdGuiItems::purpleFrame( this );
  QHBoxLayout* layout4 = new QHBoxLayout( headerFrame );
  layout4->setMargin( 2 ); // to make sure the frame gets displayed
  layout4->setSpacing( 0 );
  QLabel* pixmapLabelLeft = new QLabel( headerFrame, "pixmapLabelLeft" );
  pixmapLabelLeft->setScaledContents( FALSE );
  layout4->addWidget( pixmapLabelLeft );
  m_labelTitle = new K3bTitleLabel( headerFrame, "m_labelTitle" );
  layout4->addWidget( m_labelTitle );
  layout4->setStretchFactor( m_labelTitle, 1 );
  QLabel* pixmapLabelRight = new QLabel( headerFrame, "pixmapLabelRight" );
  pixmapLabelRight->setScaledContents( FALSE );
  layout4->addWidget( pixmapLabelRight );

  mainGrid->addMultiCellWidget( headerFrame, 0, 0, 0, 1 );

  if( K3bTheme* theme = k3bappcore->themeManager()->currentTheme() ) {
    pixmapLabelLeft->setPaletteBackgroundColor( theme->backgroundColor() );
    pixmapLabelLeft->setPixmap( theme->pixmap( K3bTheme::MEDIA_LEFT ) );
    pixmapLabelRight->setPaletteBackgroundColor( theme->backgroundColor() );
    pixmapLabelRight->setPixmap( theme->pixmap( K3bTheme::MEDIA_NONE ) );
    m_labelTitle->setPaletteBackgroundColor( theme->backgroundColor() );
    m_labelTitle->setPaletteForegroundColor( theme->foregroundColor() );
  }


  // action buttons
  // ---------------------------------------------------------------------------------------------------
  QVBoxLayout* layout5 = new QVBoxLayout( 0, 0, spacingHint(), "layout5");

  if( buttonMask & START_BUTTON ) {
    KGuiItem startItem = KStdGuiItem::ok();
    startItem.setText( i18n("Start") );
    m_buttonStart = new KPushButton( startItem, this, "m_buttonStart" );
    layout5->addWidget( m_buttonStart );
  }
  else
    m_buttonStart = 0;
  if( buttonMask & SAVE_BUTTON ) {
    m_buttonSave = new KPushButton( KStdGuiItem::save(), this, "m_buttonSave" );
    layout5->addWidget( m_buttonSave );
  }
  else
    m_buttonSave = 0;
  if( buttonMask & CANCEL_BUTTON ) {
    m_buttonCancel = new KPushButton( KStdGuiItem::cancel(), this, "m_buttonCancel" );
    layout5->addWidget( m_buttonCancel );
  }
  else
    m_buttonCancel = 0;
  QSpacerItem* spacer_2 = new QSpacerItem( 10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding );
  layout5->addItem( spacer_2 );

  mainGrid->addMultiCellLayout( layout5, 1, 2, 1, 1 );


  // settings buttons
  // ---------------------------------------------------------------------------------------------------
  if( !m_configGroup.isEmpty() ) {
    QHBoxLayout* layout2 = new QHBoxLayout( 0, 0, spacingHint(), "layout2");
    m_buttonK3bDefaults = new QPushButton( i18n("K3b Defaults"), this, "m_buttonK3bDefaults" );
    layout2->addWidget( m_buttonK3bDefaults );
    QSpacerItem* spacer = new QSpacerItem( 10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum );
    layout2->addItem( spacer );
    m_buttonUserDefaults = new K3bPushButton( i18n("User Defaults"), this, "m_buttonUserDefaults" );
    QPopupMenu* userDefaultsPopup = new QPopupMenu( m_buttonUserDefaults );
    userDefaultsPopup->insertItem( i18n("Default Settings"), this, SLOT(slotLoadUserDefaults()) );
    userDefaultsPopup->insertItem( i18n("Last Used Settings"), this, SLOT(slotLoadLastSettings()) );
    ((K3bPushButton*)m_buttonUserDefaults)->setDelayedPopupMenu( userDefaultsPopup );
    layout2->addWidget( m_buttonUserDefaults );
    m_buttonSaveUserDefaults = new QPushButton( i18n("Save User Defaults"), this, "m_buttonSaveUserDefaults" );
    layout2->addWidget( m_buttonSaveUserDefaults );

    mainGrid->addLayout( layout2, 2, 0 );
  }

  mainGrid->setRowStretch( 1, 1 );

  setTitle( title, subTitle );

  initConnections();
  initToolTipsAndWhatsThis();
}

K3bInteractionDialog::~K3bInteractionDialog()
{
}


void K3bInteractionDialog::show()
{
  slotLoadUserDefaults();
  KDialog::show();
  init();
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
//   m_labelTitle->setText( QString("<qt><nobr><font size=\"+1\"><b>%1</b></font> "
//  				 "<font size=\"-1\">%2</font></nobr></qt>").arg(title).arg(subTitle) );
  m_labelTitle->setTitle( title, subTitle );

  setCaption( title );
}


void K3bInteractionDialog::setMainWidget( QWidget* w )
{
  w->reparent( this, QPoint(0,0) );
  mainGrid->addWidget( w, 1, 0 );
  m_mainWidget = w;
}

QWidget* K3bInteractionDialog::mainWidget()
{
  if( !m_mainWidget ) {
    m_mainWidget = new QWidget( this );
    mainGrid->addWidget( m_mainWidget, 1, 0 );
  }
  return m_mainWidget;
}

void K3bInteractionDialog::slotLoadK3bDefaults()
{
  loadK3bDefaults();
}

void K3bInteractionDialog::slotLoadUserDefaults()
{
  KConfig* c = k3bcore->config();
  QString lastGroup = c->group();
  c->setGroup( m_configGroup );
  loadUserDefaults( c );
  c->setGroup( lastGroup );
}

void K3bInteractionDialog::slotSaveUserDefaults()
{
  KConfig* c = k3bcore->config();
  QString lastGroup = c->group();
  c->setGroup( m_configGroup );
  saveUserDefaults( c );
  c->setGroup( lastGroup );
}


void K3bInteractionDialog::slotLoadLastSettings()
{
  KConfig* c = k3bcore->config();
  QString lastGroup = c->group();
  c->setGroup( "last used " + m_configGroup );
  loadUserDefaults( c );
  c->setGroup( lastGroup );
}


void K3bInteractionDialog::saveLastSettings()
{
  KConfig* c = k3bcore->config();
  QString lastGroup = c->group();
  c->setGroup( "last used " + m_configGroup );
  saveUserDefaults( c );
  c->setGroup( lastGroup );
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


void K3bInteractionDialog::setDefaultButton( int b )
{
  m_defaultButton = b;
}


void K3bInteractionDialog::keyPressEvent( QKeyEvent* e )
{
  switch ( e->key() ) {
  case Key_Enter:
  case Key_Return:
    // if the process finished this closes the dialog
    if( m_defaultButton == START_BUTTON ) {
      if( m_buttonStart->isEnabled() )
	slotStartClicked();
    }
    else if( m_defaultButton == CANCEL_BUTTON ) {
      if( m_buttonCancel->isEnabled() )
	slotCancelClicked();
    }
    else if( m_defaultButton == SAVE_BUTTON ) {
      if( m_buttonSave->isEnabled() )
	slotSaveClicked();
    }
    break;
  case Key_Escape:
    // simulate button clicks
    if( m_buttonCancel ) {
      if( m_buttonCancel->isEnabled() )
	slotCancelClicked();
    }
    break;
  default:
    // nothing
    break;
  }

  // Does the tab key still work to jump between children?

  e->accept();
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


void K3bInteractionDialog::saveUserDefaults( KConfig* )
{
}


void K3bInteractionDialog::loadUserDefaults( KConfig* )
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

  show();
  
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


#include "k3binteractiondialog.moc"
