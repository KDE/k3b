/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *           (C) 2009      Arthur Mello <arthur@mandriva.com>
 *           (C) 2009      Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
 *           (C) 2009-2010 Michal Malek <michalm@jabster.pl>
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

#include "k3baudioview.h"

#include "k3bapplication.h"
#include "k3baudioburndialog.h"
#include "k3baudiodoc.h"
#include "k3baudioprojectmodel.h"
#include "k3baudioviewimpl.h"
#include "k3bfillstatusdisplay.h"
#include "k3bpluginmanager.h"

#include <QLayout>
#include <QString>
#include <QScrollBar>
#include <QTreeView>

#include <KAction>
#include <KActionCollection>
#include <KDebug>
#include <KLocale>
#include <KMessageBox>
#include <KToolBar>
#include <fcntl.h>


K3b::AudioView::AudioView( K3b::AudioDoc* doc, QWidget* parent )
    : K3b::View( doc, parent )
{
    m_doc = doc;
    m_audioViewImpl = new AudioViewImpl( this, m_doc, actionCollection() );

    setMainWidget( m_audioViewImpl->view() );

    fillStatusDisplay()->showTime();

    toolBox()->addAction( actionCollection()->action( "project_audio_convert" ) );
    toolBox()->addSeparator();

#ifdef __GNUC__
#warning enable player once ported to Phonon
#endif
//     toolBox()->addAction( m_songlist->player()->action( AudioTrackPlayer::ACTION_PLAY ) );
//     toolBox()->addAction( m_songlist->player()->action( AudioTrackPlayer::ACTION_PAUSE ) );
//     toolBox()->addAction( m_songlist->player()->action( AudioTrackPlayer::ACTION_STOP ) );
//     toolBox()->addSpacing();
//     toolBox()->addAction( m_songlist->player()->action( AudioTrackPlayer::ACTION_PREV ) );
//     toolBox()->addAction( m_songlist->player()->action( AudioTrackPlayer::ACTION_NEXT ) );
//     toolBox()->addSpacing();
//     m_songlist->player()->action( AudioTrackPlayer::ACTION_SEEK )->plug( toolBox() );
//     toolBox()->addSeparator();

#if 0
#ifdef HAVE_MUSICBRAINZ
    kDebug() << "(AudioView) m_songlist->actionCollection()->actions().count() " << m_songlist->actionCollection()->actions().count();
    toolBox()->addAction( m_songlist->actionCollection()->action( "project_audio_musicbrainz" ) );
    toolBox()->addSeparator();
#endif
#endif

    toolBox()->addActions( createPluginsActions( m_doc->type() ) );

    // this is just for testing (or not?)
    // most likely every project type will have it's rc file in the future
    // we only add the additional actions since View already added the default actions
    setXML( "<!DOCTYPE kpartgui SYSTEM \"kpartgui.dtd\">"
            "<kpartgui name=\"k3bproject\" version=\"1\">"
            "<MenuBar>"
            " <Menu name=\"project\"><text>&amp;Project</text>"
            "  <Action name=\"project_audio_convert\"/>"
#ifdef HAVE_MUSICBRAINZ
            "  <Action name=\"project_audio_musicbrainz\"/>"
#endif
            " </Menu>"
            "</MenuBar>"
            "</kpartgui>", true );
}

K3b::AudioView::~AudioView()
{
}


void K3b::AudioView::addUrls( const KUrl::List& urls )
{
    m_audioViewImpl->addUrls( urls );
}


K3b::ProjectBurnDialog* K3b::AudioView::newBurnDialog( QWidget* parent )
{
    return new AudioBurnDialog( m_doc, parent );
}

#include "k3baudioview.moc"
