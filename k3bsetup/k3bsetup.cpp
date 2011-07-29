/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009 Michal Malek <michalm@jabster.pl>
 * Copyright (C) 2010 Dario Freddi <drf@kde.org>
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

#include <config-k3b.h>

#include "k3bsetup.h"
#include "k3bsetupdevicesmodel.h"
#include "k3bsetupprogramsmodel.h"
#include "k3bexternalbinmanager.h"

#include <QCheckBox>
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
#include <KAuth/Action>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <grp.h>


class K3bSetup::Private
{
public:
    KConfig* config;
    K3b::Setup::DevicesModel* devicesModel;
    K3b::Setup::ProgramsModel* programsModel;
};


K_PLUGIN_FACTORY(K3bSetupFactory, registerPlugin<K3bSetup>();)
K_EXPORT_PLUGIN(K3bSetupFactory("k3bsetup"))



K3bSetup::K3bSetup( QWidget *parent, const QVariantList& )
    : KCModule( K3bSetupFactory::componentData(), parent )
{
    d = new Private();
    d->config = new KConfig( "k3bsetuprc" );

    qRegisterMetaType<K3b::Setup::ProgramItem>();
    qRegisterMetaTypeStreamOperators<K3b::Setup::ProgramItem>( "K3b::Setup::ProgramItem" );

    KAboutData* aboutData = new KAboutData("k3bsetup", 0,
                                           ki18n("K3bSetup"), "2.0",
                                           KLocalizedString(), KAboutData::License_GPL,
                                           ki18n("(C) 2003-2007 Sebastian Trueg"), ki18n(0L));
    aboutData->addAuthor(ki18n("Sebastian Trueg"), KLocalizedString(), "trueg@k3b.org");
    setAboutData( aboutData );

    QHBoxLayout* box = new QHBoxLayout( this );
    box->setContentsMargins( 0, 0, 0, 0 );

    KTextEdit* label = new KTextEdit( this );
    label->setText( "<h2>K3b::Setup</h2>"
                    + i18n("<p>This simple setup assistant is able to set the permissions needed by K3b in order to "
                           "burn CDs and DVDs. "
                           "<p>It does not take things like devfs or resmgr into account. In most cases this is not a "
                           "problem but on some systems the permissions may be altered the next time you login or restart "
                           "your computer. In those cases it is best to consult the distribution documentation."
                           "<p><b>Caution:</b> Although K3b::Setup should not be able "
                           "to mess up your system no guarantee can be given.") );
    label->setReadOnly( true );
    label->setFixedWidth( 200 );

    QWidget* w = new QWidget( this );
    setupUi( w );

    // TODO: enable this and let root specify users
    m_frameUsers->hide();

    box->addWidget( label );
    box->addWidget( w );

    d->devicesModel = new K3b::Setup::DevicesModel( this );
    d->programsModel = new K3b::Setup::ProgramsModel(this );

    connect( d->devicesModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
             this, SLOT(slotDataChanged()) );
    connect( d->devicesModel, SIGNAL(modelReset()),
             this, SLOT(slotDataChanged()) );
    connect( d->programsModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
             this, SLOT(slotDataChanged()) );
    connect( d->programsModel, SIGNAL(modelReset()),
             this, SLOT(slotDataChanged()) );
    connect( m_checkUseBurningGroup, SIGNAL(toggled(bool)),
             this, SLOT(slotBurningGroupChanged()) );
    connect( m_editBurningGroup, SIGNAL(textChanged(QString)),
             this, SLOT(slotBurningGroupChanged()) );
    connect( m_editSearchPath, SIGNAL(changed()),
             this, SLOT(slotSearchPathChanged()) );

    m_viewDevices->setModel( d->devicesModel );
    m_viewDevices->header()->setResizeMode( QHeaderView::ResizeToContents );
    m_viewPrograms->setModel( d->programsModel );
    m_viewPrograms->header()->setResizeMode( QHeaderView::ResizeToContents );

    setNeedsAuthorization(true);

    load();
}


K3bSetup::~K3bSetup()
{
    delete d->config;
    delete d;
}


QString K3bSetup::quickHelp() const
{
    return i18n("<h2>K3b::Setup</h2>"
                "<p>This simple setup assistant is able to set the permissions needed by K3b in order to "
                "burn CDs and DVDs."
                "<p>It does not take into account devfs or resmgr, or similar. In most cases this is not a "
                "problem, but on some systems the permissions may be altered the next time you login or restart "
                "your computer. In these cases it is best to consult the distribution's documentation."
                "<p>The important task that K3b::Setup performs is grant write access to the CD and DVD devices."
                "<p><b>Caution:</b> Although K3b::Setup should not be able "
                "to damage your system, no guarantee can be given.");
}


void K3bSetup::defaults()
{
    m_checkUseBurningGroup->setChecked(false);
    m_editBurningGroup->setText( "burning" );

    d->devicesModel->defaults();
    d->programsModel->defaults();
}


void K3bSetup::load()
{
    d->devicesModel->load( *d->config );
    d->programsModel->load( *d->config );

    KConfigGroup grp(d->config, "General Settings" );
    m_checkUseBurningGroup->setChecked( grp.readEntry( "use burning group", false ) );
    m_editBurningGroup->setText( grp.readEntry( "burning group", "burning" ) );

    // load search path
    m_editSearchPath->clear();
    m_editSearchPath->insertStringList( d->programsModel->searchPaths() );
}


void K3bSetup::save()
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

    QVariantMap args;
    // Set burning group name as first argument
    if( m_checkUseBurningGroup->isChecked() && !m_editBurningGroup->text().isEmpty() )
        args["burningGroup"] = m_editBurningGroup->text();
    else
        args["burningGroup"] = QString();

    // Set devices list as second argument
    args["devices"] =  d->devicesModel->selectedDevices();

    // Set programs list as third argument
    QVariantList programs;
    Q_FOREACH( const K3b::Setup::ProgramItem& item, d->programsModel->selectedPrograms() )
    {
        programs << QVariant::fromValue( item );
    }
    args["programs"] = programs;

    KAuth::Action *action = authAction();
    action->setArguments(args);

    KAuth::ActionReply reply = action->execute();

    if (reply.failed()) {
        // TODO: We can give some more details about the error here
        kDebug() << reply.errorCode() << reply.errorDescription();
        KMessageBox::error( this, i18n("Cannot run worker.") );
        emit changed( true );
    } else {
        // Success!!
        QStringList updated = reply.data()["updated"].toStringList();
        QStringList failedToUpdate = reply.data()["failedToUpdate"].toStringList();
        kDebug() << "Objects updated: " << updated;
        kDebug() << "Objects failed to update: " << failedToUpdate;

        if( !failedToUpdate.isEmpty() )
            KMessageBox::errorList( this, i18n("Following devices and programs could not be updated:"), failedToUpdate );

        // WE MAY USE "newgrp -" to reinitialize the environment if we add users to a group

        d->devicesModel->update();
        d->programsModel->update();
    }
}


void K3bSetup::slotDataChanged()
{
    KConfigGroup grp(d->config, "General Settings" );
    bool useBurningGroupChanged = m_checkUseBurningGroup->isChecked() != grp.readEntry( "use burning group", false );
    bool burningGroupChanged = m_checkUseBurningGroup->isChecked() && m_editBurningGroup->text() != grp.readEntry( "burning group", "burning" );

    emit changed(
        useBurningGroupChanged ||
        burningGroupChanged ||
        d->devicesModel->changesNeeded() ||
        d->programsModel->changesNeeded() );
}


void K3bSetup::slotBurningGroupChanged()
{
    if( m_checkUseBurningGroup->isChecked() ) {
        d->devicesModel->setBurningGroup( m_editBurningGroup->text() );
        d->programsModel->setBurningGroup( m_editBurningGroup->text() );
    }
    else {
        d->devicesModel->setBurningGroup( QString() );
        d->programsModel->setBurningGroup( QString() );
    }

    slotDataChanged();
}


void K3bSetup::slotSearchPathChanged()
{
    d->programsModel->setSearchPaths( m_editSearchPath->items() );
}

#include "k3bsetup.moc"
