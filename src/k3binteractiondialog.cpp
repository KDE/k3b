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
#include <QEventLoop>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QPointer>

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
      m_inToggleMode(false),
      m_delayedInit(false),
      m_eventLoop( 0 )
{
    installEventFilter( this );
    setButtons( KDialog::None );

    mainGrid = new QGridLayout( KDialog::mainWidget() );

    mainGrid->setSpacing( spacingHint() );
    mainGrid->setMargin( 0 );

    // header
    // ---------------------------------------------------------------------------------------------------
    m_dialogHeader = new K3bThemedHeader( KDialog::mainWidget() );
    mainGrid->addWidget( m_dialogHeader, 0, 0, 1, 3 );


    // settings buttons
    // ---------------------------------------------------------------------------------------------------
    if( !m_configGroup.isEmpty() ) {
        QHBoxLayout* layout2 = new QHBoxLayout;
        layout2->setSpacing( spacingHint() );
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
        m_buttonCancel = new KPushButton( KConfigGroup( k3bcore->config(), "General Options" )
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

K3bInteractionDialog::~K3bInteractionDialog()
{
    kDebug() << this;
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
    w->setParent( KDialog::mainWidget() );
    mainGrid->addWidget( w, 1, 0, 1, 3 );
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
    close();
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
        b->setVisible( shown );
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
    kDebug() << this;

    // the following code is mainly taken from QDialog::exec
    if( m_eventLoop ) {
        kError() << "(K3bInteractionDialog::exec) Recursive call detected." << endl;
        return -1;
    }

    bool deleteOnClose = testAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_DeleteOnClose, false);

    bool wasShowModal = testAttribute(Qt::WA_ShowModal);
    setAttribute(Qt::WA_ShowModal, true);
    setResult(0);

    loadStartupSettings();

    show();

    if( m_delayedInit )
        QMetaObject::invokeMethod( this, "slotInternalInit", Qt::QueuedConnection );
    else
        slotInternalInit();

    QEventLoop eventLoop;
    m_eventLoop = &eventLoop;
    QPointer<QDialog> guard = this;
    (void) eventLoop.exec();
    if (guard.isNull())
        return QDialog::Rejected;
    m_eventLoop = 0;

    setAttribute(Qt::WA_ShowModal, wasShowModal);

    int res = result();
    if (deleteOnClose)
        delete this;
    return res;
}


void K3bInteractionDialog::hideTemporarily()
{
    hide();
}


void K3bInteractionDialog::close()
{
    if( m_eventLoop ) {
        m_eventLoop->exit();
    }
    KDialog::close();
}


void K3bInteractionDialog::done( int r )
{
    if( m_eventLoop ) {
        m_eventLoop->exit();
    }
    KDialog::done( r );
}


void K3bInteractionDialog::hideEvent( QHideEvent* e )
{
    kDebug() << this;
    KDialog::hideEvent( e );
}


void K3bInteractionDialog::slotToggleAll()
{
    if( !m_inToggleMode ) {
        m_inToggleMode = true;
        toggleAll();
        m_inToggleMode = false;
    }
}


void K3bInteractionDialog::slotInternalInit()
{
    init();
    slotToggleAll();
}


void K3bInteractionDialog::toggleAll()
{
}

#include "k3binteractiondialog.moc"
