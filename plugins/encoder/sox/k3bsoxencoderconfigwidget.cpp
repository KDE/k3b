/*
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bsoxencoderconfigwidget.h"
#include "k3bsoxencoderdefaults.h"

#include <KConfigGroup>
#include <KSharedConfig>

K3B_EXPORT_PLUGIN_CONFIG_WIDGET( kcm_k3bsoxencoder, K3bSoxEncoderConfigWidget )

K3bSoxEncoderConfigWidget::K3bSoxEncoderConfigWidget(QObject *parent, const KPluginMetaData& metaData, const QVariantList& args )
    : K3b::PluginConfigWidget( parent, metaData, args )
{
    setupUi( widget() );
    m_editSamplerate->setValidator( new QIntValidator( m_editSamplerate ) );
    
    connect( m_checkManual, SIGNAL(toggled(bool)), this, SLOT(changed()) );
    connect( m_comboChannels, SIGNAL(currentIndexChanged(int)), this, SLOT(changed()) );
    connect( m_editSamplerate, SIGNAL(textChanged(QString)), this, SLOT(changed()) );
    connect( m_comboSize, SIGNAL(currentIndexChanged(int)), this, SLOT(changed()) );
    connect( m_comboEncoding, SIGNAL(currentIndexChanged(int)), this, SLOT(changed()) );
}


K3bSoxEncoderConfigWidget::~K3bSoxEncoderConfigWidget()
{
}


void K3bSoxEncoderConfigWidget::load()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup grp( config, "K3bSoxEncoderPlugin" );

    m_checkManual->setChecked( grp.readEntry( "manual settings", DEFAULT_MANUAL_SETTINGS ) );
    setChannels( grp.readEntry( "channels", DEFAULT_CHANNELS ) );
    m_editSamplerate->setText( QString::number( grp.readEntry( "samplerate", DEFAULT_SAMPLE_RATE ) ) );
    setDataEncoding( grp.readEntry( "data encoding", DEFAULT_DATA_ENCODING ) );
    setDataSize( grp.readEntry( "data size", DEFAULT_DATA_SIZE ) );
}


void K3bSoxEncoderConfigWidget::save()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup grp( config, "K3bSoxEncoderPlugin" );

    grp.writeEntry( "manual settings", m_checkManual->isChecked() );
    grp.writeEntry( "channels", getChannels() );
    grp.writeEntry( "data size", getDataSize() );
    grp.writeEntry( "samplerate", m_editSamplerate->text().toInt() );
    grp.writeEntry( "data encoding", getDataEncoding() );
}


void K3bSoxEncoderConfigWidget::defaults()
{
    m_checkManual->setChecked( DEFAULT_MANUAL_SETTINGS );
    setChannels( DEFAULT_CHANNELS );
    m_editSamplerate->setText( QString::number( DEFAULT_SAMPLE_RATE ) );
    setDataEncoding( DEFAULT_DATA_ENCODING );
    setDataSize( DEFAULT_DATA_SIZE );
    setNeedsSave( true );
}


void K3bSoxEncoderConfigWidget::setChannels( int channels )
{
    m_comboChannels->setCurrentIndex( channels == 4 ? 2 : channels-1 );
}


int K3bSoxEncoderConfigWidget::getChannels() const
{
    if( m_comboChannels->currentIndex() == 0 )
        return 1;
    else if( m_comboChannels->currentIndex() == 2 )
        return 4;
    else
        return 2;
}


void K3bSoxEncoderConfigWidget::setDataSize( int size )
{
    m_comboSize->setCurrentIndex( size == 8 ? 0 : ( size == 32 ? 2 : 1 ) );
}


int K3bSoxEncoderConfigWidget::getDataSize() const
{
    if( m_comboSize->currentIndex() == 0 )
        return 8;
    else if( m_comboSize->currentIndex() == 2 )
        return 32;
    else
        return 16;
}


void K3bSoxEncoderConfigWidget::setDataEncoding( const QString& encoding )
{
    if( encoding == "unsigned" )
        m_comboEncoding->setCurrentIndex(1);
    else if( encoding == "u-law" )
        m_comboEncoding->setCurrentIndex(2);
    else if( encoding == "A-law" )
        m_comboEncoding->setCurrentIndex(3);
    else if( encoding == "ADPCM" )
        m_comboEncoding->setCurrentIndex(4);
    else if( encoding == "IMA_ADPCM" )
        m_comboEncoding->setCurrentIndex(5);
    else if( encoding == "GSM" )
        m_comboEncoding->setCurrentIndex(6);
    else if( encoding == "Floating-point" )
        m_comboEncoding->setCurrentIndex(7);
    else
        m_comboEncoding->setCurrentIndex(0);
}


QString K3bSoxEncoderConfigWidget::getDataEncoding() const
{
    switch( m_comboEncoding->currentIndex() ) {
        case 1: return "unsigned";
        case 2: return "u-law";
        case 3: return "A-law";
        case 4: return "ADPCM";
        case 5: return "IMA_ADPCM";
        case 6: return "GSM";
        case 7: return "Floating-point";
        default: return "signed";
    }
}

#include "k3bsoxencoderconfigwidget.moc"
