/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009 Michal Malek <michalm@jabster.pl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include <config-k3b.h>

#include "k3bsetup2.h"
#include "k3bsetupdevices.h"
#include "k3bsetupprograms.h"
#include <k3bexternalbinmanager.h>

#include <QCheckBox>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QFile>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMap>
#include <QPushButton>
#include <QTimer>

#include <KAboutData>
#include <KConfig>
#include <KDebug>
#include <kdeversion.h>
#include <KEditListBox>
#include <KGenericFactory>
#include <KGlobal>
#include <KLocale>
#include <KMessageBox>
#include <KTextEdit>

#include <PolicyKit/polkit-qt/Action>
#include <PolicyKit/polkit-qt/Auth>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <grp.h>


class K3bSetup2::Private
{
public:
    KConfig* config;
    PolkitQt::Action* authAction;
    K3b::SetupDevices* devicesModel;
    K3b::SetupPrograms* programsModel;
};


K_PLUGIN_FACTORY(K3bSetup2Factory, registerPlugin<K3bSetup2>();)
K_EXPORT_PLUGIN(K3bSetup2Factory("k3bsetup"))



K3bSetup2::K3bSetup2( QWidget *parent, const QVariantList& )
    : KCModule( K3bSetup2Factory::componentData(), parent )
{
    d = new Private();
    d->config = new KConfig( "k3bsetup2rc" );
    d->authAction = new PolkitQt::Action( "org.k3b.setup.update-permissions", this );

    KAboutData* aboutData = new KAboutData("k3bsetup2", 0,
                                           ki18n("K3bSetup 2"), "2.0",
                                           KLocalizedString(), KAboutData::License_GPL,
                                           ki18n("(C) 2003-2007 Sebastian Trueg"), ki18n(0L));
    aboutData->addAuthor(ki18n("Sebastian Trueg"), KLocalizedString(), "trueg@k3b.org");
    setAboutData( aboutData );

    QHBoxLayout* box = new QHBoxLayout( this );
    box->setMargin(0);

    KTextEdit* label = new KTextEdit( this );
    label->setText( "<h2>K3b::Setup</h2>"
                    + i18n("<p>This simple setup assistant is able to set the permissions needed by K3b in order to "
                           "burn CDs and DVDs. "
                           "<p>It does not take things like devfs or resmgr into account. In most cases this is not a "
                           "problem but on some systems the permissions may be altered the next time you login or restart "
                           "your computer. In those cases it is best to consult the distribution documentation."
                           "<p><b>Caution:</b> Although K3b::Setup 2 should not be able "
                           "to mess up your system no guarantee can be given.") );
    label->setReadOnly( true );
    label->setFixedWidth( 200 );

    QWidget* w = new QWidget( this );
    setupUi( w );

    // TODO: enable this and let root specify users
    m_editUsers->hide();
    textLabel2->hide();

    box->addWidget( label );
    box->addWidget( w );

    d->devicesModel = new K3b::SetupDevices( this );
    d->programsModel = new K3b::SetupPrograms(this );

    connect( d->authAction, SIGNAL(activated()),
             this, SLOT(slotPerformPermissionUpdating()) );
    connect( d->authAction, SIGNAL(dataChanged()),
             this, SLOT(slotDataChanged()) );
    connect( d->devicesModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
             this, SLOT(slotDataChanged()) );
    connect( d->devicesModel, SIGNAL(modelReset()),
             this, SLOT(slotDataChanged()) );
    connect( d->programsModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
             this, SLOT(slotDataChanged()) );
    connect( d->programsModel, SIGNAL(modelReset()),
             this, SLOT(slotDataChanged()) );
    connect( m_checkUseBurningGroup, SIGNAL(toggled(bool)),
             this, SLOT(slotBurningGroup()) );
    connect( m_editBurningGroup, SIGNAL(textChanged(const QString&)),
             this, SLOT(slotBurningGroup()) );
    connect( m_editSearchPath, SIGNAL(changed()),
             this, SLOT(slotSearchPrograms()) );

    m_viewDevices->setModel( d->devicesModel );
    m_viewDevices->header()->setResizeMode( QHeaderView::ResizeToContents );
    m_viewPrograms->setModel( d->programsModel );
    m_viewPrograms->header()->setResizeMode( QHeaderView::ResizeToContents );
    
    // Register ProgramItem as meta type
    // to be able to send it through D-BUS
    qDBusRegisterMetaType<K3b::Setup::ProgramItem>();

    load();
}


K3bSetup2::~K3bSetup2()
{
    delete d->config;
    delete d;
}


QString K3bSetup2::quickHelp() const
{
    return i18n("<h2>K3b::Setup 2</h2>"
                "<p>This simple setup assistant is able to set the permissions needed by K3b in order to "
                "burn CDs and DVDs."
                "<p>It does not take into account devfs or resmgr, or similar. In most cases this is not a "
                "problem, but on some systems the permissions may be altered the next time you login or restart "
                "your computer. In these cases it is best to consult the distribution's documentation."
                "<p>The important task that K3b::Setup 2 performs is grant write access to the CD and DVD devices."
                "<p><b>Caution:</b> Although K3b::Setup 2 should not be able "
                "to damage your system, no guarantee can be given.");
}


void K3bSetup2::defaults()
{
    m_checkUseBurningGroup->setChecked(false);
    m_editBurningGroup->setText( "burning" );

    d->devicesModel->defaults();
    d->programsModel->defaults();
}


void K3bSetup2::load()
{
    d->devicesModel->load( *d->config );
    d->programsModel->load( *d->config );

    KConfigGroup grp(d->config, "General Settings" );
    m_checkUseBurningGroup->setChecked( grp.readEntry( "use burning group", false ) );
    m_editBurningGroup->setText( grp.readEntry( "burning group", "burning" ) );

    // load search path
    m_editSearchPath->clear();
    m_editSearchPath->insertStringList( d->programsModel->searchPaths() );

    //
    // This is a hack to work around a kcm bug which makes the faulty assumption that
    // every module starts without anything to apply
    //
    QTimer::singleShot( 0, this, SLOT(slotDataChanged()) );
}


void K3bSetup2::save()
{
    QString burningGroup = m_editBurningGroup->text();

    if( m_checkUseBurningGroup->isChecked() && !burningGroup.isEmpty() ) {
        if( !getgrnam( burningGroup.toLocal8Bit() ) ) {
            KMessageBox::error( this, i18n( "There is no group \"%1\".", burningGroup ) );
            QTimer::singleShot( 0, this, SLOT(slotDataChanged()) );
            return;
        }
    }

    KConfigGroup grp(d->config, "General Settings" );
    grp.writeEntry( "use burning group", m_checkUseBurningGroup->isChecked() );
    grp.writeEntry( "burning group", burningGroup.isEmpty() ? QString("burning") : burningGroup );
    grp.sync();

    d->devicesModel->save( *d->config );
    d->programsModel->save( *d->config );

    if( !d->authAction->activate() ) {
        QTimer::singleShot( 0, this, SLOT(slotDataChanged()) );
    }
}


void K3bSetup2::slotPerformPermissionUpdating()
{
    QDBusConnection::systemBus().connect( "org.k3b.setup",
        "/", "org.k3b.setup", QLatin1String("done"),
        this, SLOT(slotPermissionsUpdated(QStringList,QStringList)) );
    QDBusConnection::systemBus().connect( "org.k3b.setup",
        "/", "org.k3b.setup", QLatin1String("authorizationFailed"),
        this, SLOT(slotAuthFailed()) );
    
    QDBusMessage message = QDBusMessage::createMethodCall( "org.k3b.setup",
        "/", "org.k3b.setup", QLatin1String("updatePermissions"));
    
    // Set burning group name as first argument
    if( m_checkUseBurningGroup->isChecked() && !m_editBurningGroup->text().isEmpty() )
        message << m_editBurningGroup->text();
    else
        message << QString();
    
    // Set devices list as second argument
    message << d->devicesModel->selectedDevices();
    
    // Set programs list as third argument
    QVariantList programs;
    Q_FOREACH( const K3b::Setup::ProgramItem& item, d->programsModel->selectedPrograms() )
    {
        programs << QVariant::fromValue( item );
    }
    message << programs;
    
    // Invoke D-BUS method
    QDBusMessage reply = QDBusConnection::systemBus().call( message, QDBus::NoBlock );
    if( reply.type() == QDBusMessage::ErrorMessage )
    {
        KMessageBox::error( this, i18n("Cannot run helper!") );
        emit changed( true );
    }
}


void K3bSetup2::slotPermissionsUpdated( QStringList updated, QStringList failedToUpdate )
{
    kDebug() << "Objects updated: " << updated;
    kDebug() << "Objects failed to update: " << failedToUpdate;
    
    if( failedToUpdate.isEmpty() )
        KMessageBox::information( this, i18n("Successfully updated all permissions.") );
    else
        KMessageBox::errorList( this, i18n("Following devices and programs could not be updated:"), failedToUpdate );

    // WE MAY USE "newgrp -" to reinitialize the environment if we add users to a group

    d->devicesModel->update();
    d->programsModel->update();
}


void K3bSetup2::slotAuthFailed()
{
    KMessageBox::error( this, i18n("Authorization failed.") );
    emit changed( true );
}


void K3bSetup2::slotDataChanged()
{
    emit changed( d->devicesModel->changesNeeded() || d->programsModel->changesNeeded() );
}


void K3bSetup2::slotBurningGroup()
{
    if( m_checkUseBurningGroup->isChecked() ) {
        d->devicesModel->setBurningGroup( m_editBurningGroup->text() );
        d->programsModel->setBurningGroup( m_editBurningGroup->text() );
    }
    else {
        d->devicesModel->setBurningGroup( QString() );
        d->programsModel->setBurningGroup( QString() );
    }
}


void K3bSetup2::slotSearchPrograms()
{
    d->programsModel->setSearchPaths( m_editSearchPath->items() );
}

#include "k3bsetup2.moc"
