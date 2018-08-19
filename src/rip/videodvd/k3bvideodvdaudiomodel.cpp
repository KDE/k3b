/*
 *
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

#include "k3bvideodvdaudiomodel.h"
#include "k3bvideodvd.h"
#include "k3bvideodvdtitle.h"

#include <KLocalizedString>

#include <QHash>
#include <QLocale>
#include <QSize>

namespace K3b {
    
class VideoDVDAudioModel::Private
{
public:
    Private( const VideoDVD::VideoDVD& d, const QList<int>& t )
        : dvd( d ), titles( t ) {}
    
    VideoDVD::VideoDVD dvd;
    QList<int> titles;
    QHash< const VideoDVD::AudioStream*, const VideoDVD::Title* > parents;
    QHash< const VideoDVD::Title*, QSize > videoSizes;
    QHash< const VideoDVD::Title*, KIO::filesize_t > fileSizes;
    QHash< const VideoDVD::Title*, QString > fileNames;
    QHash< const VideoDVD::Title*, int > chosenAudio;
};


VideoDVDAudioModel::VideoDVDAudioModel( const VideoDVD::VideoDVD& dvd, const QList<int>& titles, QObject* parent )
    : QAbstractItemModel( parent ),
      d( new Private( dvd, titles ) )
{
    Q_FOREACH( int titleNum, titles ) {
        if( titleNum > 0 && titleNum <= static_cast<int>( d->dvd.numTitles() ) ) {
            const VideoDVD::Title& title = d->dvd[ titleNum-1 ];
            
            d->videoSizes.insert( &title, QSize( title.videoStream().realPictureWidth(),
                                                 title.videoStream().realPictureHeight() ) );
            d->fileSizes.insert( &title, 0 );
            d->fileNames.insert( &title, QString() );
            d->chosenAudio.insert( &title, 0 );
            
            for( int i = 0; i < static_cast<int>( title.numAudioStreams() ); ++i ) {
                const VideoDVD::AudioStream& audio = title.audioStream( i );
                d->parents.insert( &audio, &title );
                if( QLocale( audio.langCode() ).language() == QLocale().language() &&
                    audio.format() != K3b::VideoDVD::AUDIO_FORMAT_DTS ) {
                    d->chosenAudio[ &title ] = i;
                }
            }
        }
    }
}


VideoDVDAudioModel::~VideoDVDAudioModel()
{
    delete d;
}


const VideoDVD::Title* VideoDVDAudioModel::titleForIndex( const QModelIndex& index ) const
{
    if( index.isValid() && !index.internalPointer() && index.row() >= 0 && index.row() < d->titles.size() )
    {
        const int title = d->titles.at( index.row() ) - 1;
        if( title >= 0 && title < static_cast<int>( d->dvd.numTitles() ) )
            return &d->dvd[ title ];
    }
    return 0;
}


QModelIndex VideoDVDAudioModel::indexForTitle( const VideoDVD::Title& title, int column ) const
{
    int row = d->titles.indexOf( title.titleNumber() );
    if( row >= 0 )
        return createIndex( row, column, nullptr );
    else
        return QModelIndex();
}

        
const VideoDVD::AudioStream* VideoDVDAudioModel::audioForIndex( const QModelIndex& index ) const
{
    if( index.isValid() && index.internalPointer() )
        return static_cast<VideoDVD::AudioStream*>( index.internalPointer() );
    else
        return 0;
}


QModelIndex VideoDVDAudioModel::indexForAudio( const VideoDVD::AudioStream& audio, int column ) const
{
    if( const VideoDVD::Title* title = d->parents[ &audio ] ) {
        for( int i = 0; i < static_cast<int>( title->numAudioStreams() ); ++i ) {
            if( &title->audioStream( i ) == &audio ) {
                return createIndex( i, column, const_cast<void*>( reinterpret_cast<const void*>( &audio ) ) );
            }
        }
    }
    return QModelIndex();
}


void VideoDVDAudioModel::setVideoSize( const VideoDVD::Title& title, const QSize& videoSize )
{
    d->videoSizes[ &title ] = videoSize;
    QModelIndex index = indexForTitle( title, VideoSizeColumn );
    Q_EMIT dataChanged( index, index );
}


void VideoDVDAudioModel::setFileSize( const VideoDVD::Title& title, KIO::filesize_t fileSize )
{
    d->fileSizes[ &title ] = fileSize;
    QModelIndex index = indexForTitle( title, FileSizeColumn );
    Q_EMIT dataChanged( index, index );
}


void VideoDVDAudioModel::setFileName( const VideoDVD::Title& title, const QString& fileName )
{
    d->fileNames[ &title ] = fileName;
    QModelIndex index = indexForTitle( title, FileNameColumn );
    Q_EMIT dataChanged( index, index );
}
        

int VideoDVDAudioModel::chosenAudio( const VideoDVD::Title& title ) const
{
    return d->chosenAudio[ &title ];
}


QVariant VideoDVDAudioModel::data( const QModelIndex& index, int role ) const
{
    if( const VideoDVD::Title* title = titleForIndex( index ) ) {
        if( Qt::DisplayRole == role ) {
            switch( index.column() ) {
                case TitleColumn:
                    return i18n("Title %1 (%2)",
                                title->titleNumber(),
                                title->playbackTime().toString() );
                case VideoSizeColumn:
                    return QString("%1x%2")
                            .arg( d->videoSizes[ title ].width() )
                            .arg( d->videoSizes[ title ].height() );
                case FileSizeColumn:
                    return KIO::convertSize( d->fileSizes[ title ] );
                case FileNameColumn:
                    return d->fileNames[ title ];
                default:
                    break;
            }
        }
        else if( Qt::ToolTipRole == role && index.column() == FileNameColumn ) {
            return d->fileNames[ title ];
        }
    }
    else if( const VideoDVD::AudioStream* audio = audioForIndex( index ) ) {
        if( Qt::DisplayRole == role && index.column() == TitleColumn ) {
            QString text = i18n("%1 %2Ch (%3%4)",
                           K3b::VideoDVD::audioFormatString( audio->format() ),
                           audio->channels(),
                                ( audio->langCode().isEmpty()
                                  ? i18n("unknown language")
                                  : QLocale( audio->langCode() ).nativeLanguageName() ),
                                ( audio->codeExtension() != K3b::VideoDVD::AUDIO_CODE_EXT_UNSPECIFIED
                                  ? QString(" ") + K3b::VideoDVD::audioCodeExtensionString( audio->codeExtension() )
                                  : QString() ) );
            
            if( audio->format() == K3b::VideoDVD::AUDIO_FORMAT_DTS )
                return i18n( "%1 (not supported)", text );
            else
                return text;
        }
        else if( Qt::CheckStateRole == role && index.column() == TitleColumn ) {
            if( d->chosenAudio[ d->parents[ audio ] ] == index.row() )
                return Qt::Checked;
            else
                return Qt::Unchecked;
        }
    }
    return QVariant();
}


int VideoDVDAudioModel::columnCount( const QModelIndex& /*parent*/ ) const
{
    return NumColumns;
}


int VideoDVDAudioModel::rowCount( const QModelIndex& parent ) const
{
    if( !parent.isValid() )
        return d->titles.count();
    else if( const VideoDVD::Title* title = titleForIndex( parent ) )
        return title->numAudioStreams();
    else
        return 0;
}


QModelIndex VideoDVDAudioModel::parent( const QModelIndex& child ) const
{
    if( const VideoDVD::AudioStream* audio = audioForIndex( child ) ) {
        if( const VideoDVD::Title* title = d->parents[ audio ] ) {
            return indexForTitle( *title );
        }
    }
    return QModelIndex();
}


QModelIndex VideoDVDAudioModel::index( int row, int column, const QModelIndex& parent ) const
{
    if( !hasIndex( row, column, parent ) ) {
        return QModelIndex();
    }
    else if( !parent.isValid() ) {
        if( row >= 0 && row < d->titles.size() )
            return createIndex( row, column, nullptr );
        else
            return QModelIndex();
    }
    else if( const VideoDVD::Title* title = titleForIndex( parent ) ) {
        if( row >= 0 && row < static_cast<int>( title->numAudioStreams() ) )
            return createIndex( row, column, const_cast<void*>( reinterpret_cast<const void*>( &title->audioStream( row ) ) ) );
        else
            return QModelIndex();
    }
    else {
        return QModelIndex();
    }
}


QVariant VideoDVDAudioModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if( orientation == Qt::Horizontal && Qt::DisplayRole == role ) {
        switch( section ) {
            case TitleColumn: return i18n("Title");
            case VideoSizeColumn: return i18n("Video Size");
            case FileSizeColumn: return i18n("File Size");
            case FileNameColumn: return i18n("Filename");
            default: break;
        }
    }
    return QVariant();
}


bool VideoDVDAudioModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
    if( role == Qt::CheckStateRole && value.toBool() ) {
        if( const VideoDVD::AudioStream* audio = audioForIndex( index ) ) {
            if( const VideoDVD::Title* title = d->parents[ audio ] ) {
                d->chosenAudio[ title ] = index.row();
                QModelIndex titleIndex = indexForTitle( *title );
                Q_EMIT dataChanged( VideoDVDAudioModel::index( 0, TitleColumn, titleIndex ),
                                    VideoDVDAudioModel::index( rowCount( titleIndex )-1, TitleColumn, titleIndex ) );
            }
        }
    }
    return false;
}


Qt::ItemFlags VideoDVDAudioModel::flags( const QModelIndex& index ) const
{
    if( audioForIndex( index ) != 0 )
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
    else if( titleForIndex( index ) != 0 )
        return Qt::ItemIsEnabled;
    else
        return Qt::NoItemFlags;
}

} // namespace K3b



