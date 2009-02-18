/*
 *
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

#include "k3blameencoderconfigwidget.h"
#include "k3blamemanualsettingsdialog.h"
#include "k3blametyes.h"

#include <qlayout.h>
#include <qradiobutton.h>
#include <qcheckbox.h>
#include <qspinbox.h>
#include <qtextcodec.h>
#include <qfile.h>
#include <qslider.h>
#include <qlabel.h>
#include <qpushbutton.h>

#include <k3bcore.h>

#include <kconfig.h>
#include <kdebug.h>
#include <KAboutData>


K3B_EXPORT_PLUGIN_CONFIG_WIDGET( kcm_k3blameencoder, K3bLameEncoderSettingsWidget )


K3bLameEncoderSettingsWidget::K3bLameEncoderSettingsWidget( QWidget* parent, const QVariantList& args )
    : K3bPluginConfigWidget( parent, args )
{
    setupUi( this );

    m_sliderQuality->setRange( 0, 9 );
    m_spinEncoderQuality->setRange( 0, 9, true );

    m_manualSettingsDialog = new K3bLameManualSettingsDialog( this );
    for( int i = 0; s_lame_bitrates[i]; ++i )
        m_manualSettingsDialog->m_comboMaximumBitrate->addItem( i18n("%1 kbps" , s_lame_bitrates[i]) );

    for( int i = 0; s_lame_bitrates[i]; ++i )
        m_manualSettingsDialog->m_comboMinimumBitrate->addItem( i18n("%1 kbps" , s_lame_bitrates[i]) );

    for( int i = 0; s_lame_bitrates[i]; ++i )
        m_manualSettingsDialog->m_comboConstantBitrate->addItem( i18n("%1 kbps" , s_lame_bitrates[i]) );


    // TODO: add whatsthis help for the quality level.
    //  QString qualityLevelWhatsThis = i18n("<p>");

    connect( m_buttonManualSettings, SIGNAL(clicked()),
             this, SLOT(slotShowManualSettings()) );
    connect( m_sliderQuality, SIGNAL(valueChanged(int)),
             this, SLOT(slotQualityLevelChanged(int)) );

    updateManualSettingsLabel();
    slotQualityLevelChanged( 5 );

    connect( m_radioQualityLevel, SIGNAL(toggled( bool )), this, SLOT( changed() ) );
    connect( m_sliderQuality, SIGNAL(valueChanged( int )), this, SLOT( changed() ) );
    connect( m_radioManual, SIGNAL(toggled( bool )), this, SLOT( changed() ) );
    connect( m_spinEncoderQuality, SIGNAL(valueChanged( int )), this, SLOT( changed() ) );
    connect( m_checkCopyright, SIGNAL(toggled( bool )), this, SLOT( changed() ) );
    connect( m_checkOriginal, SIGNAL(toggled( bool )), this, SLOT( changed() ) );
    connect( m_checkISO, SIGNAL(toggled( bool )), this, SLOT( changed() ) );
    connect( m_checkError, SIGNAL(toggled( bool )), this, SLOT( changed() ) );
}


K3bLameEncoderSettingsWidget::~K3bLameEncoderSettingsWidget()
{
}


void K3bLameEncoderSettingsWidget::slotShowManualSettings()
{
    // save current settings for proper cancellation
    bool constant = m_manualSettingsDialog->m_radioConstantBitrate->isChecked();
    int constBitrate = m_manualSettingsDialog->m_comboConstantBitrate->currentIndex();
    int max = m_manualSettingsDialog->m_comboMaximumBitrate->currentIndex();
    int min = m_manualSettingsDialog->m_comboMinimumBitrate->currentIndex();
    int av = m_manualSettingsDialog->m_spinAverageBitrate->value();
    int mode = m_manualSettingsDialog->m_comboMode->currentIndex();

    if( m_manualSettingsDialog->exec() == QDialog::Rejected ) {
        m_manualSettingsDialog->m_radioConstantBitrate->setChecked( constant );
        m_manualSettingsDialog->m_comboConstantBitrate->setCurrentIndex( constBitrate );
        m_manualSettingsDialog->m_comboMaximumBitrate->setCurrentIndex( max );
        m_manualSettingsDialog->m_comboMinimumBitrate->setCurrentIndex( min );
        m_manualSettingsDialog->m_spinAverageBitrate->setValue( av );
        m_manualSettingsDialog->m_comboMode->setCurrentIndex( mode );
    }
    else
        updateManualSettingsLabel();

    changed();
}


void K3bLameEncoderSettingsWidget::updateManualSettingsLabel()
{
    if( m_manualSettingsDialog->m_radioConstantBitrate->isChecked() )
        m_labelManualSettings->setText( i18n("Constant Bitrate: %1 kbps (%2)",
                                             s_lame_bitrates[m_manualSettingsDialog->m_comboConstantBitrate->currentIndex()],
                                             i18n(s_lame_mode_strings[m_manualSettingsDialog->m_comboMode->currentIndex()])) );
    else
        m_labelManualSettings->setText( i18n("Variable Bitrate (%1)",
                                        i18n(s_lame_mode_strings[m_manualSettingsDialog->m_comboMode->currentIndex()])) );
}


void K3bLameEncoderSettingsWidget::slotQualityLevelChanged( int val )
{
    m_labelQualityLevel->setText( i18n(s_lame_preset_strings[val]) );
}


void K3bLameEncoderSettingsWidget::load()
{
    kDebug();

    Q_ASSERT( k3bcore );
    KSharedConfig::Ptr c = KGlobal::config();
    KConfigGroup grp(c, "K3bLameEncoderPlugin" );

    QString mode = grp.readEntry( "Mode", "stereo" );
    if( mode == "stereo" )
        m_manualSettingsDialog->m_comboMode->setCurrentIndex( 0 );
    else if( mode == "joint" )
        m_manualSettingsDialog->m_comboMode->setCurrentIndex( 1 );
    else // mono
        m_manualSettingsDialog->m_comboMode->setCurrentIndex( 2 );

    bool manual = grp.readEntry( "Manual Bitrate Settings", false );
    if( manual )
        m_radioManual->setChecked(true);
    else
        m_radioQualityLevel->setChecked(true);

    if(grp.readEntry( "VBR", false ) )
        m_manualSettingsDialog->m_radioVariableBitrate->setChecked( true );
    else
        m_manualSettingsDialog->m_radioConstantBitrate->setChecked( true );

    m_manualSettingsDialog->m_comboConstantBitrate->setCurrentItem( i18n("%1 kbps",grp.readEntry( "Constant Bitrate", 128 )) );
    m_manualSettingsDialog->m_comboMaximumBitrate->setCurrentItem( i18n("%1 kbps",grp.readEntry( "Maximum Bitrate", 224 )) );
    m_manualSettingsDialog->m_comboMinimumBitrate->setCurrentItem( i18n("%1 kbps",grp.readEntry( "Minimum Bitrate", 32 )) );
    m_manualSettingsDialog->m_spinAverageBitrate->setValue( grp.readEntry( "Average Bitrate", 128) );

    m_manualSettingsDialog->m_checkBitrateMaximum->setChecked( grp.readEntry( "Use Maximum Bitrate", false ) );
    m_manualSettingsDialog->m_checkBitrateMinimum->setChecked( grp.readEntry( "Use Minimum Bitrate", false ) );
    m_manualSettingsDialog->m_checkBitrateAverage->setChecked( grp.readEntry( "Use Average Bitrate", true ) );

    m_sliderQuality->setValue( grp.readEntry( "Quality Level", 5 ) );

    m_checkCopyright->setChecked( grp.readEntry( "Copyright", false ) );
    m_checkOriginal->setChecked( grp.readEntry( "Original", true ) );
    m_checkISO->setChecked( grp.readEntry( "ISO compliance", false ) );
    m_checkError->setChecked( grp.readEntry( "Error Protection", false ) );

    // default to 2 which is the same as the -h lame option
    m_spinEncoderQuality->setValue( grp.readEntry( "Encoder Quality", 7 ) );

    updateManualSettingsLabel();
}


void K3bLameEncoderSettingsWidget::save()
{
    kDebug();

    Q_ASSERT( k3bcore );
    KSharedConfig::Ptr c = KGlobal::config();
    KConfigGroup grp(c, "K3bLameEncoderPlugin" );

    QString mode;
    switch( m_manualSettingsDialog->m_comboMode->currentIndex() ) {
    case 0:
        mode = "stereo";
        break;
    case 1:
        mode = "joint";
        break;
    case 2:
        mode = "mono";
        break;
    }
    grp.writeEntry( "Mode", mode );

    grp.writeEntry( "Manual Bitrate Settings", m_radioManual->isChecked() );

    grp.writeEntry( "VBR", !m_manualSettingsDialog->m_radioConstantBitrate->isChecked() );
    grp.writeEntry( "Constant Bitrate", m_manualSettingsDialog->m_comboConstantBitrate->currentText().left(3).toInt() );
    grp.writeEntry( "Maximum Bitrate", m_manualSettingsDialog->m_comboMaximumBitrate->currentText().left(3).toInt() );
    grp.writeEntry( "Minimum Bitrate", m_manualSettingsDialog->m_comboMinimumBitrate->currentText().left(3).toInt() );
    grp.writeEntry( "Average Bitrate", m_manualSettingsDialog->m_spinAverageBitrate->value() );

    grp.writeEntry( "Use Maximum Bitrate", m_manualSettingsDialog->m_checkBitrateMaximum->isChecked() );
    grp.writeEntry( "Use Minimum Bitrate", m_manualSettingsDialog->m_checkBitrateMinimum->isChecked() );
    grp.writeEntry( "Use Average Bitrate", m_manualSettingsDialog->m_checkBitrateAverage->isChecked() );

    grp.writeEntry( "Quality Level", m_sliderQuality->value() );

    grp.writeEntry( "Copyright", m_checkCopyright->isChecked() );
    grp.writeEntry( "Original", m_checkOriginal->isChecked() );
    grp.writeEntry( "ISO compliance", m_checkISO->isChecked() );
    grp.writeEntry( "Error Protection", m_checkError->isChecked() );

    // default to 2 which is the same as the -h lame option
    grp.writeEntry( "Encoder Quality", m_spinEncoderQuality->value() );

    grp.sync();
}


void K3bLameEncoderSettingsWidget::defaults()
{
    kDebug();
    // FIXME
}

#include "k3blameencoderconfigwidget.moc"
