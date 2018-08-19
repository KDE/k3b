/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2010 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bdataadvancedimagesettingsdialog.h"
#include "k3bisooptions.h"

#include <KLocalizedString>

#include <QDebug>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QRadioButton>


K3b::DataAdvancedImageSettingsDialog::DataAdvancedImageSettingsDialog( QWidget* parent )
    : QDialog( parent )
{
    setupUi( this );

    setWindowTitle(i18n("Custom Data Project Filesystems"));
    setModal(true);

    connect( m_checkRockRidge, SIGNAL(toggled(bool)), m_groupRockRidgeSettings, SLOT(setEnabled(bool)) );
    connect( m_checkJoliet, SIGNAL(toggled(bool)), m_groupJolietSettings, SLOT(setEnabled(bool)) );
}


K3b::DataAdvancedImageSettingsDialog::~DataAdvancedImageSettingsDialog()
{
}


void K3b::DataAdvancedImageSettingsDialog::load( const K3b::IsoOptions& options )
{
    m_checkRockRidge->setChecked( options.createRockRidge() );
    m_checkJoliet->setChecked( options.createJoliet() );
    m_checkUdf->setChecked( options.createUdf() );

    switch( options.ISOLevel() ) {
    case 1:
        m_radioIsoLevel1->setChecked(true);
        break;
    case 2:
        m_radioIsoLevel2->setChecked(true);
        break;
    case 3:
        m_radioIsoLevel3->setChecked(true);
        break;
    }

    m_checkPreservePermissions->setChecked( options.preserveFilePermissions() );

    // RR settings
    m_checkCreateTransTbl->setChecked( options.createTRANS_TBL() );
    m_checkHideTransTbl->setChecked( options.hideTRANS_TBL() );

    // iso9660 settings
    m_checkAllowUntranslatedFilenames->setChecked( options.ISOuntranslatedFilenames() );
    m_checkAllow31CharFilenames->setChecked( options.ISOallow31charFilenames() );
    m_checkAllowMaxLengthFilenames->setChecked( options.ISOmaxFilenameLength() );
    m_checkAllowBeginningPeriod->setChecked( options.ISOallowPeriodAtBegin() );
    m_checkAllowFullAscii->setChecked( options.ISOrelaxedFilenames() );
    m_checkOmitVersionNumbers->setChecked( options.ISOomitVersionNumbers() );
    m_checkOmitTrailingPeriod->setChecked( options.ISOomitTrailingPeriod() );
    m_checkAllowOther->setChecked( options.ISOnoIsoTranslate() );
    m_checkAllowMultiDot->setChecked( options.ISOallowMultiDot() );
    m_checkAllowLowercaseCharacters->setChecked( options.ISOallowLowercase() );

    // joliet settings
    m_checkJolietLong->setChecked( options.jolietLong() );

    // misc (FIXME: should not be here)
    m_checkDoNotCacheInodes->setChecked( options.doNotCacheInodes() );
    m_checkDoNotImportSession->setChecked( options.doNotImportSession() );
}


void K3b::DataAdvancedImageSettingsDialog::save( K3b::IsoOptions& options )
{
    options.setCreateRockRidge( m_checkRockRidge->isChecked() );
    options.setCreateJoliet( m_checkJoliet->isChecked() );
    options.setCreateUdf( m_checkUdf->isChecked() );

    // save iso-level
    if( m_radioIsoLevel3->isChecked() )
        options.setISOLevel( 3 );
    else if( m_radioIsoLevel2->isChecked() )
        options.setISOLevel( 2 );
    else
        options.setISOLevel( 1 );

    options.setPreserveFilePermissions( m_checkPreservePermissions->isChecked() );

    options.setCreateTRANS_TBL( m_checkCreateTransTbl->isChecked() );
    options.setHideTRANS_TBL( m_checkHideTransTbl->isChecked() );
    options.setISOuntranslatedFilenames( m_checkAllowUntranslatedFilenames->isChecked() );
    options.setISOallow31charFilenames( m_checkAllow31CharFilenames->isChecked() );
    options.setISOmaxFilenameLength( m_checkAllowMaxLengthFilenames->isChecked() );
    options.setISOallowPeriodAtBegin( m_checkAllowBeginningPeriod->isChecked() );
    options.setISOrelaxedFilenames( m_checkAllowFullAscii->isChecked() );
    options.setISOomitVersionNumbers( m_checkOmitVersionNumbers->isChecked() );
    options.setISOomitTrailingPeriod( m_checkOmitTrailingPeriod->isChecked() );
    options.setISOnoIsoTranslate( m_checkAllowOther->isChecked() );
    options.setISOallowMultiDot( m_checkAllowMultiDot->isChecked() );
    options.setISOallowLowercase( m_checkAllowLowercaseCharacters->isChecked() );
    //  o.setFollowSymbolicLinks( m_checkFollowSymbolicLinks->isChecked() );
    options.setJolietLong( m_checkJolietLong->isChecked() );
    options.setDoNotCacheInodes( m_checkDoNotCacheInodes->isChecked() );
    options.setDoNotImportSession( m_checkDoNotImportSession->isChecked() );
}


