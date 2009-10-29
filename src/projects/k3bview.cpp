/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

// application specific includes
#include "k3bview.h"
#include "k3bdoc.h"
#include "k3bfillstatusdisplay.h"
#include "k3bprojectburndialog.h"
#include "k3bprojectplugindialog.h"
#include "k3bpluginmanager.h"
#include "k3bprojectplugin.h"
#include "k3bcore.h"
#include "k3baction.h"

// include files for Qt
#include <QVBoxLayout>
#include <QList>

// include files for KDE
#include <KAction>
#include <KLocale>
#include <KMessageBox>
#include <KDebug>
#include <KToolBar>

K3b::View::View( K3b::Doc* pDoc, QWidget *parent )
    : QWidget( parent ),
      m_doc( pDoc )
{
    m_toolBox = new KToolBar( this );
    m_fillStatusDisplay = new K3b::FillStatusDisplay( m_doc, this );
    
    m_layout = new QVBoxLayout;
    m_layout->addWidget( m_fillStatusDisplay );
    m_layout->setSpacing( 5 );
    m_layout->setMargin( 0 );
    
    QVBoxLayout* mainLayout = new QVBoxLayout( this );
    mainLayout->addWidget( m_toolBox );
    mainLayout->addLayout( m_layout );
    mainLayout->setSpacing( 0 );
    mainLayout->setContentsMargins( 2, 0, 2, 2 );

    KAction* burnAction = K3b::createAction(this,i18n("&Burn"), "tools-media-optical-burn", Qt::CTRL + Qt::Key_B, this, SLOT(slotBurn()),
                                            actionCollection(), "project_burn");
    burnAction->setToolTip( i18n("Open the burn dialog for the current project") );
    KAction* propAction = K3b::createAction(this, i18n("&Properties"), "document-properties", Qt::CTRL + Qt::Key_P, this, SLOT(slotProperties()),
                                            actionCollection(), "project_properties");
    propAction->setToolTip( i18n("Open the properties dialog") );

    m_toolBox->addAction( burnAction/*, true*/ );
    m_toolBox->addSeparator();

    // this is just for testing (or not?)
    // most likely every project type will have it's rc file in the future
    setXML( "<!DOCTYPE kpartgui SYSTEM \"kpartgui.dtd\">"
            "<kpartgui name=\"k3bproject\" version=\"1\">"
            "<MenuBar>"
            " <Menu name=\"project\"><text>&amp;Project</text>"
            "  <Action name=\"project_burn\"/>"
            "  <Action name=\"project_properties\"/>"
            " </Menu>"
            "</MenuBar>"
            "</kpartgui>", true );
}

K3b::View::~View()
{
}


void K3b::View::setMainWidget( QWidget* w )
{
    m_layout->insertWidget( 0, w, 1 );
}


void K3b::View::slotBurn()
{
    if( m_doc->numOfTracks() == 0 || m_doc->size() == 0 ) {
        KMessageBox::information( this, i18n("Please add files to your project first."),
                                  i18n("No Data to Burn"), QString(), false );
    }
    else {
        K3b::ProjectBurnDialog* dlg = newBurnDialog( this );
        if( dlg ) {
            dlg->execBurnDialog(true);
            delete dlg;
        }
        else {
            kDebug() << "(K3b::Doc) Error: no burndialog available.";
        }
    }
}


void K3b::View::slotProperties()
{
    K3b::ProjectBurnDialog* dlg = newBurnDialog( this );
    if( dlg ) {
        dlg->execBurnDialog(false);
        delete dlg;
    }
    else {
        kDebug() << "(K3b::Doc) Error: no burndialog available.";
    }
}


void K3b::View::addPluginButtons()
{
    QList<K3b::Plugin*> pl = k3bcore->pluginManager()->plugins( "ProjectPlugin" );
    for( QList<K3b::Plugin*>::const_iterator it = pl.constBegin();
         it != pl.constEnd(); ++it ) {
        K3b::ProjectPlugin* pp = dynamic_cast<K3b::ProjectPlugin*>( *it );
        if( pp && (pp->type() & m_doc->type()) ) {
            QAction* button = toolBox()->addAction(     pp->text(),
                                                        this,
                                                        SLOT(slotPluginButtonClicked()) );
            button->setIcon(QIcon(pp->icon()));
            button->setToolTip (pp->toolTip());
            button->setWhatsThis(pp->whatsThis());
            m_plugins.insert( button, pp );
        }
    }
}


void K3b::View::slotPluginButtonClicked()
{
    const QObject* o = sender();
    if( K3b::ProjectPlugin* p = m_plugins[o] ) {
        if( p->hasGUI() ) {
            K3b::ProjectPluginDialog dlg( p, doc(), this );
            dlg.exec();
        }
        else
            p->activate( doc(), this );
    }
}


void K3b::View::addUrl( const KUrl& url )
{
    KUrl::List urls(url);
    addUrls( urls );
}


void K3b::View::addUrls( const KUrl::List& urls )
{
    doc()->addUrls( urls );
}

#include "k3bview.moc"
