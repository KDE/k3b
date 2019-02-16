/*
 *
 * Copyright (C) 2006-2009 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bvideodvdrippingdialog.h"
#include "k3bapplication.h"
#include "k3bfilesysteminfo.h"
#include "k3bglobals.h"
#include "k3bjobprogressdialog.h"
#include "k3bmedium.h"
#include "k3bmediacache.h"
#include "k3bvideodvd.h"
#include "k3bvideodvdaudiomodel.h"
#include "k3bvideodvdrippingjob.h"
#include "k3bvideodvdrippingwidget.h"
#include "k3bvideodvdtitletranscodingjob.h"

#include <KComboBox>
#include <KLineEdit>
#include <KConfig>
#include <KLocalizedString>
#include <KUrlRequester>
#include <KIO/Global>
#include <KMessageBox>

#include <QList>
#include <QLocale>
#include <QMap>
#include <QVector>
#include <QFontMetrics>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLayout>
#include <QSpinBox>
#include <QStyle>


namespace {

QString videoCodecId( K3b::VideoDVDTitleTranscodingJob::VideoCodec codec )
{
    switch( codec ) {
    case K3b::VideoDVDTitleTranscodingJob::VIDEO_CODEC_FFMPEG_MPEG4:
        return "ffmpeg_mpeg4";
    case K3b::VideoDVDTitleTranscodingJob::VIDEO_CODEC_XVID:
        return "xvid";
    default:
        return "none";
    }
}


QString audioCodecId( K3b::VideoDVDTitleTranscodingJob::AudioCodec codec )
{
    switch( codec ) {
    case K3b::VideoDVDTitleTranscodingJob::AUDIO_CODEC_MP3:
        return "mp3";
    case K3b::VideoDVDTitleTranscodingJob::AUDIO_CODEC_AC3_STEREO:
        return "ac3_stereo";
    case K3b::VideoDVDTitleTranscodingJob::AUDIO_CODEC_AC3_PASSTHROUGH:
        return "ac3_passthrough";
    default:
        return "none";
    }
}


K3b::VideoDVDTitleTranscodingJob::VideoCodec videoCodecFromId( const QString& codec )
{
    if( codec == "xvid" )
        return K3b::VideoDVDTitleTranscodingJob::VIDEO_CODEC_XVID;
    else //  if( codec == "ffmpeg_mpeg4" )
        return K3b::VideoDVDTitleTranscodingJob::VIDEO_CODEC_FFMPEG_MPEG4;
}


K3b::VideoDVDTitleTranscodingJob::AudioCodec audioCodecFromId( const QString& codec )
{
    if( codec == "ac3_stereo" )
        return K3b::VideoDVDTitleTranscodingJob::AUDIO_CODEC_AC3_STEREO;
    else if( codec == "ac3_passthrough" )
        return K3b::VideoDVDTitleTranscodingJob::AUDIO_CODEC_AC3_PASSTHROUGH;
    else // if( codec == "mp3" )
        return K3b::VideoDVDTitleTranscodingJob::AUDIO_CODEC_MP3;
}


// resize according to aspect ratio
QSize resizeTitle( const K3b::VideoDVD::VideoStream& title, const QSize& size )
{
    int w = size.width();
    int h = size.height();
    int rw = title.realPictureWidth();
    int rh = title.realPictureHeight();

    if( w == 0 && h == 0 ) {
        w = rw;
        h = rh;
    }
    else if( w == 0 ) {
        w = h * rw / rh;
    }
    else if( h == 0 ) {
        h = w * rh / rw;
    }

    return QSize(w,h);
}

typedef QMap< const K3b::VideoDVD::Title*, K3b::VideoDVDRippingJob::TitleRipInfo > TitleRipInfos;

} // namespace


class K3b::VideoDVDRippingDialog::Private
{
public:
    Private( const K3b::VideoDVD::VideoDVD& d )
        : dvd( d ), w( 0 ), audioModel( 0 ) {}

    VideoDVD::VideoDVD dvd;
    VideoDVDRippingWidget* w;
    VideoDVDAudioModel* audioModel;
    TitleRipInfos titleRipInfos;
    K3b::FileSystemInfo fsInfo;

    QString createFilename( const VideoDVDRippingJob::TitleRipInfo& info, const QString& pattern ) const;
};


QString K3b::VideoDVDRippingDialog::Private::createFilename( const K3b::VideoDVDRippingJob::TitleRipInfo& info, const QString& pattern ) const
{
    QString f;

    const K3b::VideoDVD::Title& title = dvd[info.title-1];

    for( int i = 0; i < pattern.length(); ++i ) {
        //
        // every pattern starts with a % sign
        //
        if( pattern[i] == '%' && i+1 < pattern.length() ) {
            ++i; // skip the %
            QChar c = pattern[i];

            //
            // first check if we have a long keyword instead of a one-char
            //
            if( pattern[i] == '{' ) {
                int j = pattern.indexOf( '}', i );
                if( j < 0 ) // no closing bracket -> no valid pattern
                    c = '*';
                else {
                    QString keyword = pattern.mid( i+1, j-i-1 );
                    if( keyword == "titlenumber"  ||
                        keyword == "title_number" ||
                        keyword == "title" ) {
                        c = PATTERN_TITLE_NUMBER;
                    }
                    else if( keyword == "volumeid"  ||
                             keyword == "volume_id" ||
                             keyword == "volid"     ||
                             keyword == "vol_id" ) {
                        c = PATTERN_VOLUME_ID;
                    }
                    else if( keyword == "beautifiedvolumeid"   ||
                             keyword == "beautified_volumeid"  ||
                             keyword == "beautified_volume_id" ||
                             keyword == "beautifiedvolid"      ||
                             keyword == "beautified_volid"     ||
                             keyword == "beautified_vol_id"    ||
                             keyword == "nicevolid"            ||
                             keyword == "nice_volid"           ||
                             keyword == "nice_vol_id" ) {
                        c = PATTERN_BEAUTIFIED_VOLUME_ID;
                    }
                    else if( keyword == "languagecode"  ||
                             keyword == "language_code" ||
                             keyword == "langcode"      ||
                             keyword == "lang_code" ) {
                        c = PATTERN_LANGUAGE_CODE;

                    }
                    else if( keyword == "lang" ||
                             keyword == "language" ||
                             keyword == "langname" ||
                             keyword == "languagename" ||
                             keyword == "lang_name" ||
                             keyword == "language_name" ) {
                        c = PATTERN_LANGUAGE_NAME;
                    }
                    else if( keyword == "audioformat"  ||
                             keyword == "audio_format" ||
                             keyword == "audio" ) {
                        c = PATTERN_AUDIO_FORMAT;
                    }
                    else if( keyword == "channels" ||
                             keyword == "audiochannels" ||
                             keyword == "audio_channels" ||
                             keyword == "ch" ) {
                        c = PATTERN_AUDIO_CHANNELS;
                    }
                    else if( keyword == "videosize"  ||
                             keyword == "video_size" ||
                             keyword == "vsize" ) {
                        c = PATTERN_VIDEO_SIZE;
                    }
                    else if( keyword == "originalvideosize"  ||
                             keyword == "original_video_size" ||
                             keyword == "origvideosize"  ||
                             keyword == "orig_video_size" ||
                             keyword == "origvsize" ) {
                        c = PATTERN_ORIG_VIDEO_SIZE;
                    }
                    else if( keyword == "aspect_ratio" ||
                             keyword == "aspectratio" ||
                             keyword == "ratio" ) {
                        c = PATTERN_ASPECT_RATIO;
                    }
                    else if( keyword == "current_date" ||
                             keyword == "currentdate" ||
                             keyword == "date" ) {
                        c = PATTERN_CURRENT_DATE;
                    }
                    else {
                        // unusable pattern
                        c = '*';
                    }

                    //
                    // skip the keyword and the closing bracket
                    //
                    if( c != '*' ) {
                        i += keyword.length() + 1;
                    }
                }
            }

            switch( c.toLatin1() ) {
            case PATTERN_TITLE_NUMBER:
                f.append( QString::number(info.title).rightJustified( 2, '0' ) );
                break;
            case PATTERN_VOLUME_ID:
                f.append( dvd.volumeIdentifier() );
                break;
            case PATTERN_BEAUTIFIED_VOLUME_ID:
                f.append( k3bappcore->mediaCache()->medium( dvd.device() ).beautifiedVolumeId() );
                break;
            case PATTERN_LANGUAGE_CODE:
                if( title.numAudioStreams() > 0 )
                    f.append( title.audioStream( info.audioStream ).langCode() );
                break;
            case PATTERN_LANGUAGE_NAME:
                if( title.numAudioStreams() > 0 )
                    f.append( QLocale( title.audioStream( info.audioStream ).langCode() ).nativeLanguageName() );
                break;
            case PATTERN_AUDIO_FORMAT:
                // FIXME: what about MPEG audio streams?
                if( title.numAudioStreams() > 0 ) {
                    if( w->selectedAudioCodec() == K3b::VideoDVDTitleTranscodingJob::AUDIO_CODEC_MP3 )
                        f.append( K3b::VideoDVDTitleTranscodingJob::audioCodecString( w->selectedAudioCodec() ) );
                    else
                        f.append( K3b::VideoDVD::audioFormatString( title.audioStream( info.audioStream ).format() ) );
                }
                break;
            case PATTERN_AUDIO_CHANNELS:
                if( title.numAudioStreams() > 0 )
                    // xgettext: no-c-format
                    f.append( i18ncp("Ch is short for Channels", "%1Ch", "%1Ch",
                                     w->selectedAudioCodec() == K3b::VideoDVDTitleTranscodingJob::AUDIO_CODEC_AC3_PASSTHROUGH
                                     ? title.audioStream( info.audioStream ).channels()
                                     : 2 ) );
                break;
            case PATTERN_ORIG_VIDEO_SIZE:
                f.append( QString("%1x%2")
                          .arg(title.videoStream().pictureWidth())
                          .arg(title.videoStream().pictureHeight()) );
                break;
            case PATTERN_VIDEO_SIZE: {
                QSize s( resizeTitle( dvd[info.title-1].videoStream(), w->selectedPictureSize() ) );
                f.append( QString("%1x%2").arg(s.width()).arg(s.height()) );
                break;
            }
            case PATTERN_ASPECT_RATIO:
                if( title.videoStream().displayAspectRatio() == K3b::VideoDVD::VIDEO_ASPECT_RATIO_4_3 )
                    f.append( "4:3" );
                else
                    f.append( "16:9" );
                break;
            case PATTERN_CURRENT_DATE:
                f.append( QLocale().toString( QDate::currentDate() ) );
                break;
            default:
                f.append( pattern[i-1] );
                f.append( pattern[i] );
            }
        }

        //
        // normal character -> just append to filename
        //
        else {
            f.append( pattern[i] );
        }
    }

    //
    // and the extension (for now only avi)
    //
    f.append( ".avi" );

    return f;
}


K3b::VideoDVDRippingDialog::VideoDVDRippingDialog( const K3b::VideoDVD::VideoDVD& dvd,
                                                    const QList<int>& titles,
                                                    QWidget* parent )
    : K3b::InteractionDialog( parent,
                            i18n("Video DVD Ripping"),
                            QString(),
                            START_BUTTON|CANCEL_BUTTON,
                            START_BUTTON,
                            "VideoDVD Ripping" ), // config group
      d( new Private( dvd ) )
{
    d->audioModel = new VideoDVDAudioModel( dvd, titles, this );

    QWidget* frame = mainWidget();
    d->w = new K3b::VideoDVDRippingWidget( frame );
    d->w->m_titleView->setModel( d->audioModel );
    d->w->m_titleView->expandAll();
    d->w->m_titleView->header()->setSectionResizeMode( VideoDVDAudioModel::TitleColumn, QHeaderView::ResizeToContents );
    d->w->m_titleView->header()->setSectionResizeMode( VideoDVDAudioModel::VideoSizeColumn, QHeaderView::ResizeToContents );
    d->w->m_titleView->header()->setSectionResizeMode( VideoDVDAudioModel::FileSizeColumn, QHeaderView::ResizeToContents );

    QHBoxLayout* frameLayout = new QHBoxLayout( frame );
    frameLayout->setContentsMargins( 0, 0, 0, 0 );
    frameLayout->addWidget( d->w );

    connect( d->w, SIGNAL(changed()),
             this, SLOT(slotUpdateFilesizes()) );
    connect( d->w, SIGNAL(changed()),
             this, SLOT(slotUpdateFilenames()) );
    connect( d->w, SIGNAL(changed()),
             this, SLOT(slotUpdateVideoSizes()) );
    connect( d->audioModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
             this, SLOT(slotAudioModelChanged(QModelIndex,QModelIndex)) );

    setTitle( i18n("Video DVD Ripping"),
              i18np("1 title from %2", "%1 titles from %2", titles.count(),
                    k3bappcore->mediaCache()->medium(d->dvd.device()).beautifiedVolumeId() ) );

    // populate rip infos
    d->titleRipInfos.clear();
    for( QList<int>::const_iterator it = titles.begin(); it != titles.end(); ++it ) {
        if( *it > 0 && *it <= static_cast<int>( d->dvd.numTitles() ) ) {
            const VideoDVD::Title& title = d->dvd[ *it-1 ];
            K3b::VideoDVDRippingJob::TitleRipInfo ri( *it );
            ri.audioStream = d->audioModel->chosenAudio( title );
            d->titleRipInfos.insert( &title, ri );
        }
    }
}


K3b::VideoDVDRippingDialog::~VideoDVDRippingDialog()
{
    delete d;
}


void K3b::VideoDVDRippingDialog::slotUpdateFilenames()
{
    QString baseDir = K3b::prepareDir( d->w->m_editBaseDir->url().toLocalFile() );
    d->fsInfo.setPath( baseDir );

    for( TitleRipInfos::iterator it = d->titleRipInfos.begin(); it != d->titleRipInfos.end(); ++it ) {
        QString f = d->fsInfo.fixupPath( d->createFilename( it.value(), d->w->m_comboFilenamePattern->currentText() ) );
        if( d->w->m_checkBlankReplace->isChecked() )
            f.replace( QRegExp( "\\s" ), d->w->m_editBlankReplace->text() );
        it.value().filename = baseDir + f;

        d->audioModel->setFileName( *it.key(), f );
    }
}


void K3b::VideoDVDRippingDialog::slotUpdateFilesizes()
{
    double bitrate = (double)d->w->m_spinVideoBitrate->value();
    KIO::filesize_t overallSize = 0ULL;

    // update file sizes
    for( TitleRipInfos::iterator it = d->titleRipInfos.begin(); it != d->titleRipInfos.end(); ++it ) {

        double sec = d->dvd[it.value().title-1].playbackTime().totalSeconds();

        // estimate the filesize
        KIO::filesize_t size = (KIO::filesize_t)( sec * bitrate * 1000.0 / 8.0 );

        // add audio stream size
        // FIXME: consider AC3 passthrough
        size += (KIO::filesize_t)( sec * d->w->selectedAudioBitrate() / 8.0 * 1024.0 );

        d->audioModel->setFileSize( *it.key(), size );

        overallSize += size;
    }

    d->w->setNeededSize( overallSize );
}


void K3b::VideoDVDRippingDialog::slotUpdateVideoSizes()
{
    QSize size = d->w->selectedPictureSize();
    for( TitleRipInfos::iterator it = d->titleRipInfos.begin(); it != d->titleRipInfos.end(); ++it ) {
        QSize s( resizeTitle( d->dvd[it.value().title-1].videoStream(), size ) );
        d->audioModel->setVideoSize( *it.key(), s );
    }
}


void K3b::VideoDVDRippingDialog::slotAudioModelChanged( const QModelIndex& topLeft, const QModelIndex& /*bottomRight*/ )
{
    QModelIndex parent = topLeft.parent();
    if( parent.isValid() ) {
        if( const VideoDVD::Title* title = d->audioModel->titleForIndex( parent ) ) {
            d->titleRipInfos[ title ].audioStream = d->audioModel->chosenAudio( *title );
            slotUpdateFilenames();
        }
    }
}


void K3b::VideoDVDRippingDialog::setBaseDir( const QString& path )
{
    d->w->m_editBaseDir->setUrl(QUrl::fromLocalFile(path));
}


void K3b::VideoDVDRippingDialog::loadSettings( const KConfigGroup& c )
{
    d->w->m_spinVideoBitrate->setValue( c.readEntry( "video bitrate", 1200 ) );
    d->w->m_checkTwoPassEncoding->setChecked( c.readEntry( "two pass encoding", true ) );
    d->w->m_checkAudioResampling->setChecked( c.readEntry( "audio resampling", false ) );
    d->w->m_checkAutoClipping->setChecked( c.readEntry( "auto clipping", false ) );
    d->w->m_checkLowPriority->setChecked( c.readEntry( "low priority", true ) );
    d->w->m_checkAudioVBR->setChecked( c.readEntry( "vbr audio", true ) );
    d->w->setSelectedAudioBitrate( c.readEntry( "audio bitrate", 128 ) );
    d->w->setSelectedVideoCodec( videoCodecFromId( c.readEntry( "video codec", videoCodecId( K3b::VideoDVDTitleTranscodingJob::VIDEO_CODEC_FFMPEG_MPEG4 ) ) ) );
    d->w->setSelectedAudioCodec( audioCodecFromId( c.readEntry( "audio codec", audioCodecId( K3b::VideoDVDTitleTranscodingJob::AUDIO_CODEC_MP3 ) ) ) );
    d->w->m_checkBlankReplace->setChecked( c.readEntry( "replace blanks", false ) );
    d->w->m_editBlankReplace->setText( c.readEntry( "blank replace string", "_" ) );
    d->w->m_comboFilenamePattern->setEditText( c.readEntry( "filename pattern", d->w->m_comboFilenamePattern->itemText(0) ) );
    d->w->m_editBaseDir->setUrl(QUrl::fromLocalFile(c.readPathEntry("base dir", K3b::defaultTempPath())));
}


void K3b::VideoDVDRippingDialog::saveSettings( KConfigGroup c )
{
    c.writeEntry( "video bitrate", d->w->m_spinVideoBitrate->value() );
    c.writeEntry( "two pass encoding", d->w->m_checkTwoPassEncoding->isChecked() );
    c.writeEntry( "audio resampling", d->w->m_checkAudioResampling->isChecked() );
    c.writeEntry( "auto clipping", d->w->m_checkAutoClipping->isChecked() );
    c.writeEntry( "low priority", d->w->m_checkLowPriority->isChecked() );
    c.writeEntry( "vbr audio", d->w->m_checkAudioVBR->isChecked() );
    c.writeEntry( "audio bitrate", d->w->selectedAudioBitrate() );
    c.writeEntry( "video codec", videoCodecId( d->w->selectedVideoCodec() ) );
    c.writeEntry( "audio codec", audioCodecId( d->w->selectedAudioCodec() ) );
    c.writeEntry( "replace blanks", d->w->m_checkBlankReplace->isChecked() );
    c.writeEntry( "blank replace string", d->w->m_editBlankReplace->text() );
    c.writeEntry( "filename pattern", d->w->m_comboFilenamePattern->currentText() );
    c.writePathEntry( "base dir", d->w->m_editBaseDir->url().toLocalFile() );
}


void K3b::VideoDVDRippingDialog::slotStartClicked()
{
    //
    // check if the selected audio codec is usable for all selected audio streams
    // We can only use the AC3 pass-through mode for AC3 streams
    //
    if( d->w->selectedAudioCodec() == K3b::VideoDVDTitleTranscodingJob::AUDIO_CODEC_AC3_PASSTHROUGH ) {
        for( TitleRipInfos::iterator it = d->titleRipInfos.begin(); it != d->titleRipInfos.end(); ++it ) {
            if( d->dvd[it.value().title-1].numAudioStreams() > 0 &&
                d->dvd[it.value().title-1].audioStream(it.value().audioStream).format() != K3b::VideoDVD::AUDIO_FORMAT_AC3 ) {
                KMessageBox::sorry( this, i18n("<p>When using the <em>AC3 pass-through</em> audio codec all selected audio "
                                               "streams need to be in AC3 format. Please select another audio codec or "
                                               "choose AC3 audio streams for all ripped titles."),
                                    i18n("AC3 Pass-through") );
                return;
            }
        }
    }

    // check if we need to overwrite some files...
    QStringList filesToOverwrite;
    for( TitleRipInfos::iterator it = d->titleRipInfos.begin(); it != d->titleRipInfos.end(); ++it ) {
        if( QFile::exists( it.value().filename ) )
            filesToOverwrite.append( it.value().filename );
    }

    if( !filesToOverwrite.isEmpty() )
        if( KMessageBox::questionYesNoList( this,
                                            i18n("Do you want to overwrite these files?"),
                                            filesToOverwrite,
                                            i18n("Files Exist"),
                                            KStandardGuiItem::overwrite(),
                                            KStandardGuiItem::cancel() ) == KMessageBox::No )
            return;


    QSize videoSize = d->w->selectedPictureSize();
    int i = 0;
    QVector<K3b::VideoDVDRippingJob::TitleRipInfo> titles( d->titleRipInfos.count() );
    for( TitleRipInfos::const_iterator it = d->titleRipInfos.constBegin(); it != d->titleRipInfos.constEnd(); ++it ) {
        titles[i] = it.value();
        titles[i].videoBitrate = 0; // use the global bitrate set below
        titles[i].width = videoSize.width();
        titles[i].height = videoSize.height();
        ++i;
    }

    // sort the titles which come from a map and are thus not sorted properly
    // simple bubble sort for these small arrays is sufficient
    for( int i = 0; i < titles.count(); ++i ) {
        for( int j = i+1; j < titles.count(); ++j ) {
            if( titles[i].title > titles[j].title ) {
                K3b::VideoDVDRippingJob::TitleRipInfo tmp = titles[i];
                titles[i] = titles[j];
                titles[j] = tmp;
            }
        }
    }

    // start the job
    K3b::JobProgressDialog dlg( parentWidget() );
    K3b::VideoDVDRippingJob* job = new K3b::VideoDVDRippingJob( &dlg, &dlg );
    job->setVideoDVD( d->dvd );
    job->setTitles( titles );

    job->setVideoBitrate( d->w->m_spinVideoBitrate->value() );
    job->setTwoPassEncoding( d->w->m_checkTwoPassEncoding->isChecked() );
    job->setResampleAudioTo44100( d->w->m_checkAudioResampling->isChecked() );
    job->setAutoClipping( d->w->m_checkAutoClipping->isChecked() );
    job->setVideoCodec( d->w->selectedVideoCodec() );
    job->setAudioCodec( d->w->selectedAudioCodec() );
    job->setLowPriority( d->w->m_checkLowPriority->isChecked() );
    job->setAudioBitrate( d->w->selectedAudioBitrate() );
    job->setAudioVBR( d->w->m_checkAudioVBR->isChecked() );

    hide();
    dlg.startJob( job );
    close();
}


