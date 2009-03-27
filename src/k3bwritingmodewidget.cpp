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

#include <k3bglobals.h>

#include <klocale.h>
#include <kconfig.h>

#include <qtooltip.h>
#include <q3whatsthis.h>

static const QString s_autoHelp = i18n("Let K3b select the best-suited mode. This is the recommended selection.");
static const QString s_daoHelp = i18n("<em>Disk At Once</em> or more properly <em>Session At Once</em>. "
                                      "The laser is never turned off while writing the CD or DVD. "
                                      "This is the preferred mode to write audio CDs since it allows "
                                      "pregaps other than 2 seconds. Not all writers support DAO.<br>"
                                      "DVD-R(W)s written in DAO provide the best DVD-Video compatibility.");
static const QString s_taoHelp = i18n("<em>Track At Once</em> should be supported by every CD writer. "
                                      "The laser will be turned off after every track.<br>"
                                      "Most CD writers need this mode for writing multisession CDs.");
// TODO: add something like: "No CD-TEXT writing in TAO mode."

static const QString s_rawHelp = i18n("RAW writing mode. The error correction data is created by the "
                                      "software instead of the writer device.<br>"
                                      "Try this if your CD writer fails to write in DAO and TAO.");
static const QString s_seqHelp = i18n("Incremental sequential is the default writing mode for DVD-R(W). "
                                      "It allows multisession DVD-R(W)s. It only applies to DVD-R(W).");
static const QString s_ovwHelp = i18n("Restricted Overwrite allows to use a DVD-RW just like a DVD-RAM "
                                      "or a DVD+RW. The media may just be overwritten. It is not possible "
                                      "to write multisession DVD-RWs in this mode but K3b uses growisofs "
                                      "to grow an ISO9660 filesystem within the first session, thus allowing "
                                      "new files to be added to an already burned disk.");


class K3b::WritingModeWidget::Private
{
public:
    // modes set via setSupportedModes
    K3b::WritingModes supportedModes;

    // filtered modes
    K3b::WritingModes selectedModes;

    K3b::Device::Device* device;

    void _k_writingModeChanged( int mode ) {
        emit q->writingModeChanged( K3b::WritingMode( mode ) );
    }

    K3b::WritingModeWidget* q;
};


K3b::WritingModeWidget::WritingModeWidget( K3b::WritingModes modes, QWidget* parent )
    : K3b::IntMapComboBox( parent )
{
    init();
    setSupportedModes( modes );
}


K3b::WritingModeWidget::WritingModeWidget( QWidget* parent )
    : K3b::IntMapComboBox( parent )
{
    init();
    setSupportedModes( K3b::WRITING_MODE_DAO | K3b::WRITING_MODE_TAO | K3b::WRITING_MODE_RAW );   // default: support all CD-R(W) modes
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
                            i18n("Be aware that the writing mode is ignored when writing DVD+R(W) since "
                                 "there is only one way to write them.")
                            + "<p><i>"
                            + i18n("The selection of writing modes depends on the inserted burning medium.")
                            + "</i>" );
}


K3b::WritingMode K3b::WritingModeWidget::writingMode() const
{
    return K3b::WritingMode( selectedValue() );
}


K3b::WritingModes K3b::WritingModeWidget::supportedWritingModes() const
{
    return d->selectedModes;
}


void K3b::WritingModeWidget::setWritingMode( K3b::WritingMode m )
{
    if( m & d->selectedModes ) {
        setSelectedValue( m );
    }
    else {
        setSelectedValue( K3b::WRITING_MODE_AUTO );
    }
}


void K3b::WritingModeWidget::setSupportedModes( K3b::WritingModes m )
{
    d->supportedModes = m|K3b::WRITING_MODE_AUTO;  // we always support the Auto mode
    updateModes();
}


void K3b::WritingModeWidget::setDevice( K3b::Device::Device* dev )
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

    insertItem( K3b::WRITING_MODE_AUTO, i18n("Auto"), s_autoHelp );
    if( d->selectedModes & K3b::WRITING_MODE_DAO )
        insertItem( K3b::WRITING_MODE_DAO, i18n("DAO"), s_daoHelp );
    if( d->selectedModes & K3b::WRITING_MODE_TAO )
        insertItem( K3b::WRITING_MODE_TAO, i18n("TAO"), s_taoHelp );
    if( d->selectedModes & K3b::WRITING_MODE_RAW )
        insertItem( K3b::WRITING_MODE_RAW, i18n("RAW"), s_rawHelp );
    if( d->selectedModes & K3b::WRITING_MODE_RES_OVWR )
        insertItem( K3b::WRITING_MODE_RES_OVWR, i18n("Restricted Overwrite"), s_ovwHelp );
    if( d->selectedModes & K3b::WRITING_MODE_INCR_SEQ )
        insertItem( K3b::WRITING_MODE_INCR_SEQ, i18n("Incremental"), s_seqHelp );

    setWritingMode( currentMode != -1 ? K3b::WritingMode( currentMode ) : K3b::WRITING_MODE_AUTO );
}


void K3b::WritingModeWidget::saveConfig( KConfigGroup c )
{
    switch( writingMode() ) {
    case K3b::WRITING_MODE_DAO:
        c.writeEntry( "writing_mode", "dao" );
        break;
    case K3b::WRITING_MODE_TAO:
        c.writeEntry( "writing_mode", "tao" );
        break;
    case K3b::WRITING_MODE_RAW:
        c.writeEntry( "writing_mode", "raw" );
        break;
    case K3b::WRITING_MODE_INCR_SEQ:
        c.writeEntry( "writing_mode", "incremental" );
        break;
    case K3b::WRITING_MODE_RES_OVWR:
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
        setWritingMode( K3b::WRITING_MODE_DAO );
    else if( mode == "tao" )
        setWritingMode( K3b::WRITING_MODE_TAO );
    else if( mode == "raw" )
        setWritingMode( K3b::WRITING_MODE_RAW );
    else if( mode == "incremental" )
        setWritingMode( K3b::WRITING_MODE_INCR_SEQ );
    else if( mode == "overwrite" )
        setWritingMode( K3b::WRITING_MODE_RES_OVWR );
    else
        setWritingMode( K3b::WRITING_MODE_AUTO );
}


void K3b::WritingModeWidget::determineSupportedModesFromMedium( const K3b::Medium& m )
{
    K3b::WritingModes modes = 0;

    if( m.diskInfo().mediaType() & (K3b::Device::MEDIA_CD_R|K3b::Device::MEDIA_CD_RW) ) {
        modes |= K3b::WRITING_MODE_TAO;
        if( m.device()->supportsWritingMode( K3b::Device::WRITINGMODE_SAO ) )
            modes |= K3b::WRITING_MODE_DAO;
        if( m.device()->supportsWritingMode( K3b::Device::WRITINGMODE_RAW ) )
            modes |= K3b::WRITING_MODE_RAW;
    }

    if( m.diskInfo().mediaType() & K3b::Device::MEDIA_DVD_MINUS_ALL ) {
        modes |= K3b::WRITING_MODE_DAO;
        if ( !k3bcore->deviceBlocked( m.device() ) )
             if( m.device()->featureCurrent( K3b::Device::FEATURE_INCREMENTAL_STREAMING_WRITABLE ) != 0 )
                 modes |= K3b::WRITING_MODE_INCR_SEQ;
    }

    if( m.diskInfo().mediaType() & (K3b::Device::MEDIA_DVD_RW|
                                    K3b::Device::MEDIA_DVD_RW_SEQ|
                                    K3b::Device::MEDIA_DVD_RW_OVWR) )
        modes |= K3b::WRITING_MODE_RES_OVWR;

#ifdef __GNUC__
#warning FIXME: add Blu-Ray media
#endif

    setSupportedModes( modes );
    setDevice( m.device() );
}


void K3b::WritingModeWidget::determineSupportedModesFromMedium( K3b::Device::Device* dev )
{
    if( dev )
        determineSupportedModesFromMedium( k3bappcore->mediaCache()->medium( dev ) );
    else
        determineSupportedModesFromMedium( K3b::Medium() ); // no medium
}

#include "k3bwritingmodewidget.moc"
