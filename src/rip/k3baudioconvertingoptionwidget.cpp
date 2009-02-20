/*
 *
 * Copyright (C) 2004-2008 Sebastian Trueg <trueg@k3b.org>
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

#include "k3baudioconvertingoptionwidget.h"

#include <k3bpluginmanager.h>
#include <k3baudioencoder.h>
#include <k3bcore.h>
#include <k3bglobals.h>

#include <kcombobox.h>
#include <kurlrequester.h>
#include <kio/global.h>
#include <kconfig.h>
#include <klocale.h>
#include <kiconloader.h>

#include <q3intdict.h>
#include <qmap.h>
#include <qlabel.h>
#include <qtimer.h>
#include <qtoolbutton.h>
#include <qcheckbox.h>



class K3bAudioConvertingOptionWidget::Private
{
public:
    Q3IntDict<K3bAudioEncoder> encoderMap;
    QMap<int, QString> extensionMap;

    QTimer freeSpaceUpdateTimer;

    KIO::filesize_t neededSize;

    int getDefaultFormat() {
        // we prefere formats in this order:
        // 1. ogg
        // 2. mp3
        // 3. flac
        // 4. wave
        int ogg = -1;
        int mp3 = -1;
        int flac = -1;
        for( QMap<int, QString>::const_iterator it = extensionMap.constBegin();
             it != extensionMap.constEnd(); ++it ) {
            if( it.value() == "ogg" )
                ogg = it.key();
            else if( it.value() == "mp3" )
                mp3 = it.key();
            else if( it.value() == "flac" )
                flac = it.key();
        }

        if( ogg > -1 )
            return ogg;
        else if( mp3 > -1 )
            return mp3;
        else if( flac > -1 )
            return flac;
        else
            return 0;
    }
};


K3bAudioConvertingOptionWidget::K3bAudioConvertingOptionWidget( QWidget* parent )
    : base_K3bAudioRippingOptionWidget( parent )
{
    d = new Private();

    connect( m_editBaseDir, SIGNAL(textChanged(const QString&)),
             this, SLOT(slotUpdateFreeTempSpace()) );
    connect( m_comboFileType, SIGNAL(activated(int)),
             this, SLOT(slotEncoderChanged()) );
    connect( &d->freeSpaceUpdateTimer, SIGNAL(timeout()),
             this, SLOT(slotUpdateFreeTempSpace()) );
    connect( m_checkCreatePlaylist, SIGNAL(toggled(bool)), this, SIGNAL(changed()) );
    connect( m_checkSingleFile, SIGNAL(toggled(bool)), this, SIGNAL(changed()) );
    connect( m_checkWriteCueFile, SIGNAL(toggled(bool)), this, SIGNAL(changed()) );
    connect( m_comboFileType, SIGNAL(activated(int)), this, SIGNAL(changed()) );
    connect( m_editBaseDir, SIGNAL(textChanged(const QString&)), this, SIGNAL(changed()) );
    connect( m_buttonConfigurePlugin, SIGNAL(clicked()), this, SLOT(slotConfigurePlugin()) );

    m_editBaseDir->setMode( KFile::Directory | KFile::ExistingOnly | KFile::LocalOnly );
    m_buttonConfigurePlugin->setIcon( KIcon( "system-run" ) );

    // FIXME: see if sox and the sox encoder are installed and if so do not put the internal wave
    //        writer in the list of encoders.

    d->encoderMap.clear();
    d->extensionMap.clear();
    m_comboFileType->clear();
    m_comboFileType->addItem( i18n("Wave") );
    d->extensionMap[0] = "wav";

    // check the available encoding plugins
    QList<K3bPlugin*> fl = k3bcore->pluginManager()->plugins( "AudioEncoder" );
    for( QList<K3bPlugin *>::const_iterator it = fl.constBegin();
         it != fl.constEnd(); ++it ) {
        K3bAudioEncoder* f = (K3bAudioEncoder*)(*it);
        QStringList exL = f->extensions();

        for( QStringList::const_iterator exIt = exL.constBegin();
             exIt != exL.constEnd(); ++exIt ) {
            d->extensionMap.insert( m_comboFileType->count(), *exIt );
            d->encoderMap.insert( m_comboFileType->count(), f );
            m_comboFileType->addItem( f->fileTypeComment(*exIt) );
        }
    }

    // refresh every 2 seconds
    d->freeSpaceUpdateTimer.start(2000);
    slotUpdateFreeTempSpace();
}


K3bAudioConvertingOptionWidget::~K3bAudioConvertingOptionWidget()
{
    delete d;
}


QString K3bAudioConvertingOptionWidget::baseDir() const
{
    return m_editBaseDir->url().path();
}


void K3bAudioConvertingOptionWidget::setBaseDir( const QString& path )
{
    m_editBaseDir->setUrl( path );
}


void K3bAudioConvertingOptionWidget::setNeededSize( KIO::filesize_t size )
{
    d->neededSize = size;
    if( size > 0 )
        m_labelNeededSpace->setText( KIO::convertSize( size ) );
    else
        m_labelNeededSpace->setText( i18n("unknown") );

    slotUpdateFreeTempSpace();
}


void K3bAudioConvertingOptionWidget::slotConfigurePlugin()
{
    // 0 for wave
    K3bAudioEncoder* encoder = d->encoderMap[m_comboFileType->currentIndex()];
    if( encoder )
        k3bcore->pluginManager()->execPluginDialog( encoder, this );
}


void K3bAudioConvertingOptionWidget::slotUpdateFreeTempSpace()
{
    QString path = m_editBaseDir->url().url();

    if( !QFile::exists( path ) )
        path.truncate( path.lastIndexOf('/') );

    QPalette pal( m_labelNeededSpace->palette() );

    unsigned long size, avail;
    if( K3b::kbFreeOnFs( path, size, avail ) ) {
        m_labelFreeSpace->setText( KIO::convertSizeFromKiB(avail) );

        pal.setColor( m_labelNeededSpace->foregroundRole(),
                      avail < d->neededSize/1024
                      ? Qt::red
                      : palette().color( QPalette::Text ) );
    }
    else {
        m_labelFreeSpace->setText("-");
        pal.setColor( m_labelNeededSpace->foregroundRole(), palette().color( QPalette::Text ) );
    }

    m_labelNeededSpace->setPalette( pal );
}


void K3bAudioConvertingOptionWidget::slotEncoderChanged()
{
    // 0 for wave
    m_buttonConfigurePlugin->setEnabled( d->encoderMap[m_comboFileType->currentIndex()] != 0 );
}


K3bAudioEncoder* K3bAudioConvertingOptionWidget::encoder() const
{
    return d->encoderMap[m_comboFileType->currentIndex()];  // 0 for wave
}


QString K3bAudioConvertingOptionWidget::extension() const
{
    return d->extensionMap[m_comboFileType->currentIndex()];
}


void K3bAudioConvertingOptionWidget::loadDefaults()
{
    m_editBaseDir->setUrl( QDir::homePath() );
    m_checkSingleFile->setChecked( false );
    m_checkWriteCueFile->setChecked( false );
    m_comboFileType->setCurrentIndex( d->getDefaultFormat() );
    m_checkCreatePlaylist->setChecked(false);
    m_checkPlaylistRelative->setChecked(false);

    slotEncoderChanged();
}


void K3bAudioConvertingOptionWidget::loadConfig( const KConfigGroup& c )
{
    m_editBaseDir->setUrl( c.readEntry( "last ripping directory", QDir::homePath() ) );

    m_checkSingleFile->setChecked( c.readEntry( "single_file", false ) );
    m_checkWriteCueFile->setChecked( c.readEntry( "write_cue_file", false ) );

    m_checkCreatePlaylist->setChecked( c.readEntry( "create_playlist", false ) );
    m_checkPlaylistRelative->setChecked( c.readEntry( "relative_path_in_playlist", false ) );

    QString filetype = c.readEntry( "filetype", d->extensionMap[d->getDefaultFormat()] );
    if( filetype == "wav" )
        m_comboFileType->setCurrentIndex(0);
    else {
        for( QMap<int, QString>::ConstIterator it = d->extensionMap.constBegin();
             it != d->extensionMap.constEnd(); ++it ) {
            if( it.value() == filetype ) {
                m_comboFileType->setCurrentIndex( it.key() );
                break;
            }
        }
    }

    slotEncoderChanged();
}


void K3bAudioConvertingOptionWidget::saveConfig( KConfigGroup c )
{
    c.writePathEntry( "last ripping directory", m_editBaseDir->url().url() );

    c.writeEntry( "single_file", m_checkSingleFile->isChecked() );
    c.writeEntry( "write_cue_file", m_checkWriteCueFile->isChecked() );

    c.writeEntry( "create_playlist", m_checkCreatePlaylist->isChecked() );
    c.writeEntry( "relative_path_in_playlist", m_checkPlaylistRelative->isChecked() );

    if( d->extensionMap.contains(m_comboFileType->currentIndex()) )
        c.writeEntry( "filetype", d->extensionMap[m_comboFileType->currentIndex()] );
    else
        c.writeEntry( "filetype", "wav" );
}

#include "k3baudioconvertingoptionwidget.moc"
