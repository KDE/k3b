/*
 *
 * Copyright (C) 2006-2008 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bvideodvdrippingwidget.h"

#include "k3bvideodvdtitletranscodingjob.h"
#include "k3bglobals.h"
#include "k3bintmapcombobox.h"

#include <KLineEdit>
#include <KColorScheme>
#include <KLocalizedString>
#include <KDiskFreeSpaceInfo>
#include <KIO/Global>
#include <KUrlRequester>
#include <KUrlLabel>

#include <QTimer>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QWhatsThis>


static const int s_mp3Bitrates[] = {
    32,
    40,
    48,
    56,
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
    0 // just used for the loops below
};


static const int PICTURE_SIZE_ORIGINAL = 0;
static const int PICTURE_SIZE_640 = 1;
static const int PICTURE_SIZE_320 = 2;
static const int PICTURE_SIZE_CUSTOM = 3;
static const int PICTURE_SIZE_MAX = 4;

static const char* s_pictureSizeNames[] = {
    I18N_NOOP("Keep original dimensions"),
    I18N_NOOP("640x? (automatic height)"),
    I18N_NOOP("320x? (automatic height)"),
    I18N_NOOP("Custom")
};


K3b::VideoDVDRippingWidget::VideoDVDRippingWidget( QWidget* parent )
    : QWidget( parent ),
      m_neededSize( 0 )
{
    setupUi( this );

    m_editBaseDir->setMode( KFile::Directory | KFile::ExistingOnly | KFile::LocalOnly );

    //
    // Example filename pattern
    //
    m_comboFilenamePattern->addItem( QString( "%b - %1 %t (%n %a %c)").arg(i18n("Title") ) );
    m_comboFilenamePattern->addItem( QString( "%{volumeid} (%{title})" ) );


    //
    // Add the Audio bitrates
    //
    for( int i = 0; s_mp3Bitrates[i]; ++i )
        m_comboAudioBitrate->addItem( i18n("%1 kbps" ,s_mp3Bitrates[i]) );


    for( int i = 0; i < K3b::VideoDVDTitleTranscodingJob::VIDEO_CODEC_NUM_ENTRIES; ++i ) {
        K3b::VideoDVDTitleTranscodingJob::VideoCodec codec( (K3b::VideoDVDTitleTranscodingJob::VideoCodec)i );
        if( K3b::VideoDVDTitleTranscodingJob::transcodeBinaryHasSupportFor( codec ) )
            m_comboVideoCodec->insertItem( i,
                                           K3b::VideoDVDTitleTranscodingJob::videoCodecString( codec ),
                                           K3b::VideoDVDTitleTranscodingJob::videoCodecDescription( codec ) );
    }
    for( int i = 0; i < K3b::VideoDVDTitleTranscodingJob::AUDIO_CODEC_NUM_ENTRIES; ++i ) {
        K3b::VideoDVDTitleTranscodingJob::AudioCodec codec( (K3b::VideoDVDTitleTranscodingJob::AudioCodec)i );
        if( K3b::VideoDVDTitleTranscodingJob::transcodeBinaryHasSupportFor( codec ) )
            m_comboAudioCodec->insertItem( i,
                                           K3b::VideoDVDTitleTranscodingJob::audioCodecString( codec ),
                                           K3b::VideoDVDTitleTranscodingJob::audioCodecDescription( codec ) );
    }

    for( int i = 0; i < PICTURE_SIZE_MAX; ++i ) {
        m_comboVideoSize->addItem( i18n( s_pictureSizeNames[i] ) );
    }

    slotAudioCodecChanged( m_comboAudioCodec->selectedValue() );

    connect( m_comboAudioBitrate, SIGNAL(textChanged(QString)),
             this, SIGNAL(changed()) );
    connect( m_spinVideoBitrate, SIGNAL(valueChanged(int)),
             this, SIGNAL(changed()) );
    connect( m_checkBlankReplace, SIGNAL(toggled(bool)),
             this, SIGNAL(changed()) );
    connect( m_editBlankReplace, SIGNAL(textChanged(QString)),
             this, SIGNAL(changed()) );
    connect( m_comboFilenamePattern, SIGNAL(textChanged(QString)),
             this, SIGNAL(changed()) );
    connect( m_editBaseDir, SIGNAL(textChanged(QString)),
             this, SIGNAL(changed()) );

    connect( m_comboAudioCodec, SIGNAL(valueChanged(int)),
             this, SLOT(slotAudioCodecChanged(int)) );
    connect( m_specialStringsLabel, SIGNAL(leftClickedUrl()),
             this, SLOT(slotSeeSpecialStrings()) );
    connect( m_buttonCustomPictureSize, SIGNAL(clicked()),
             this, SLOT(slotCustomPictureSize()) );
    connect( m_comboVideoSize, SIGNAL(activated(int)),
             this, SLOT(slotVideoSizeChanged(int)) );

    // refresh every 2 seconds
    m_freeSpaceUpdateTimer = new QTimer( this );
    connect( m_freeSpaceUpdateTimer, SIGNAL(timeout()),
             this, SLOT(slotUpdateFreeTempSpace()) );
    m_freeSpaceUpdateTimer->start(2000);
    slotUpdateFreeTempSpace();
}


K3b::VideoDVDRippingWidget::~VideoDVDRippingWidget()
{
}


K3b::VideoDVDTitleTranscodingJob::VideoCodec K3b::VideoDVDRippingWidget::selectedVideoCodec() const
{
    return (K3b::VideoDVDTitleTranscodingJob::VideoCodec)m_comboVideoCodec->selectedValue();
}


QSize K3b::VideoDVDRippingWidget::selectedPictureSize() const
{
    switch( m_comboVideoSize->currentIndex() ) {
    case PICTURE_SIZE_ORIGINAL:
        return QSize(0,0);
    case PICTURE_SIZE_640:
        return QSize(640,0);
    case PICTURE_SIZE_320:
        return QSize(320,0);
    default:
        return m_customVideoSize;
    }
}


void K3b::VideoDVDRippingWidget::setSelectedPictureSize( const QSize& size )
{
    m_customVideoSize = size;
    if( size == QSize(0,0) )
        m_comboVideoSize->setCurrentIndex( PICTURE_SIZE_ORIGINAL );
    else if( size == QSize(640,0) )
        m_comboVideoSize->setCurrentIndex( PICTURE_SIZE_640 );
    else if( size == QSize(320,0) )
        m_comboVideoSize->setCurrentIndex( PICTURE_SIZE_320 );
    else {
        m_comboVideoSize->setItemText( PICTURE_SIZE_CUSTOM,
                                       i18n(s_pictureSizeNames[PICTURE_SIZE_CUSTOM])
                                       + QString(" (%1x%2)")
                                       .arg(size.width() == 0 ? i18n("auto") : QString::number(size.width()))
                                       .arg(size.height() == 0 ? i18n("auto") : QString::number(size.height())));
        m_comboVideoSize->setCurrentIndex( PICTURE_SIZE_CUSTOM );
    }
}


void K3b::VideoDVDRippingWidget::setSelectedVideoCodec( K3b::VideoDVDTitleTranscodingJob::VideoCodec codec )
{
    m_comboVideoCodec->setSelectedValue( (int)codec );
}


K3b::VideoDVDTitleTranscodingJob::AudioCodec K3b::VideoDVDRippingWidget::selectedAudioCodec() const
{
    return (K3b::VideoDVDTitleTranscodingJob::AudioCodec)m_comboAudioCodec->selectedValue();
}


void K3b::VideoDVDRippingWidget::setSelectedAudioCodec( K3b::VideoDVDTitleTranscodingJob::AudioCodec codec )
{
    m_comboAudioCodec->setSelectedValue( (int)codec );
    slotAudioCodecChanged( (int)codec );
}


int K3b::VideoDVDRippingWidget::selectedAudioBitrate() const
{
    if( selectedAudioCodec() == K3b::VideoDVDTitleTranscodingJob::AUDIO_CODEC_MP3 )
        return s_mp3Bitrates[m_comboAudioBitrate->currentIndex()];
    else
        return m_spinAudioBitrate->value();
}


void K3b::VideoDVDRippingWidget::setSelectedAudioBitrate( int bitrate )
{
    m_spinAudioBitrate->setValue( bitrate );

    // select the bitrate closest to "bitrate"
    int bi = 0;
    int diff = 1000;
    for( int i = 0; s_mp3Bitrates[i]; ++i ) {
        int newDiff = s_mp3Bitrates[i] - bitrate;
        if( newDiff < 0 )
            newDiff = -1 * newDiff;
        if( newDiff < diff ) {
            diff = newDiff;
            bi = i;
        }
    }

    m_comboAudioBitrate->setCurrentIndex( bi );
}


void K3b::VideoDVDRippingWidget::slotUpdateFreeTempSpace()
{
    QString path = m_editBaseDir->url().toLocalFile();

    if( !QFile::exists( path ) )
        path.truncate( path.lastIndexOf('/') );

    const KColorScheme colorScheme( isEnabled() ? QPalette::Normal : QPalette::Disabled, KColorScheme::Window );
    QColor textColor;

    KDiskFreeSpaceInfo free = KDiskFreeSpaceInfo::freeSpaceInfo( path );
    if( free.isValid() ) {
        m_labelFreeSpace->setText( KIO::convertSizeFromKiB(free.available()/1024) );
        if( free.available() < m_neededSize )
            textColor = colorScheme.foreground( KColorScheme::NegativeText ).color();
        else
            textColor = colorScheme.foreground( KColorScheme::NormalText ).color();
    }
    else {
        textColor = colorScheme.foreground( KColorScheme::NormalText ).color();
        m_labelFreeSpace->setText("-");
    }

    QPalette pal( m_labelFreeSpace->palette() );
    pal.setColor( QPalette::Text, textColor );
    m_labelFreeSpace->setPalette( pal );
}


void K3b::VideoDVDRippingWidget::setNeededSize( KIO::filesize_t size )
{
    m_neededSize = size;
    if( size > 0 )
        m_labelNeededSpace->setText( KIO::convertSize( size ) );
    else
        m_labelNeededSpace->setText( i18n("unknown") );

    slotUpdateFreeTempSpace();
}


void K3b::VideoDVDRippingWidget::slotSeeSpecialStrings()
{
    QWhatsThis::showText( QCursor::pos(),
                          i18n( "<p><b>Pattern special strings:</b>"
                                "<p>The following strings will be replaced with their respective meaning in every "
                                "track name.<br>"
                                "<p><table border=\"0\">"
                                "<tr><td></td><td><em>Meaning</em></td><td><em>Alternatives</em></td></tr>"
                                "<tr><td>%t</td><td>title number</td><td>%{t} or %{title_number}</td></tr>"
                                "<tr><td>%i</td><td>volume id (mostly the name of the Video DVD)</td><td>%{i} or %{volume_id}</td></tr>"
                                "<tr><td>%b</td><td>beautified volume id</td><td>%{b} or %{beautified_volume_id}</td></tr>"
                                "<tr><td>%l</td><td>two chars language code</td><td>%{l} or %{lang_code}</td></tr>"
                                "<tr><td>%n</td><td>language name</td><td>%{n} or %{lang_name}</td></tr>"
                                "<tr><td>%a</td><td>audio format (on the Video DVD)</td><td>%{a} or %{audio_format}</td></tr>"
                                "<tr><td>%c</td><td>number of audio channels (on the Video DVD)</td><td>%{c} or %{channels}</td></tr>"
                                "<tr><td>%v</td><td>size of the original video</td><td>%{v} or %{orig_video_size}</td></tr>"
                                "<tr><td>%s</td><td>size of the resulting video (<em>Caution: auto-clipping values are not taken into account.</em>)</td><td>%{s} or %{video_size}</td></tr>"
                                "<tr><td>%r</td><td>aspect ratio of the original video</td><td>%{r} or %{aspect_ratio}</td></tr>"
                                "<tr><td>%d</td><td>current date</td><td>%{d} or %{date}</td></tr>"
                                "</table>"
                                "<p><em>Hint: K3b also accepts slight variations of the long special strings. "
                                "One can, for example, leave out the underscores.</em>"),
                          this );
}


void K3b::VideoDVDRippingWidget::slotAudioCodecChanged( int codec )
{
    switch( codec ) {
    case K3b::VideoDVDTitleTranscodingJob::AUDIO_CODEC_MP3:
        m_stackAudioQuality->setCurrentWidget( m_stackPageAudioQualityMp3 );
        break;
    case K3b::VideoDVDTitleTranscodingJob::AUDIO_CODEC_AC3_STEREO:
        m_stackAudioQuality->setCurrentWidget( m_stackPageAudioQualityAC3 );
        break;
    case K3b::VideoDVDTitleTranscodingJob::AUDIO_CODEC_AC3_PASSTHROUGH:
        m_stackAudioQuality->setCurrentWidget( m_stackPageAudioQualityAC3Pt );
        break;
    }

    emit changed();
}


void K3b::VideoDVDRippingWidget::slotVideoSizeChanged( int sizeIndex )
{
    if( sizeIndex == PICTURE_SIZE_CUSTOM )
        slotCustomPictureSize();
    else
        emit changed();
}


void K3b::VideoDVDRippingWidget::slotCustomPictureSize()
{
    QDialog dlg( this );
    dlg.setWindowTitle( i18n("Video Picture Size") );

    QLabel* label = new QLabel( i18n("<p>Please choose the width and height of the resulting video. "
                                     "If one value is set to <em>Auto</em> K3b will choose this value "
                                     "depending on the aspect ratio of the video picture.<br>"
                                     "Be aware that setting both the width and the height to fixed values "
                                     "will result in no aspect ratio correction being performed."),
                                &dlg );
    label->setWordWrap( true );
    QSpinBox* spinWidth = new QSpinBox( &dlg );
    spinWidth->setRange( 0, 20000 );
    spinWidth->setSingleStep( 16 );
    QSpinBox* spinHeight = new QSpinBox( &dlg );
    spinHeight->setRange( 0, 20000 );
    spinHeight->setSingleStep( 16 );
    spinWidth->setSpecialValueText( i18n("Auto") );
    spinHeight->setSpecialValueText( i18n("Auto") );
    QLabel* labelW = new QLabel( i18n("Width:"), &dlg );
    labelW->setBuddy( spinWidth );
    QLabel* labelH = new QLabel( i18n("Height:"), &dlg );
    labelH->setBuddy( spinHeight );
    labelW->setAlignment( Qt::AlignRight|Qt::AlignVCenter );
    labelH->setAlignment( Qt::AlignRight|Qt::AlignVCenter );

    QDialogButtonBox* buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg );
    connect( buttonBox, SIGNAL(accepted()), &dlg, SLOT(accept()) );
    connect( buttonBox, SIGNAL(rejected()), &dlg, SLOT(reject()) );

    QGridLayout* grid = new QGridLayout( &dlg );
    grid->setContentsMargins( 0, 0, 0, 0 );
    grid->addWidget( label, 0, 0, 1, 4 );
    grid->addWidget( labelW, 1, 0 );
    grid->addWidget( spinWidth, 1, 1 );
    grid->addWidget( labelH, 1, 2 );
    grid->addWidget( spinHeight, 1, 3 );
    grid->addWidget( buttonBox, 2, 0, 1, 4 );

    spinWidth->setValue( m_customVideoSize.width() );
    spinHeight->setValue( m_customVideoSize.height() );

    if( dlg.exec() ) {
        setSelectedPictureSize( QSize( spinWidth->value(), spinHeight->value() ) );
        emit changed();
    }
}


