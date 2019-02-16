/*
 *
 * Copyright (C) 2005-2008 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bdatamultisessioncombobox.h"

#include <KConfigGroup>
#include <KLocalizedString>

#include <QToolTip>


static const int s_autoIndex = 0;
static const int s_noneIndex = 1;
static const int s_startIndex = 2;
static const int s_continueIndex = 3;
static const int s_finishIndex = 4;


K3b::DataMultiSessionCombobox::DataMultiSessionCombobox( QWidget* parent )
    : QComboBox( parent ),
      m_forceNoMultiSession(false)
{
    init( false );

    this->setToolTip( i18n("Select the Multisession Mode for the project.") );
    this->setWhatsThis( i18n("<p><b>Multisession Mode</b>"
                             "<p><b>Auto</b><br>"
                             "Let K3b decide which mode to use. The decision will be based "
                             "on the size of the project (does it fill the whole media) and "
                             "the state of the inserted media (appendable or not)."
                             "<p><b>No Multisession</b><br>"
                             "Create a single-session CD or DVD and close the disk."
                             "<p><b>Start Multisession</b><br>"
                             "Start a multisession CD or DVD, not closing the disk to "
                             "allow further sessions to be appended."
                             "<p><b>Continue Multisession</b><br>"
                             "Continue an appendable data CD (as for example created in "
                             "<em>Start Multisession</em> mode) and add another session "
                             "without closing the disk to "
                             "allow further sessions to be appended."
                             "<p><b>Finish Multisession</b><br>"
                             "Continue an appendable data CD (as for example created in "
                             "<em>Start Multisession</em> mode), add another session, "
                             "and close the disk."
                             "<p><em>In the case of DVD+RW and DVD-RW restricted overwrite media "
                             "K3b will not actually create multiple sessions but grow the "
                             "file system to include the new data.</em>") );
}


K3b::DataMultiSessionCombobox::~DataMultiSessionCombobox()
{
}


void K3b::DataMultiSessionCombobox::init( bool force )
{
    m_forceNoMultiSession = force;

    clear();

    insertItem( s_autoIndex, i18n("Auto") );
    insertItem( s_noneIndex, i18n("No Multisession") );
    if( !m_forceNoMultiSession ) {
        insertItem( s_startIndex, i18n("Start Multisession") );
        insertItem( s_continueIndex, i18n("Continue Multisession") );
        insertItem( s_finishIndex, i18n("Finish Multisession") );
    }
}


K3b::DataDoc::MultiSessionMode K3b::DataMultiSessionCombobox::multiSessionMode() const
{
    switch( currentIndex() ) {
    case s_noneIndex:
        return K3b::DataDoc::NONE;
    case s_startIndex:
        return K3b::DataDoc::START;
    case s_continueIndex:
        return K3b::DataDoc::CONTINUE;
    case s_finishIndex:
        return K3b::DataDoc::FINISH;
    default:
        return K3b::DataDoc::AUTO;
    }
}


void K3b::DataMultiSessionCombobox::saveConfig( KConfigGroup c )
{
    QString s;
    switch( currentIndex() ) {
    case s_autoIndex:
        s = "auto";
        break;
    case s_noneIndex:
        s = "none";
        break;
    case s_startIndex:
        s = "start";
        break;
    case s_continueIndex:
        s = "continue";
        break;
    case s_finishIndex:
        s = "finish";
        break;
    }

    c.writeEntry( "multisession mode", s );
}


void K3b::DataMultiSessionCombobox::loadConfig( const KConfigGroup& c )
{
    QString s = c.readEntry( "multisession mode" );
    if( s == "none" )
        setMultiSessionMode( K3b::DataDoc::NONE );
    else if( s == "start" )
        setMultiSessionMode( K3b::DataDoc::START );
    else if( s == "continue" )
        setMultiSessionMode( K3b::DataDoc::CONTINUE );
    else if( s == "finish" )
        setMultiSessionMode( K3b::DataDoc::FINISH );
    else
        setMultiSessionMode( K3b::DataDoc::AUTO );
}


void K3b::DataMultiSessionCombobox::setMultiSessionMode( K3b::DataDoc::MultiSessionMode m )
{
    switch( m ) {
    case K3b::DataDoc::AUTO:
        setCurrentIndex( s_autoIndex );
        break;
    case K3b::DataDoc::NONE:
        setCurrentIndex( s_noneIndex );
        break;
    case K3b::DataDoc::START:
        if( !m_forceNoMultiSession )
            setCurrentIndex( s_startIndex );
        break;
    case K3b::DataDoc::CONTINUE:
        if( !m_forceNoMultiSession )
            setCurrentIndex( s_continueIndex );
        break;
    case K3b::DataDoc::FINISH:
        if( !m_forceNoMultiSession )
            setCurrentIndex( s_finishIndex );
        break;
    }
}


void K3b::DataMultiSessionCombobox::setForceNoMultisession( bool f )
{
    if( f != m_forceNoMultiSession ) {
        K3b::DataDoc::MultiSessionMode m = multiSessionMode();
        init( f );
        setMultiSessionMode( m );
    }
}


