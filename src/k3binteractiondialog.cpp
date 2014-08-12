/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
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
#include "k3bapplication.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QEvent>
#include <QFont>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QPoint>
#include <QString>
#include <QTimer>
#include <QToolButton>
#include <QToolTip>

#include <KConfigCore/KConfig>
#include <KConfigCore/KSharedConfig>
#include <QtCore/QDebug>
#include <KIconThemes/KIconLoader>
#include <KDELibs4Support/KDE/KGlobalSettings>
#include <KDELibs4Support/KDE/KIcon>
#include <KDELibs4Support/KDE/KLocale>
#include <KDELibs4Support/KDE/KPushButton>
#include <KDELibs4Support/KDE/KStandardDirs>
#include <KDELibs4Support/KDE/KStandardGuiItem>


K3b::InteractionDialog::InteractionDialog( QWidget* parent,
                                           const QString& title,
                                           const QString& subTitle,
                                           int buttonMask,
                                           int defaultButton,
                                           const QString& configGroup )
    : KDialog( parent ),
      m_mainWidget(0),
      m_defaultButton(defaultButton),
      m_configGroup(configGroup),
      m_inToggleMode(false),
      m_delayedInit(false)
{
    installEventFilter( this );
    setButtons( KDialog::None );

    mainGrid = new QGridLayout( KDialog::mainWidget() );

    mainGrid->setContentsMargins( 0, 0, 0, 0 );

    // header
    // ---------------------------------------------------------------------------------------------------
    m_dialogHeader = new K3b::ThemedHeader( KDialog::mainWidget() );
    mainGrid->addWidget( m_dialogHeader, 0, 0, 1, 3 );


    // settings buttons
    // ---------------------------------------------------------------------------------------------------
    if( !m_configGroup.isEmpty() ) {
        QHBoxLayout* layout2 = new QHBoxLayout;
        m_buttonLoadSettings = new QToolButton( KDialog::mainWidget() );
        m_buttonLoadSettings->setIcon( KIcon( "document-revert" ) );
        m_buttonLoadSettings->setPopupMode( QToolButton::InstantPopup );
        QMenu* userDefaultsPopup = new QMenu( m_buttonLoadSettings );
        userDefaultsPopup->addAction( i18n("Load default settings"), this, SLOT(slotLoadK3bDefaults()) );
        userDefaultsPopup->addAction( i18n("Load saved settings"), this, SLOT(slotLoadUserDefaults()) );
        userDefaultsPopup->addAction( i18n("Load last used settings"), this, SLOT(slotLoadLastSettings()) );
        m_buttonLoadSettings->setMenu( userDefaultsPopup );
        layout2->addWidget( m_buttonLoadSettings );

        m_buttonSaveSettings = new QToolButton( KDialog::mainWidget() );
        m_buttonSaveSettings->setIcon( KIcon( "document-save" ) );
        layout2->addWidget( m_buttonSaveSettings );

        mainGrid->addLayout( layout2, 2, 0 );
    }

    QSpacerItem* spacer = new QSpacerItem( 10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum );
    mainGrid->addItem( spacer, 2, 1 );


    // action buttons
    // ---------------------------------------------------------------------------------------------------
    QDialogButtonBox *buttonBox = new QDialogButtonBox( KDialog::mainWidget() );
    if( buttonMask & START_BUTTON ) {
        KGuiItem startItem = KStandardGuiItem::ok();
        m_buttonStart = new KPushButton( startItem, buttonBox );
        // refine the button text
        setButtonText( START_BUTTON,
                       i18n("Start"),
                       i18n("Start the task") );
        QFont fnt( m_buttonStart->font() );
        fnt.setBold(true);
        m_buttonStart->setFont( fnt );
        buttonBox->addButton( m_buttonStart, QDialogButtonBox::AcceptRole );
    }
    if( buttonMask & SAVE_BUTTON ) {
        m_buttonSave = new KPushButton( KStandardGuiItem::save(), buttonBox );
        buttonBox->addButton( m_buttonSave, QDialogButtonBox::ApplyRole );
    }
    else {
        m_buttonSave = 0;
    }
    if( buttonMask & CANCEL_BUTTON ) {
        m_buttonCancel = new KPushButton( KConfigGroup( KSharedConfig::openConfig(), "General Options" )
                                          .readEntry( "keep action dialogs open", false )
                                          ? KStandardGuiItem::close()
                                          : KStandardGuiItem::cancel(),
                                          buttonBox );
        buttonBox->addButton( m_buttonCancel, QDialogButtonBox::RejectRole );
    }
    else {
        m_buttonCancel = 0;
    }

    mainGrid->addWidget( buttonBox, 2, 2 );
    mainGrid->setRowStretch( 1, 1 );

    setTitle( title, subTitle );

    initConnections();
    initToolTipsAndWhatsThis();

    setDefaultButton( START_BUTTON );
}

K3b::InteractionDialog::~InteractionDialog()
{
    qDebug() << this;
}


void K3b::InteractionDialog::show()
{
    KDialog::show();
    if( KPushButton* b = getButton( m_defaultButton ) )
        b->setFocus();
}


QSize K3b::InteractionDialog::sizeHint() const
{
    QSize s = KDialog::sizeHint();
    // I want the dialogs to look good.
    // That means their height should never outgrow their width
    if( s.height() > s.width() )
        s.setWidth( s.height() );

    return s;
}


void K3b::InteractionDialog::initConnections()
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


void K3b::InteractionDialog::initToolTipsAndWhatsThis()
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


void K3b::InteractionDialog::setTitle( const QString& title, const QString& subTitle )
{
    m_dialogHeader->setTitle( title, subTitle );

    setWindowTitle( title );
}


void K3b::InteractionDialog::setMainWidget( QWidget* w )
{
    w->setParent( KDialog::mainWidget() );
    mainGrid->addWidget( w, 1, 0, 1, 3 );
    m_mainWidget = w;
}


QWidget* K3b::InteractionDialog::mainWidget()
{
    if( !m_mainWidget ) {
        QWidget *widget = new QWidget(this);
        setMainWidget( widget );
    }
    return m_mainWidget;
}


void K3b::InteractionDialog::slotLoadK3bDefaults()
{
    KSharedConfig::Ptr c = KSharedConfig::openConfig();
    c->setReadDefaults( true );
    loadSettings( c->group( m_configGroup ) );
    c->setReadDefaults( false );
}


void K3b::InteractionDialog::slotLoadUserDefaults()
{
    KConfigGroup c( KSharedConfig::openConfig(), m_configGroup );
    loadSettings( c );
}


void K3b::InteractionDialog::slotSaveUserDefaults()
{
    KConfigGroup c( KSharedConfig::openConfig(), m_configGroup );
    saveSettings( c );
}


void K3b::InteractionDialog::slotLoadLastSettings()
{
    KConfigGroup c( KSharedConfig::openConfig(), "last used " + m_configGroup );
    loadSettings( c );
}


void K3b::InteractionDialog::saveLastSettings()
{
    KConfigGroup c( KSharedConfig::openConfig(), "last used " + m_configGroup );
    saveSettings( c );
}


void K3b::InteractionDialog::slotStartClickedInternal()
{
    saveLastSettings();

    slotStartClicked();
}


void K3b::InteractionDialog::slotStartClicked()
{
    emit started();
}


void K3b::InteractionDialog::slotCancelClicked()
{
    emit canceled();
    close();
}


void K3b::InteractionDialog::slotSaveClicked()
{
    emit saved();
}


void K3b::InteractionDialog::setDefaultButton( int button )
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


bool K3b::InteractionDialog::eventFilter( QObject* o, QEvent* ev )
{
    if( dynamic_cast<K3b::InteractionDialog*>(o) == this &&
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


KPushButton* K3b::InteractionDialog::getButton( int button )
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


void K3b::InteractionDialog::setButtonGui( int button,
                                           const KGuiItem& item )
{
    if( KPushButton* b = getButton( button ) )
        b->setGuiItem( item );
}


void K3b::InteractionDialog::setButtonText( int button,
                                            const QString& text,
                                            const QString& tooltip,
                                            const QString& whatsthis )
{
    if( KPushButton* b = getButton( button ) ) {
        b->setText( text );
        b->setToolTip( tooltip );
        b->setWhatsThis( whatsthis );
    }
}


void K3b::InteractionDialog::setButtonEnabled( int button, bool enabled )
{
    if( KPushButton* b = getButton( button ) ) {
        b->setEnabled( enabled );
        // make sure the correct button is selected as default again
        setDefaultButton( m_defaultButton );
    }
}


void K3b::InteractionDialog::setButtonShown( int button, bool shown )
{
    if( KPushButton* b = getButton( button ) ) {
        b->setVisible( shown );
        // make sure the correct button is selected as default again
        setDefaultButton( m_defaultButton );
    }
}


void K3b::InteractionDialog::setStartButtonText( const QString& text,
                                                 const QString& tooltip,
                                                 const QString& whatsthis )
{
    if( m_buttonStart ) {
        m_buttonStart->setText( text );
        m_buttonStart->setToolTip( tooltip );
        m_buttonStart->setWhatsThis( whatsthis );
    }
}


void K3b::InteractionDialog::setCancelButtonText( const QString& text,
                                                  const QString& tooltip,
                                                  const QString& whatsthis )
{
    if( m_buttonCancel ) {
        m_buttonCancel->setText( text );
        m_buttonCancel->setToolTip( tooltip );
        m_buttonCancel->setWhatsThis( whatsthis );
    }
}


void K3b::InteractionDialog::setSaveButtonText( const QString& text,
                                                const QString& tooltip,
                                                const QString& whatsthis )
{
    if( m_buttonSave ) {
        m_buttonSave->setText( text );
        m_buttonSave->setToolTip( tooltip );
        m_buttonSave->setWhatsThis( whatsthis );
    }
}


void K3b::InteractionDialog::saveSettings( KConfigGroup )
{
}


void K3b::InteractionDialog::loadSettings( const KConfigGroup& )
{
}


void K3b::InteractionDialog::loadStartupSettings()
{
    KConfigGroup c( KSharedConfig::openConfig(), "General Options" );

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


int K3b::InteractionDialog::exec()
{
    qDebug() << this;

    loadStartupSettings();

    if( m_delayedInit )
        QMetaObject::invokeMethod( this, "slotInternalInit", Qt::QueuedConnection );
    else
        slotInternalInit();

    return KDialog::exec();
}


void K3b::InteractionDialog::hideTemporarily()
{
    hide();
}


void K3b::InteractionDialog::close()
{
    KDialog::close();
}


void K3b::InteractionDialog::done( int r )
{
    KDialog::done( r );
}


void K3b::InteractionDialog::hideEvent( QHideEvent* e )
{
    qDebug() << this;
    KDialog::hideEvent( e );
}


void K3b::InteractionDialog::slotToggleAll()
{
    if( !m_inToggleMode ) {
        m_inToggleMode = true;
        toggleAll();
        m_inToggleMode = false;
    }
}


void K3b::InteractionDialog::slotInternalInit()
{
    init();
    slotToggleAll();
}


void K3b::InteractionDialog::toggleAll()
{
}

#include "k3binteractiondialog.moc"
