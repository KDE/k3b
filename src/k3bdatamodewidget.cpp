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


#include "k3bdatamodewidget.h"
#include "k3bglobals.h"

#include <KConfig>
#include <KLocalizedString>
#include <QToolTip>

static const int s_autoIndex = 0;
static const int s_mode1Index = 1;
static const int s_mode2Index = 2;


K3b::DataModeWidget::DataModeWidget( QWidget* parent )
    : QComboBox( parent )
{
    insertItem( s_autoIndex, i18n("Auto") );
    insertItem( s_mode1Index, i18n("Mode1") );
    insertItem( s_mode2Index, i18n("Mode2") );

    this->setToolTip(i18n("Select the mode for the data-track") );
    this->setWhatsThis( i18n("<p><b>Data Mode</b>"
                             "<p>Data tracks may be written in two different modes:</p>"
                             "<p><b>Auto</b><br>"
                             "Let K3b select the best suited data mode.</p>"
                             "<p><b>Mode 1</b><br>"
                             "This is the <em>original</em> writing mode as introduced in the "
                             "<em>Yellow Book</em> standard. It is the preferred mode when writing "
                             "pure data CDs.</p>"
                             "<p><b>Mode 2</b><br>"
                             "To be exact <em>XA Mode 2 Form 1</em>, but since the "
                             "other modes are rarely used it is common to refer to it as <em>Mode 2</em>.</p>"
                             "<p><b>Be aware:</b> Do not mix different modes on one CD. "
                             "Some older drives may have problems reading mode 1 multisession CDs.") );
}


K3b::DataModeWidget::~DataModeWidget()
{
}


int K3b::DataModeWidget::dataMode() const
{
    if( currentIndex() == s_autoIndex )
        return K3b::DataModeAuto;
    else if( currentIndex() == s_mode1Index )
        return K3b::DataMode1;
    else
        return K3b::DataMode2;
}


void K3b::DataModeWidget::setDataMode( int mode )
{
    if( mode == K3b::DataMode1 )
        setCurrentIndex( s_mode1Index );
    else if( mode == K3b::DataMode2 )
        setCurrentIndex( s_mode2Index );
    else
        setCurrentIndex( s_autoIndex );
}


void K3b::DataModeWidget::saveConfig( KConfigGroup c )
{
    QString datamode;
    if( dataMode() == K3b::DataMode1 )
        datamode = "mode1";
    else if( dataMode() == K3b::DataMode2 )
        datamode = "mode2";
    else
        datamode = "auto";
    c.writeEntry( "data_track_mode", datamode );
}


void K3b::DataModeWidget::loadConfig( const KConfigGroup& c )
{
    QString datamode = c.readEntry( "data_track_mode" );
    if( datamode == "mode1" )
        setDataMode( K3b::DataMode1 );
    else if( datamode == "mode2" )
        setDataMode( K3b::DataMode2 );
    else
        setDataMode( K3b::DataModeAuto );
}


