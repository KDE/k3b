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

#include "k3boggvorbisencoderconfigwidget.h"
#include "k3boggvorbisencoderdefaults.h"
#include "k3bcore.h"

#include <KConfigCore/KConfig>
#include <KConfigCore/KSharedConfig>
#include <QtCore/QDebug>
#include <KI18n/KLocalizedString>
#include <KNumInput>

#include <QCheckBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLayout>
#include <QLCDNumber>
#include <QRadioButton>
#include <QSlider>
#include <QtWidgets/QToolTip>



K3B_EXPORT_PLUGIN_CONFIG_WIDGET( kcm_k3bexternalencoder, K3bOggVorbisEncoderSettingsWidget )

namespace {

    // quality levels -1 to 10 map to 0 to 11
    const int s_rough_average_quality_level_bitrates[] = {
        45,
        64,
        80,
        96,
        112,
        128,
        160,
        192,
        224,
        256,
        320,
        400
    };

} // namespace

K3bOggVorbisEncoderSettingsWidget::K3bOggVorbisEncoderSettingsWidget( QWidget* parent, const QVariantList& args )
    : K3b::PluginConfigWidget( parent, args )
{
    w = new base_K3bOggVorbisEncoderSettingsWidget( this );

    QString ttQuality = i18n("Controls the quality of the encoded files.");
    QString wsQuality = i18n("<p>Vorbis' audio quality is not best measured in kilobits per second, "
                             "but on a scale from -1 to 10 called <em>quality</em>."
                             "<p>For now, quality -1 is roughly equivalent to 45kbps average, "
                             "5 is roughly 160kbps, and 10 gives about 400kbps. "
                             "Most people seeking very-near-CD-quality audio encode at a quality of 5 or, "
                             "for lossless stereo coupling, 6. The quality 3 gives, at "
                             "approximately 110kbps a smaller filesize and significantly better fidelity "
                             "than .mp3 compression at 128kbps."
                             "<p><em>This explanation is based on the one from the www.vorbis.com FAQ.</em>");

    w->m_radioQualityLevel->setToolTip( ttQuality );
    w->m_labelQualityLevel->setToolTip( ttQuality );
    w->m_slideQualityLevel->setToolTip( ttQuality );
    w->m_radioQualityLevel->setWhatsThis( wsQuality );
    w->m_labelQualityLevel->setWhatsThis( wsQuality );
    w->m_slideQualityLevel->setWhatsThis( wsQuality );


    QHBoxLayout* lay = new QHBoxLayout( this );
    lay->setContentsMargins( 0, 0, 0, 0 );

    lay->addWidget( w );

    connect( w->m_slideQualityLevel, SIGNAL(valueChanged(int)),
             this, SLOT(slotQualityLevelChanged(int)) );

    slotQualityLevelChanged( 4 );

    connect( w->m_radioQualityLevel, SIGNAL(toggled(bool)), this, SLOT(changed()) );
    connect( w->m_slideQualityLevel, SIGNAL(valueChanged(int)), this, SLOT(changed()) );
    connect( w->m_radioManual, SIGNAL(toggled(bool)), this, SLOT(changed()) );
    connect( w->m_checkBitrateUpper, SIGNAL(toggled(bool)), this, SLOT(changed()) );
    connect( w->m_checkBitrateNominal, SIGNAL(toggled(bool)), this, SLOT(changed()) );
    connect( w->m_checkBitrateLower, SIGNAL(toggled(bool)), this, SLOT(changed()) );
    connect( w->m_inputBitrateUpper, SIGNAL(valueChanged(int)), this, SLOT(changed()) );
    connect( w->m_inputBitrateNominal, SIGNAL(valueChanged(int)), this, SLOT(changed()) );
    connect( w->m_inputBitrateLower, SIGNAL(valueChanged(int)), this, SLOT(changed()) );
}


K3bOggVorbisEncoderSettingsWidget::~K3bOggVorbisEncoderSettingsWidget()
{
}


void K3bOggVorbisEncoderSettingsWidget::slotQualityLevelChanged( int val )
{
    w->m_labelQualityLevel->setText( QString::number(val) + ' '
                                     + i18n("(targeted VBR of %1)",s_rough_average_quality_level_bitrates[val+1]) );
}


void K3bOggVorbisEncoderSettingsWidget::load()
{
    KSharedConfig::Ptr c = KSharedConfig::openConfig();

    KConfigGroup grp(c, "K3bOggVorbisEncoderPlugin" );

    if( grp.readEntry( "manual bitrate", DEFAULT_MANUAL_BITRATE ) )
        w->m_radioManual->setChecked(true);
    else
        w->m_radioQualityLevel->setChecked(true);
    w->m_slideQualityLevel->setValue( grp.readEntry( "quality level", DEFAULT_QUALITY_LEVEL ) );
    w->m_inputBitrateUpper->setValue( grp.readEntry( "bitrate upper", DEFAULT_BITRATE_UPPER ) );
    w->m_checkBitrateUpper->setChecked( grp.readEntry( "bitrate upper", DEFAULT_BITRATE_UPPER ) != -1 );
    w->m_inputBitrateNominal->setValue( grp.readEntry( "bitrate nominal", DEFAULT_BITRATE_NOMINAL ) );
    w->m_checkBitrateNominal->setChecked( grp.readEntry( "bitrate nominal", DEFAULT_BITRATE_NOMINAL ) != -1 );
    w->m_inputBitrateLower->setValue( grp.readEntry( "bitrate lower", DEFAULT_BITRATE_LOWER ) );
    w->m_checkBitrateLower->setChecked( grp.readEntry( "bitrate lower", DEFAULT_BITRATE_LOWER ) != -1 );
    //  w->m_inputSamplerate->setValue( grp.readEntry( "samplerate", DEFAULT_SAMPLERATE ) );
}


void K3bOggVorbisEncoderSettingsWidget::save()
{
    KSharedConfig::Ptr c = KSharedConfig::openConfig();

    KConfigGroup grp(c,"K3bOggVorbisEncoderPlugin" );

    grp.writeEntry( "manual bitrate", w->m_radioManual->isChecked() );
    grp.writeEntry( "quality level", w->m_slideQualityLevel->value() );
    grp.writeEntry( "bitrate upper", w->m_checkBitrateUpper->isChecked() ? w->m_inputBitrateUpper->value() : -1 );
    grp.writeEntry( "bitrate nominal", w->m_checkBitrateNominal->isChecked() ? w->m_inputBitrateNominal->value() : -1 );
    grp.writeEntry( "bitrate lower", w->m_checkBitrateLower->isChecked() ? w->m_inputBitrateLower->value() : -1 );
    //  c->writeEntry( "samplerate", w->m_inputSamplerate->value() );
}


void K3bOggVorbisEncoderSettingsWidget::defaults()
{
    if( DEFAULT_MANUAL_BITRATE )
        w->m_radioManual->setChecked(true);
    else
        w->m_radioQualityLevel->setChecked(true);
    w->m_slideQualityLevel->setValue( DEFAULT_QUALITY_LEVEL );
    w->m_inputBitrateUpper->setValue( DEFAULT_BITRATE_UPPER );
    w->m_checkBitrateUpper->setChecked( DEFAULT_BITRATE_UPPER != -1 );
    w->m_inputBitrateNominal->setValue( DEFAULT_BITRATE_NOMINAL );
    w->m_checkBitrateNominal->setChecked( DEFAULT_BITRATE_NOMINAL != -1 );
    w->m_inputBitrateLower->setValue( DEFAULT_BITRATE_LOWER );
    w->m_checkBitrateLower->setChecked( DEFAULT_BITRATE_LOWER != -1 );
    //  w->m_inputSamplerate->setValue( DEFAULT_SAMPLERATE );
    emit changed( true );
}

#include "k3boggvorbisencoderconfigwidget.moc"
