/*
 *
 * Copyright (C) 2006-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009 Michal Malek <michalm@jabster.pl>
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

#include "k3bvideodvdtitlemodel.h"
#include "k3bapplication.h"
#include "k3bmediacache.h"
#include "k3bmedium.h"
#include "k3bvideodvd.h"
#include "k3bvideodvdrippingpreview.h"
#include "k3bvideodvdtitle.h"

#include <KLocalizedString>

#include <QHash>
#include <QLocale>
#include <QSet>
#include <QStringList>
#include <QImage>
#include <QPixmap>

namespace K3b {

namespace
{

const unsigned int MAX_LINES = 2;
const unsigned int TOOLTIP_MAX_LINES = 9999;


QStringList audioStreamString( const K3b::VideoDVD::Title& title )
{
    QStringList list;

    if( title.numAudioStreams() > 0 ) {
        for( unsigned int i = 0; i < qMin( title.numAudioStreams(), MAX_LINES ); ++i ) {
            list << QString::number(i+1) + ": "
                + i18n("%1 %2Ch (%3)",
                        K3b::VideoDVD::audioFormatString( title.audioStream(i).format() ),
                        title.audioStream(i).channels(),
                        title.audioStream(i).langCode().isEmpty()
                            ? i18n("unknown language")
                            : QLocale( title.audioStream(i).langCode() ).nativeLanguageName() );
        }
        if( title.numAudioStreams() > MAX_LINES )
            list.last() += "...";
    }
    else {
        list << i18n("No audio streams");
    }
    return list;
}


QString audioStreamStringToolTip( const K3b::VideoDVD::Title& title )
{
    QString s = "<p><b>" + i18n("Audio Streams") + "</b><p>";
    for( unsigned int i = 0; i < qMin( title.numAudioStreams(), TOOLTIP_MAX_LINES ); ++i ) {
        if( i > 0 )
            s += "<br>";
        s += QString::number(i+1) + ": "
             + i18n("%1 %2Ch (%3<em>%4</em>)",
                    K3b::VideoDVD::audioFormatString( title.audioStream(i).format() ),
                    title.audioStream(i).channels(),
                    title.audioStream(i).langCode().isEmpty()
                        ? i18n("unknown language")
                        : QLocale( title.audioStream(i).langCode() ).nativeLanguageName(),
                    title.audioStream(i).codeExtension() != K3b::VideoDVD::AUDIO_CODE_EXT_UNSPECIFIED
                        ? QString(" ") + K3b::VideoDVD::audioCodeExtensionString( title.audioStream(i).codeExtension() )
                        : QString() );
    }
    if( title.numAudioStreams() > TOOLTIP_MAX_LINES )
        s += "...";

    return s;
}


QStringList subpictureStreamString( const K3b::VideoDVD::Title& title )
{
    QStringList list;

    if( title.numSubPictureStreams() > 0 ) {
        for( unsigned int i = 0; i < qMin( title.numSubPictureStreams(), MAX_LINES ); ++i ) {
            list << QString::number(i+1) + ": "
                + QString("%1 (%2)")
                .arg( title.subPictureStream(i).codeMode() == K3b::VideoDVD::SUBPIC_CODE_MODE_RLE
                    ? i18n("RLE")
                    : i18n("Extended") )
                .arg( title.subPictureStream(i).langCode().isEmpty()
                    ? i18n("unknown language")
                    : QLocale( title.subPictureStream(i).langCode() ).nativeLanguageName() );
        }
        if( title.numSubPictureStreams() > MAX_LINES )
            list.last() += "...";
    }
    else {
        list << i18n("No Subpicture streams");
    }
    return list;
}


QString subpictureStreamStringToolTip( const K3b::VideoDVD::Title& title )
{
    QString s = "<p><b>" + i18n("Subpicture Streams") + "</b><p>";
    for( unsigned int i = 0; i < qMin( title.numSubPictureStreams(), TOOLTIP_MAX_LINES ); ++i ) {
        if( i > 0 )
            s += "<br>";
        s += QString::number(i+1) + ": "
             + QString("%1 (%2<em>%3</em>)")
             .arg( title.subPictureStream(i).codeMode() == K3b::VideoDVD::SUBPIC_CODE_MODE_RLE
                   ? i18n("RLE")
                   : i18n("Extended") )
             .arg( title.subPictureStream(i).langCode().isEmpty()
                   ? i18n("unknown language")
                   : QLocale( title.subPictureStream(i).langCode() ).nativeLanguageName() )
             .arg( title.subPictureStream(i).codeExtension() != K3b::VideoDVD::SUBPIC_CODE_EXT_UNSPECIFIED
                   ? QString(" ") + K3b::VideoDVD::subPictureCodeExtensionString( title.subPictureStream(i).codeExtension() )
                   : QString() );
    }
    if( title.numSubPictureStreams() > TOOLTIP_MAX_LINES )
        s += "...";

    return s;
}

} // namespace


class VideoDVDTitleModel::Private
{
public:
    typedef QSet<int> Titles;
    typedef QHash<const VideoDVD::Title*,QPixmap> Previews;

    VideoDVD::VideoDVD dvd;
    Titles selectedTitles;
    Previews previews;
    VideoDVDRippingPreview* previewGen;
    unsigned int currentPreviewTitle;
    bool previewGenStopped;
    Medium medium;
};


VideoDVDTitleModel::VideoDVDTitleModel( QObject* parent )
:
    QAbstractTableModel( parent ),
    d( new Private )
{
    d->currentPreviewTitle = 0;
    d->previewGen = new K3b::VideoDVDRippingPreview( this );
    d->previewGenStopped = true;
    connect( d->previewGen, SIGNAL(previewDone(bool)),
             this, SLOT(slotPreviewDone(bool)) );
}


VideoDVDTitleModel::~VideoDVDTitleModel()
{
    delete d;
}


void VideoDVDTitleModel::setVideoDVD( const VideoDVD::VideoDVD& dvd )
{
    beginResetModel();
    d->previewGen->cancel();
    d->dvd = dvd;
    d->selectedTitles.clear();
    d->previews.clear();
    d->currentPreviewTitle = 0;
    d->medium = k3bappcore->mediaCache()->medium( d->dvd.device() );
    d->previewGenStopped = false;
    d->previewGen->generatePreview( d->dvd, d->currentPreviewTitle+1 );
    endResetModel();
    checkAll();
}


QList<int> VideoDVDTitleModel::selectedTitles() const
{
    return d->selectedTitles.toList();
}


Qt::ItemFlags VideoDVDTitleModel::flags( const QModelIndex& index ) const
{
    if( index.isValid() ) {
        if( index.column() == TitleColumn )
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
        else
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }
    else
        return QAbstractTableModel::flags( index );
}


QVariant VideoDVDTitleModel::data( const QModelIndex& index, int role ) const
{
    if( !index.isValid() || index.row() >= static_cast<int>( d->dvd.numTitles() ) )
        return QVariant();

    const VideoDVD::Title& title = d->dvd.title( index.row() );

    if( Qt::DisplayRole == role || Qt::EditRole == role ) {
        switch( index.column() ) {
            case TitleColumn:
                return i18n( "Title %1 (%2)",
                        QString::number( title.titleNumber() ).rightJustified( 2 ),
                        title.playbackTime().toString( false ) );

            case VideoColumn:
                return QString("%1 %2x%3")
                    .arg( title.videoStream().mpegVersion() == 0 ? i18n("MPEG1") : i18n("MPEG2") )
                    .arg( title.videoStream().pictureWidth() )
                    .arg( title.videoStream().pictureHeight() );

            case AudioColumn:
                return audioStreamString( title ).join( ", " );

            case SubpictureColumn:
                return subpictureStreamString( title ).join( "," );

            default:
                break;
        }

    }
    else if( Qt::ToolTipRole == role ) {
        if( AudioColumn == index.column() && title.numAudioStreams() > 0 ) {
            return audioStreamStringToolTip( title );
        }
        else if( SubpictureColumn == index.column() && title.numSubPictureStreams() > 0 ) {
            return subpictureStreamStringToolTip( title );
        }
    }
    else if( Qt::CheckStateRole == role && index.column() == TitleColumn ) {
        if( d->selectedTitles.contains( title.titleNumber() ) )
            return Qt::Checked;
        else
            return Qt::Unchecked;
    }
    else if( ChaptersRole == role ) {
        return i18np("%1 chapter", "%1 chapters", title.numPTTs() );
    }
    else if( PreviewRole == role ) {
        Private::Previews::const_iterator preview = d->previews.constFind( &title );
        if( preview != d->previews.constEnd() )
            return preview.value();
    }
    else if( AspectRatioRole == role ) {
        QString aspectRatio( title.videoStream().displayAspectRatio() == K3b::VideoDVD::VIDEO_ASPECT_RATIO_4_3 ? "4:3" : "16:9" );
        if( title.videoStream().letterboxed() )
            return QString::fromLatin1("%1 - %2").arg(aspectRatio).arg(i18n("letterboxed"));
        else if( title.videoStream().permittedDf() == K3b::VideoDVD::VIDEO_PERMITTED_DF_LETTERBOXED )
            return QString::fromLatin1("%1 - %2").arg(aspectRatio).arg(i18n("anamorph"));
        else
            return aspectRatio;
    }
    else if( AudioStreamsRole == role ) {
        return audioStreamString( title );
    }
    else if( SubpictureStreamsRole == role ) {
        return subpictureStreamString( title );
    }
    return QVariant();
}


bool VideoDVDTitleModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
    if( !index.isValid() || index.row() >= static_cast<int>( d->dvd.numTitles() ) || role != Qt::CheckStateRole )
        return false;

    const VideoDVD::Title& title = d->dvd.title( index.row() );

    if( value.toInt() == Qt::Checked )
        d->selectedTitles.insert( title.titleNumber() );
    else
        d->selectedTitles.remove( title.titleNumber() );

    emit dataChanged( index, index );
    return true;
}


QVariant VideoDVDTitleModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    Q_UNUSED( orientation );

    if( Qt::DisplayRole == role ) {
        switch( section ) {
            case TitleColumn:
                return i18n("Title");
            case PreviewColumn:
                return i18n("Preview");
            case VideoColumn:
                return i18n("Video");
            case AudioColumn:
                return i18n("Audio");
            case SubpictureColumn:
                return i18n("Subpicture");
            default:
                break;
        }
    }
    return QVariant();
}


int VideoDVDTitleModel::rowCount( const QModelIndex& parent ) const
{
    if( d->dvd.valid() && !parent.isValid() )
        return d->dvd.numTitles();
    else
        return 0;
}


int VideoDVDTitleModel::columnCount( const QModelIndex& parent ) const
{
    Q_UNUSED( parent );
    return NumColumns;
}


QModelIndex VideoDVDTitleModel::buddy( const QModelIndex& index ) const
{
    if( index.isValid() && index.column() != TitleColumn )
        return QAbstractTableModel::index( index.row(), TitleColumn );
    else
        return index;
}


void VideoDVDTitleModel::checkAll()
{
    if( d->dvd.valid() ) {
        for( unsigned int i = 0; i < d->dvd.numTitles(); ++i ) {
            d->selectedTitles.insert( d->dvd.title(i).titleNumber() );
        }
        emit dataChanged( index(0,TitleColumn), index(d->dvd.numTitles()-1,TitleColumn) );
    }
}


void VideoDVDTitleModel::uncheckAll()
{
    if( d->dvd.valid() ) {
        d->selectedTitles.clear();
        emit dataChanged( index(0,TitleColumn), index(d->dvd.numTitles()-1,TitleColumn) );
    }
}


void VideoDVDTitleModel::stopPreviewGen()
{
    d->previewGenStopped = true;
    d->previewGen->cancel();
}


void VideoDVDTitleModel::slotPreviewDone( bool success )
{
    const VideoDVD::Title& title = d->dvd.title( d->currentPreviewTitle );

    if( success )
        d->previews.insert( &title, QPixmap::fromImage( d->previewGen->preview() ) );
    else
        d->previews.remove( &title );

    emit dataChanged( index(d->currentPreviewTitle,PreviewColumn), index(d->currentPreviewTitle,PreviewColumn) );

    // cancel if we previously stopped preview generation or if the medium changed.
    if( !d->previewGenStopped && d->medium == k3bappcore->mediaCache()->medium( d->dvd.device() ) ) {
        ++d->currentPreviewTitle;
        if( d->currentPreviewTitle < d->dvd.numTitles() )
            d->previewGen->generatePreview( d->dvd, d->currentPreviewTitle+1 );
    }
}

} // namespace K3b


