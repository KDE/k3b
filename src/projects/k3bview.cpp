/*
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
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

#include <KLocalizedString>
#include <KMessageBox>
#include <KToolBar>

#include <QDebug>
#include <QList>
#include <QAction>
#include <QVBoxLayout>

K3b::View::View( K3b::Doc* pDoc, QWidget *parent )
    : QWidget( parent ),
      m_doc( pDoc )
{
    m_toolBox = new KToolBar( this );
    m_fillStatusDisplay = new K3b::FillStatusDisplay( m_doc, this );

    QVBoxLayout* fillStatusDisplayLayout = new QVBoxLayout;
    fillStatusDisplayLayout->addWidget( m_fillStatusDisplay );
    fillStatusDisplayLayout->setContentsMargins( 0, 5, 0, 0 );

    m_layout = new QVBoxLayout( this );
    m_layout->addWidget( m_toolBox );
    m_layout->addLayout( fillStatusDisplayLayout );
    m_layout->setSpacing( 0 );
    m_layout->setContentsMargins( 0, 0, 0, 0 );

    QAction* burnAction = K3b::createAction(this,i18n("&Burn"), "tools-media-optical-burn", Qt::CTRL + Qt::Key_B, this, SLOT(slotBurn()),
                                            actionCollection(), "project_burn");
    burnAction->setToolTip( i18n("Open the burn dialog for the current project") );
    QAction* propAction = K3b::createAction(this, i18n("&Properties"), "document-properties", Qt::CTRL + Qt::Key_P, this, SLOT(slotProperties()),
                                            actionCollection(), "project_properties");
    propAction->setToolTip( i18n("Open the properties dialog") );

    m_toolBox->addAction( burnAction/*, true*/ );
    m_toolBox->addSeparator();

    // this is just for testing (or not?)
    // most likely every project type will have it's rc file in the future
    setXML( "<!DOCTYPE gui SYSTEM \"kpartgui.dtd\">"
            "<gui name=\"k3bproject\" version=\"1\">"
            "<MenuBar>"
            " <Menu name=\"project\"><text>&amp;Project</text>"
            "  <Action name=\"project_burn\"/>"
            "  <Action name=\"project_properties\"/>"
            " </Menu>"
            "</MenuBar>"
            "</gui>", true );
}

K3b::View::~View()
{
}


void K3b::View::setMainWidget( QWidget* w )
{
    m_layout->insertWidget( 1, w, 1 );
}


void K3b::View::slotBurn()
{
    if( m_doc->numOfTracks() == 0 || m_doc->size() == 0 ) {
        KMessageBox::information( this, i18n("Please add files to your project first."),
                                  i18n("No Data to Burn") );
    }
    else {
        K3b::ProjectBurnDialog* dlg = newBurnDialog( this );
        if( dlg ) {
            dlg->execBurnDialog(true);
            delete dlg;
        }
        else {
            qDebug() << "(K3b::Doc) Error: no burndialog available.";
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
        qDebug() << "(K3b::Doc) Error: no burndialog available.";
    }
}


QList<QAction*> K3b::View::createPluginsActions( Doc::Type docType )
{
    QList<QAction*> actions;
    Q_FOREACH( Plugin* plugin, k3bcore->pluginManager()->plugins( "ProjectPlugin" ) ) {
        ProjectPlugin* pp = dynamic_cast<ProjectPlugin*>( plugin );
        if( pp && pp->type().testFlag( docType) ) {
            QAction* action = new QAction( pp->icon(), pp->text(), this );
            action->setToolTip( pp->toolTip() );
            action->setWhatsThis( pp->whatsThis() );
            connect( action, SIGNAL(triggered(bool)),
                     this, SLOT(slotPluginButtonClicked()) );
            actions.push_back( action );
            m_plugins.insert( action, pp );
        }
    }
    return actions;
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


void K3b::View::addUrl( const QUrl& url )
{
    addUrls( QList<QUrl>() << url );
}


void K3b::View::addUrls( const QList<QUrl>& urls )
{
    doc()->addUrls( urls );
}

#include "moc_k3bview.cpp"
