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

#include <QtGui/QLayout>
#include <QtCore/QMap>
#include <QtCore/QFile>
#include <QtGui/QCheckBox>
#include <QtGui/QLineEdit>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtCore/QTimer>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>

#include <KAboutData>
#include <KConfig>
#include <kdeversion.h>
#include <KEditListBox>
#include <KGenericFactory>
#include <KGlobal>
#include <KLocale>
#include <KMessageBox>
#include <KTextEdit>

#include "k3bsetup2.h"
#include "k3bsetupdevices.h"
#include "k3bsetupprograms.h"

#include <k3bexternalbinmanager.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <grp.h>


class K3bSetup2::Private
{
public:
    K3b::SetupDevices* devicesModel;
    K3b::SetupPrograms* programsModel;

    KConfig* config;
};


K_PLUGIN_FACTORY(K3bSetup2Factory, registerPlugin<K3bSetup2>();)
K_EXPORT_PLUGIN(K3bSetup2Factory("k3bsetup"))



K3bSetup2::K3bSetup2( QWidget *parent, const QVariantList& )
    : KCModule( K3bSetup2Factory::componentData(), parent )
{
    d = new Private();
    d->config = new KConfig( "k3bsetup2rc" );

    KAboutData* aboutData = new KAboutData("k3bsetup2", 0,
                                           ki18n("K3bSetup 2"), "2.0",
                                           KLocalizedString(), KAboutData::License_GPL,
                                           ki18n("(C) 2003-2007 Sebastian Trueg"), ki18n(0L));
    aboutData->addAuthor(ki18n("Sebastian Trueg"), KLocalizedString(), "trueg@k3b.org");
    setAboutData( aboutData );

    QHBoxLayout* box = new QHBoxLayout( this );
    box->setMargin(0);
    box->setSpacing( KDialog::spacingHint() );

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

    load();

    //
    // This is a hack to work around a kcm bug which makes the faulty assumption that
    // every module starts without anything to apply
    //
    QTimer::singleShot( 0, this, SLOT(slotDataChanged()) );

    if( getuid() != 0 /*|| !d->config->isConfigWritable()*/ ) {
        m_checkUseBurningGroup->setEnabled( false );
        m_editBurningGroup->setEnabled( false );
        m_editUsers->setEnabled( false );
        m_viewDevices->setEnabled( false );
        m_viewPrograms->setEnabled( false );
        m_editSearchPath->setEnabled( false );
    }
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
}


void K3bSetup2::save()
{
    QString burningGroup = m_editBurningGroup->text();

    KConfigGroup grp(d->config, "General Settings" );
    grp.writeEntry( "use burning group", m_checkUseBurningGroup->isChecked() );
    grp.writeEntry( "burning group", burningGroup.isEmpty() ? QString("burning") : burningGroup );
    grp.sync();

    d->devicesModel->save( *d->config );
    d->programsModel->save( *d->config );

    bool success = true;

    struct group* g = 0;
    if( m_checkUseBurningGroup->isChecked() && !burningGroup.isEmpty() ) {
        // TODO: create the group if it's not there
        g = getgrnam( burningGroup.toLocal8Bit() );
        if( !g ) {
            KMessageBox::error( this, i18n("There is no group %1.",burningGroup) );
            return;
        }
    }

    Q_FOREACH( const QString& dev, d->devicesModel->selectedDevices() )
    {
        if( g != 0 ) {
            if( ::chmod( QFile::encodeName(dev), S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP ) )
                success = false;

            if( ::chown( QFile::encodeName(dev), (gid_t)-1, g->gr_gid ) )
                success = false;
        }
        else {
            if( ::chmod( QFile::encodeName(dev), S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH ) )
                success = false;
        }
    }

    Q_FOREACH( const K3b::ExternalBin* bin, d->programsModel->selectedPrograms() )
    {
        if( g != 0 ) {
            if( ::chown( QFile::encodeName(bin->path), (gid_t)0, g->gr_gid ) )
                success = false;

            int perm = 0;
            if( K3b::SetupPrograms::shouldRunSuidRoot( bin ) )
                perm = S_ISUID|S_IRWXU|S_IXGRP;
            else
                perm = S_IRWXU|S_IXGRP|S_IRGRP;

            if( ::chmod( QFile::encodeName(bin->path), perm ) )
                success = false;
        }
        else {
            if( ::chown( QFile::encodeName(bin->path), 0, 0 ) )
                success = false;

            int perm = 0;
            if( K3b::SetupPrograms::shouldRunSuidRoot( bin ) )
                perm = S_ISUID|S_IRWXU|S_IXGRP|S_IXOTH;
            else
                perm = S_IRWXU|S_IXGRP|S_IRGRP|S_IXOTH|S_IROTH;

            if( ::chmod( QFile::encodeName(bin->path), perm ) )
                success = false;
        }
    }


    if( success )
        KMessageBox::information( this, i18n("Successfully updated all permissions.") );
    else {
        if( getuid() )
            KMessageBox::error( this, i18n("Could not update all permissions. You should run K3b::Setup 2 as root.") );
        else
            KMessageBox::error( this, i18n("Could not update all permissions.") );
    }

    // WE MAY USE "newgrp -" to reinitialize the environment if we add users to a group

    d->devicesModel->update();
    d->programsModel->update();
}


void K3bSetup2::slotDataChanged()
{
    emit changed( ( getuid() != 0 ) ? false : d->devicesModel->changesNeeded() || d->programsModel->changesNeeded() );
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
