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

#include "k3bwritingmodewidget.h"
#include "k3bmediacache.h"
#include "k3bapplication.h"

#include "k3bglobals.h"

#include <KConfig>
#include <KLocalizedString>

#include <QToolTip>

static const KLocalizedString s_autoHelp =
    ki18n("Let K3b select the best-suited mode. This is the recommended selection.");
static const KLocalizedString s_daoHelp =
    ki18n("<em>Disk At Once</em> or more properly <em>Session At Once</em>. "
          "The laser is never turned off while writing the CD or DVD. "
          "This is the preferred mode to write audio CDs since it allows "
          "pregaps other than 2 seconds. Not all writers support DAO.<br>"
          "DVD-R(W)s written in DAO provide the best DVD-Video compatibility.");
static const KLocalizedString s_taoHelp =
    ki18n("<em>Track At Once</em> should be supported by every CD writer. "
          "The laser will be turned off after every track.<br>"
          "Most CD writers need this mode for writing multisession CDs.");
// TODO: add something like: "No CD-TEXT writing in TAO mode."

static const KLocalizedString s_rawHelp =
    ki18n("RAW writing mode. The error correction data is created by the "
          "software instead of the writer device.<br>"
          "Try this if your CD writer fails to write in DAO and TAO.");
static const KLocalizedString s_seqHelp =
    ki18n("Incremental sequential is the default writing mode for DVD-R(W). "
          "It allows multisession DVD-R(W)s. It only applies to DVD-R(W).");
static const KLocalizedString s_ovwHelp =
    ki18n("Restricted Overwrite allows one to use a DVD-RW just like a DVD-RAM "
          "or a DVD+RW. The media may just be overwritten. It is not possible "
          "to write multisession DVD-RWs in this mode but K3b uses growisofs "
          "to grow an ISO 9660 filesystem within the first session, thus allowing "
          "new files to be added to an already burned disk.");


class K3b::WritingModeWidget::Private
{
public:
    // modes set via setSupportedModes
    WritingModes supportedModes;

    // filtered modes
    WritingModes selectedModes;

    Device::Device* device;

    void _k_writingModeChanged( int mode ) {
        emit q->writingModeChanged( WritingMode( mode ) );
    }

    WritingModeWidget* q;
};


K3b::WritingModeWidget::WritingModeWidget( WritingModes modes, QWidget* parent )
    : IntMapComboBox( parent )
{
    init();
    setSupportedModes( modes );
}


K3b::WritingModeWidget::WritingModeWidget( QWidget* parent )
    : IntMapComboBox( parent )
{
    init();
    setSupportedModes( WritingModeSao | WritingModeTao | WritingModeRaw );   // default: support all CD-R(W) modes
}


K3b::WritingModeWidget::~WritingModeWidget()
{
    delete d;
}


void K3b::WritingModeWidget::init()
{
    d = new Private();
    d->q = this;
    d->device = 0;

    connect( this, SIGNAL(valueChanged(int)), this, SLOT(_k_writingModeChanged(int)) );

    setToolTip( i18n("Select the writing mode to use") );
    addGlobalWhatsThisText( "<p><b>" + i18n("Writing mode") + "</b></p>",
                            i18n("Be aware that the writing mode is ignored when writing DVD+R(W) and BD-R(E) since "
                                 "there is only one way to write them.")
                            + "<p><i>"
                            + i18n("The selection of writing modes depends on the inserted burning medium.")
                            + "</i>" );
}


K3b::WritingMode K3b::WritingModeWidget::writingMode() const
{
    return WritingMode( selectedValue() );
}


K3b::WritingModes K3b::WritingModeWidget::supportedWritingModes() const
{
    return d->selectedModes;
}


void K3b::WritingModeWidget::setWritingMode( WritingMode m )
{
    if( m & d->selectedModes ) {
        setSelectedValue( m );
    }
    else {
        setSelectedValue( WritingModeAuto );
    }
}


void K3b::WritingModeWidget::setSupportedModes( WritingModes m )
{
    d->supportedModes = m|WritingModeAuto;  // we always support the Auto mode
    updateModes();
}


void K3b::WritingModeWidget::setDevice( Device::Device* dev )
{
    d->device = dev;
    updateModes();
}


void K3b::WritingModeWidget::updateModes()
{
    // save current mode
    int currentMode = writingMode();

    clear();

    if( d->device )
        d->selectedModes = d->supportedModes & d->device->writingModes();
    else
        d->selectedModes = d->supportedModes;

    insertItem( WritingModeAuto, i18n("Auto"), s_autoHelp.toString() );
    if( d->selectedModes & WritingModeSao )
        insertItem( WritingModeSao, i18n("DAO"), s_daoHelp.toString() );
    if( d->selectedModes & WritingModeTao )
        insertItem( WritingModeTao, i18n("TAO"), s_taoHelp.toString() );
    if( d->selectedModes & WritingModeRaw )
        insertItem( WritingModeRaw, i18n("RAW"), s_rawHelp.toString() );
    if( d->selectedModes & WritingModeRestrictedOverwrite )
        insertItem( WritingModeRestrictedOverwrite, i18n("Restricted Overwrite"), s_ovwHelp.toString() );
    if( d->selectedModes & WritingModeIncrementalSequential )
        insertItem( WritingModeIncrementalSequential, i18n("Incremental"), s_seqHelp.toString() );

    setWritingMode( currentMode != -1 ? WritingMode( currentMode ) : WritingModeAuto );
}


void K3b::WritingModeWidget::saveConfig( KConfigGroup c )
{
    switch( writingMode() ) {
    case WritingModeSao:
        c.writeEntry( "writing_mode", "dao" );
        break;
    case WritingModeTao:
        c.writeEntry( "writing_mode", "tao" );
        break;
    case WritingModeRaw:
        c.writeEntry( "writing_mode", "raw" );
        break;
    case WritingModeIncrementalSequential:
        c.writeEntry( "writing_mode", "incremental" );
        break;
    case WritingModeRestrictedOverwrite:
        c.writeEntry( "writing_mode", "overwrite" );
        break;
    default:
        c.writeEntry( "writing_mode", "auto" );
        break;
    }
}

void K3b::WritingModeWidget::loadConfig( const KConfigGroup& c )
{
    QString mode = c.readEntry( "writing_mode" );
    if ( mode == "dao" )
        setWritingMode( WritingModeSao );
    else if( mode == "tao" )
        setWritingMode( WritingModeTao );
    else if( mode == "raw" )
        setWritingMode( WritingModeRaw );
    else if( mode == "incremental" )
        setWritingMode( WritingModeIncrementalSequential );
    else if( mode == "overwrite" )
        setWritingMode( WritingModeRestrictedOverwrite );
    else
        setWritingMode( WritingModeAuto );
}


void K3b::WritingModeWidget::determineSupportedModesFromMedium( const Medium& m )
{
    WritingModes modes = WritingModeAuto;

    if( m.diskInfo().mediaType() & (Device::MEDIA_CD_R|Device::MEDIA_CD_RW) ) {
        modes |= WritingModeTao;
        if( m.device()->supportsWritingMode( Device::WRITINGMODE_SAO ) )
            modes |= WritingModeSao;
        if( m.device()->supportsWritingMode( Device::WRITINGMODE_RAW ) )
            modes |= WritingModeRaw;
    }

    if( m.diskInfo().mediaType() & Device::MEDIA_DVD_MINUS_ALL ) {
        modes |= WritingModeSao;
        if ( !k3bcore->deviceBlocked( m.device() ) )
             if( m.device()->featureCurrent( Device::FEATURE_INCREMENTAL_STREAMING_WRITABLE ) != 0 )
                 modes |= WritingModeIncrementalSequential;
    }

    if( m.diskInfo().mediaType() & (Device::MEDIA_DVD_RW|
                                    Device::MEDIA_DVD_RW_SEQ|
                                    Device::MEDIA_DVD_RW_OVWR) ) {
        modes |= WritingModeRestrictedOverwrite;
    }

    setSupportedModes( modes );
    setDevice( m.device() );
}


void K3b::WritingModeWidget::determineSupportedModesFromMedium( K3b::Device::Device* dev )
{
    if( dev )
        determineSupportedModesFromMedium( k3bappcore->mediaCache()->medium( dev ) );
    else
        determineSupportedModesFromMedium( Medium() ); // no medium
}

#include "moc_k3bwritingmodewidget.cpp"
