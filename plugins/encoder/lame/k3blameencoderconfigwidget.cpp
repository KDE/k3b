/*

    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3blameencoderconfigwidget.h"
#include "k3blameencoderdefaults.h"
#include "k3blamemanualsettingsdialog.h"
#include "k3blametyes.h"

#include <KAboutData>
#include <KConfig>
#include <KConfigGroup>
#include <KSharedConfig>

#include <QDebug>
#include <QFile>
#include <QCheckBox>
#include <QLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>

namespace {
    
    int mode2Index( const QString& mode )
    {
        if( mode == "stereo" )
            return 0;
        else if( mode == "joint" )
            return 1;
        else // mono
            return 2;
    }
    
} // namespace


K3B_EXPORT_PLUGIN_CONFIG_WIDGET( kcm_k3blameencoder, K3bLameEncoderSettingsWidget )


K3bLameEncoderSettingsWidget::K3bLameEncoderSettingsWidget( QObject* parent, const KPluginMetaData& metaData, const QVariantList& args )
    : K3b::PluginConfigWidget( parent, metaData, args )
{
    setupUi( widget() );

    m_manualSettingsDialog = new K3bLameManualSettingsDialog( widget() );
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
    slotQualityLevelChanged( m_sliderQuality->value() );

    connect( m_radioQualityLevel, SIGNAL(toggled(bool)), this, SLOT(changed()) );
    connect( m_sliderQuality, SIGNAL(valueChanged(int)), this, SLOT(changed()) );
    connect( m_radioManual, SIGNAL(toggled(bool)), this, SLOT(changed()) );
    connect( m_spinEncoderQuality, SIGNAL(valueChanged(int)), this, SLOT(changed()) );
    connect( m_checkCopyright, SIGNAL(toggled(bool)), this, SLOT(changed()) );
    connect( m_checkOriginal, SIGNAL(toggled(bool)), this, SLOT(changed()) );
    connect( m_checkISO, SIGNAL(toggled(bool)), this, SLOT(changed()) );
    connect( m_checkError, SIGNAL(toggled(bool)), this, SLOT(changed()) );
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

    markAsChanged();
}


void K3bLameEncoderSettingsWidget::updateManualSettingsLabel()
{
    if( m_manualSettingsDialog->m_radioConstantBitrate->isChecked() )
        m_labelManualSettings->setText( i18n("Constant Bitrate: %1 kbps (%2)",
                                             s_lame_bitrates[m_manualSettingsDialog->m_comboConstantBitrate->currentIndex()],
                                             s_lame_mode_strings[m_manualSettingsDialog->m_comboMode->currentIndex()].toString()) );
    else
        m_labelManualSettings->setText( i18n("Variable Bitrate (%1)",
                                        s_lame_mode_strings[m_manualSettingsDialog->m_comboMode->currentIndex()].toString()) );
}


void K3bLameEncoderSettingsWidget::slotQualityLevelChanged( int val )
{
    m_labelQualityLevel->setText( s_lame_preset_strings[val].toString() );
}


void K3bLameEncoderSettingsWidget::load()
{
    qDebug();

    KSharedConfig::Ptr c = KSharedConfig::openConfig();
    KConfigGroup grp(c, QStringLiteral("K3bLameEncoderPlugin") );

    QString mode = grp.readEntry( "Mode", DEFAULT_MODE );
    m_manualSettingsDialog->m_comboMode->setCurrentIndex( mode2Index( mode ) );

    bool manual = grp.readEntry( "Manual Bitrate Settings", DEFAULT_MANUAL_BITRATE );
    if( manual )
        m_radioManual->setChecked(true);
    else
        m_radioQualityLevel->setChecked(true);

    if(grp.readEntry( "VBR", DEFAULT_VBR ) )
        m_manualSettingsDialog->m_radioVariableBitrate->setChecked( true );
    else
        m_manualSettingsDialog->m_radioConstantBitrate->setChecked( true );

    m_manualSettingsDialog->m_comboConstantBitrate->setCurrentText( i18n("%1 kbps",grp.readEntry( "Constant Bitrate", DEFAULT_CONSTANT_BITRATE )) );
    m_manualSettingsDialog->m_comboMaximumBitrate->setCurrentText( i18n("%1 kbps",grp.readEntry( "Maximum Bitrate", DEFAULT_MAXIMUM_BITRATE )) );
    m_manualSettingsDialog->m_comboMinimumBitrate->setCurrentText( i18n("%1 kbps",grp.readEntry( "Minimum Bitrate", DEFAULT_MINIMUM_BITRATE )) );
    m_manualSettingsDialog->m_spinAverageBitrate->setValue( grp.readEntry( "Average Bitrate", DEFAULT_AVERAGE_BITRATE) );

    m_manualSettingsDialog->m_checkBitrateMaximum->setChecked( grp.readEntry( "Use Maximum Bitrate", DEFAULT_USE_MAXIMUM_BITRATE ) );
    m_manualSettingsDialog->m_checkBitrateMinimum->setChecked( grp.readEntry( "Use Minimum Bitrate", DEFAULT_USE_MINIMUM_BITRATE ) );
    m_manualSettingsDialog->m_checkBitrateAverage->setChecked( grp.readEntry( "Use Average Bitrate", DEFAULT_USE_AVERAGE_BITRATE ) );

    m_sliderQuality->setValue( grp.readEntry( "Quality Level", DEFAULT_QUALITY_LEVEL ) );

    m_checkCopyright->setChecked( grp.readEntry( "Copyright", DEFAULT_COPYRIGHT ) );
    m_checkOriginal->setChecked( grp.readEntry( "Original", DEFAULT_ORIGINAL ) );
    m_checkISO->setChecked( grp.readEntry( "ISO compliance", DEFAULT_ISO_COMPLIANCE ) );
    m_checkError->setChecked( grp.readEntry( "Error Protection", DEFAULT_ERROR_PROTECTION ) );

    // default to 2 which is the same as the -h lame option
    m_spinEncoderQuality->setValue( grp.readEntry( "Encoder Quality", DEFAULT_ENCODER_QUALITY ) );

    updateManualSettingsLabel();
}


void K3bLameEncoderSettingsWidget::save()
{
    qDebug();

    KSharedConfig::Ptr c = KSharedConfig::openConfig();
    KConfigGroup grp(c, QStringLiteral("K3bLameEncoderPlugin") );

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
}


void K3bLameEncoderSettingsWidget::defaults()
{
    qDebug();

    m_manualSettingsDialog->m_comboMode->setCurrentIndex( mode2Index( DEFAULT_MODE ) );

    if( DEFAULT_MANUAL_BITRATE )
        m_radioManual->setChecked(true);
    else
        m_radioQualityLevel->setChecked(true);

    if( DEFAULT_VBR )
        m_manualSettingsDialog->m_radioVariableBitrate->setChecked( true );
    else
        m_manualSettingsDialog->m_radioConstantBitrate->setChecked( true );

    m_manualSettingsDialog->m_comboConstantBitrate->setCurrentText( i18n("%1 kbps", DEFAULT_CONSTANT_BITRATE ) );
    m_manualSettingsDialog->m_comboMaximumBitrate->setCurrentText( i18n("%1 kbps", DEFAULT_MAXIMUM_BITRATE ) );
    m_manualSettingsDialog->m_comboMinimumBitrate->setCurrentText( i18n("%1 kbps", DEFAULT_MINIMUM_BITRATE ) );
    m_manualSettingsDialog->m_spinAverageBitrate->setValue( DEFAULT_AVERAGE_BITRATE );

    m_manualSettingsDialog->m_checkBitrateMaximum->setChecked(  DEFAULT_USE_MAXIMUM_BITRATE );
    m_manualSettingsDialog->m_checkBitrateMinimum->setChecked( DEFAULT_USE_MINIMUM_BITRATE );
    m_manualSettingsDialog->m_checkBitrateAverage->setChecked( DEFAULT_USE_AVERAGE_BITRATE );

    m_sliderQuality->setValue( DEFAULT_QUALITY_LEVEL );

    m_checkCopyright->setChecked( DEFAULT_COPYRIGHT );
    m_checkOriginal->setChecked( DEFAULT_ORIGINAL );
    m_checkISO->setChecked( DEFAULT_ISO_COMPLIANCE );
    m_checkError->setChecked( DEFAULT_ERROR_PROTECTION );

    // default to 2 which is the same as the -h lame option
    m_spinEncoderQuality->setValue( DEFAULT_ENCODER_QUALITY );

    updateManualSettingsLabel();
    setNeedsSave( true );
}

#include "k3blameencoderconfigwidget.moc"

#include "moc_k3blameencoderconfigwidget.cpp"
