/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bmovixdocpreparer.h"
#include "k3bmovixdoc.h"
#include "k3bmovixprogram.h"
#include "k3bmovixfileitem.h"

#include "k3bcore.h"
#include "k3bdiritem.h"
#include "k3bfileitem.h"
#include "k3bbootitem.h"
#include "k3bexternalbinmanager.h"
#include "k3bisoimager.h"

#include <KLocale>
#include <KDebug>
#include <KTemporaryFile>
#include <kio/global.h>

#include <QTextStream>
#include <QDir>
#include <QStack>


class K3b::MovixDocPreparer::Private
{
public:
    Private()
        : doc(0),
          playlistFile(0),
          isolinuxConfigFile(0),
          movixRcFile(0),
          isolinuxDir(0),
          movixDir(0),
          mplayerDir(0),
          playlistFileItem(0),
          structuresCreated(false) {
    }

    K3b::MovixDoc* doc;
    const K3b::MovixBin* eMovixBin;

    KTemporaryFile* playlistFile;
    KTemporaryFile* isolinuxConfigFile;
    KTemporaryFile* movixRcFile;

    K3b::DirItem* isolinuxDir;
    K3b::DirItem* movixDir;
    K3b::DirItem* mplayerDir;
    K3b::FileItem* playlistFileItem;

    QStack<K3b::DataItem*> newMovixItems;

    bool structuresCreated;
};


K3b::MovixDocPreparer::MovixDocPreparer( K3b::MovixDoc* doc, K3b::JobHandler* jh, QObject* parent )
    : K3b::Job( jh, parent )
{
    d = new Private();
    d->doc = doc;
}


K3b::MovixDocPreparer::~MovixDocPreparer()
{
    removeMovixStructures();
    delete d;
}


K3b::MovixDoc* K3b::MovixDocPreparer::doc() const
{
    return d->doc;
}


void K3b::MovixDocPreparer::start()
{
    kDebug() << k_funcinfo;
    jobStarted();

    bool success = true;
    if( d->structuresCreated )
        removeMovixStructures();
    else
        success = createMovixStructures();

    jobFinished(success);
}


void K3b::MovixDocPreparer::cancel()
{
    // do nothing...
}


bool K3b::MovixDocPreparer::createMovixStructures()
{
    kDebug() << k_funcinfo;
    removeMovixStructures();

    if( doc() ) {
        doc()->setMultiSessionMode( K3b::DataDoc::NONE );
        doc()->prepareFilenames();
    }

    d->eMovixBin = dynamic_cast<const K3b::MovixBin*>( k3bcore->externalBinManager()->binObject("eMovix") );
    if( d->eMovixBin ) {
        bool success = false;
        if( d->eMovixBin->version() >= K3b::Version( 0, 9, 0 ) )
            success = addMovixFilesNew();
        else
            success = addMovixFiles();

        d->structuresCreated = success;
        return success;
    }
    else {
        emit infoMessage( i18n("Could not find a valid eMovix installation."), MessageError );
        return false;
    }
}


void K3b::MovixDocPreparer::removeMovixStructures()
{
    kDebug() << k_funcinfo;
    // remove movix files from doc
    // the dataitems do the cleanup in the doc
    delete d->movixDir;
    delete d->isolinuxDir;
    delete d->mplayerDir;
    delete d->playlistFileItem;

    d->movixDir = 0;
    d->isolinuxDir = 0;
    d->mplayerDir = 0;
    d->playlistFileItem = 0;

    while( !d->newMovixItems.empty() ) {
        delete d->newMovixItems.pop();
    }

    // remove all the temp files
    delete d->playlistFile;
    delete d->isolinuxConfigFile;
    delete d->movixRcFile;

    d->playlistFile = 0;
    d->isolinuxConfigFile = 0;
    d->movixRcFile = 0;

    d->structuresCreated = false;
}


bool K3b::MovixDocPreparer::writePlaylistFile()
{
    delete d->playlistFile;
    d->playlistFile = new KTemporaryFile();
    d->playlistFile->open();

    QTextStream s( d->playlistFile );

    QList<K3b::MovixFileItem*> movixFileItems = d->doc->movixFileItems();

    Q_FOREACH( K3b::MovixFileItem* item, movixFileItems ) {
        s << "/cdrom/";
        s << item->writtenName();
        s << endl;
    }

    d->playlistFile->close();
    return true;
}


bool K3b::MovixDocPreparer::writeIsolinuxConfigFile( const QString& originalPath )
{
    delete d->isolinuxConfigFile;
    d->isolinuxConfigFile = new KTemporaryFile();
    d->isolinuxConfigFile->open();

    QTextStream s( d->isolinuxConfigFile );

    // now open the default isolinux.cfg and copy everything except the first line which contains
    // the default boot label
    QFile f( originalPath );
    if( f.open( QIODevice::ReadOnly ) ) {

        QTextStream isolinuxConfigOrig( &f );

        if(  d->doc->defaultBootLabel() != i18n("default") ) {
            isolinuxConfigOrig.readLine(); // skip first line
            s << "default " << d->doc->defaultBootLabel()  << endl;
        }

        QString line = isolinuxConfigOrig.readLine();
        while( !line.isNull() ) {
            s << line << endl;
            line = isolinuxConfigOrig.readLine();
        }

        d->isolinuxConfigFile->close();
        return true;
    }
    else
        return false;
}


bool K3b::MovixDocPreparer::writeMovixRcFile()
{
    delete d->movixRcFile;
    d->movixRcFile = new KTemporaryFile();
    d->movixRcFile->open();

    QTextStream s( d->movixRcFile );

    if( !d->doc->additionalMPlayerOptions().isEmpty() )
        s << "extra-mplayer-options=" << d->doc->additionalMPlayerOptions() << endl;
    if( !d->doc->unwantedMPlayerOptions().isEmpty() )
        s << "unwanted-mplayer-options=" << d->doc->unwantedMPlayerOptions() << endl;
    s << "loop=" << d->doc->loopPlaylist() << endl;
    if( d->doc->shutdown() )
        s << "shut=y" << endl;
    if( d->doc->reboot() )
        s << "reboot=y" << endl;
    if( d->doc->ejectDisk() )
        s << "eject=y" << endl;
    if( d->doc->randomPlay() )
        s << "random=y" << endl;
    if( d->doc->noDma() )
        s << "dma=n" << endl;

    d->movixRcFile->close();
    return true;
}


bool K3b::MovixDocPreparer::addMovixFiles()
{
    // first of all we create the directories
    d->isolinuxDir = new K3b::DirItem( "isolinux", d->doc, d->doc->root() );
    d->movixDir = new K3b::DirItem( "movix", d->doc, d->doc->root() );
    K3b::DirItem* kernelDir = d->doc->addEmptyDir( "kernel", d->isolinuxDir );

    // add the linux kernel
    (void)new K3b::FileItem( d->eMovixBin->path() + "/isolinux/kernel/vmlinuz", d->doc, kernelDir );

    // add the boot image
    K3b::BootItem* bootItem = d->doc->createBootItem( d->eMovixBin->path() + "/isolinux/isolinux.bin",
                                                    d->isolinuxDir );
    bootItem->setImageType( K3b::BootItem::NONE );
    bootItem->setLoadSize( 4 );
    bootItem->setBootInfoTable(true);

    // some sort weights as defined in isolinux
    d->isolinuxDir->setSortWeight( 100 );
    kernelDir->setSortWeight( 50 );
    bootItem->setSortWeight( 200 );

    // rename the boot catalog file
    d->doc->bootCataloge()->setK3bName( "isolinux.boot" );

    // the following sucks! Redesign it!

    // add all the isolinux files
    QStringList isolinuxFiles = d->eMovixBin->isolinuxFiles();
    isolinuxFiles.removeOne( "isolinux.bin" );
    isolinuxFiles.removeOne( "isolinux.cfg" );
    isolinuxFiles.removeOne( "kernel/vmlinuz" );
    for( QStringList::const_iterator it = isolinuxFiles.constBegin();
         it != isolinuxFiles.constEnd(); ++it ) {
        QString path = d->eMovixBin->path() + "/isolinux/" + *it;
        (void)new K3b::FileItem( path, d->doc, d->isolinuxDir );
    }

    const QStringList& movixFiles = d->eMovixBin->movixFiles();
    for( QStringList::const_iterator it = movixFiles.constBegin();
         it != movixFiles.constEnd(); ++it ) {
        QString path = d->eMovixBin->path() + "/movix/" + *it;
        (void)new K3b::FileItem( path, d->doc, d->movixDir );
    }

    // add doku files
    QString path = d->eMovixBin->languageDir( d->doc->bootMessageLanguage() );
    QDir dir(path);
    QStringList helpFiles = dir.entryList(QDir::Files);
    for( QStringList::const_iterator it = helpFiles.constBegin();
         it != helpFiles.constEnd(); ++it ) {
        // some emovix installations include backup-files, no one's perfect ;)
        if( !(*it).endsWith( "~" ) )
            (void)new K3b::FileItem( path + "/" + *it, d->doc, d->isolinuxDir );
    }


    // add subtitle font dir
    if( !d->doc->subtitleFontset().isEmpty() &&
        d->doc->subtitleFontset() != i18n("none") ) {
        d->mplayerDir = new K3b::DirItem( "mplayer", d->doc, d->doc->root() );

        QString fontPath = d->eMovixBin->subtitleFontDir( d->doc->subtitleFontset() );
        QFileInfo fontType( fontPath );
        if( fontType.isDir() ) {
            K3b::DirItem* fontDir = new K3b::DirItem( "font", d->doc, d->mplayerDir );
            QDir dir( fontPath );
            QStringList fontFiles = dir.entryList( QDir::Files );
            for( QStringList::const_iterator it = fontFiles.constBegin();
                 it != fontFiles.constEnd(); ++it ) {
                (void)new K3b::FileItem( fontPath + "/" + *it, d->doc, fontDir );
            }
        }
        else {
            // just a ttf file
            // needs to be named: subfont.ttf and needs to be placed in mplayer/
            // instead of mplayer/font
            (void)new K3b::FileItem( fontPath,
                                   d->doc,
                                   d->mplayerDir,
                                   "subfont.ttf" );
        }
    }


    // add movix-config-file and boot-config file
    if( writeMovixRcFile() &&
        writeIsolinuxConfigFile( d->eMovixBin->path() + "/isolinux/isolinux.cfg" ) &&
        writePlaylistFile() ) {

        (void)new K3b::FileItem( d->movixRcFile->fileName(), d->doc, d->movixDir, "movixrc" );
        (void)new K3b::FileItem( d->isolinuxConfigFile->fileName(), d->doc, d->isolinuxDir, "isolinux.cfg" );
        d->playlistFileItem = new K3b::FileItem( d->playlistFile->fileName(), d->doc, d->doc->root(), "movix.list" );

        return true;
    }
    else
        return false;
}


bool K3b::MovixDocPreparer::addMovixFilesNew()
{
    // 1. get a list of files from the movixbin
    // 2. create file items (replace isolinux.cfg with the one created above)
    // 3. add movixrc and movix.list files
    // 4. set weights for isolinux files

    // FIXME: use the settings from the doc
    const QStringList files = d->eMovixBin->files( d->doc->keyboardLayout(),
                                             d->doc->subtitleFontset(),
                                             d->doc->audioBackground(),
                                             d->doc->bootMessageLanguage(),
                                             QStringList() << "all" /*d->doc->codecs()*/ ); // for now we simply don't allow selection

    for( QStringList::ConstIterator it = files.constBegin(); it != files.constEnd(); ++it ) {
        QString docPath = (*it).section( ' ', 0, 0 );
        QString filePath = (*it).section( ' ', 1, 1 );
        QString fileName = filePath.section( '/', -1 );

        if( fileName == "isolinux.cfg" ) {
            // replace the local file with our modified one
            if( writeIsolinuxConfigFile( filePath ) )
                createItem( d->isolinuxConfigFile->fileName(), docPath )->setK3bName( "isolinux.cfg" );
            else
                return false;
        }
        else if( fileName == "isolinux.bin" ) {
            // create boot item (no need to remember this since it's in a dir which will be removed
            // anyway)
            K3b::BootItem* bootItem = d->doc->createBootItem( filePath, createDir(docPath) );
            bootItem->setImageType( K3b::BootItem::NONE );
            bootItem->setLoadSize( 4 );
            bootItem->setBootInfoTable(true);

            // set the proper sort weight
            bootItem->setSortWeight( 200 );
            bootItem->parent()->setSortWeight( 100 );
        }
        else if( fileName != "movixrc" ) { // we create our own movixrc
            K3b::FileItem* item = createItem( filePath, docPath );

            // Truetype subtitle fonts needs to be named subfont.ttf
            if( fileName == d->doc->subtitleFontset() + ".ttf" ) {
                item->setK3bName( "subfont.ttf" );
            }
            else if( fileName == "vmlinuz" )
                item->setSortWeight( 50 );
        }
    }

    // Some distributions (such as Gentoo for example) do use the win32codecs package instead of the
    // eMovix supplied codecs. These codecs are not picked up by the movix-conf script
    K3b::DirItem* codecDir = dynamic_cast<K3b::DirItem*>( d->doc->root()->findByPath( "/eMoviX/codecs" ) );
    if( !codecDir || codecDir->isEmpty() ) {
        QDir localCodecDir( d->eMovixBin->movixDataDir() + "/codecs" );
        if( localCodecDir.exists() ) {
            QStringList codecFiles = localCodecDir.entryList( QDir::Files );
            for( QStringList::const_iterator it = codecFiles.constBegin(); it != codecFiles.constEnd(); ++it )
                createItem( localCodecDir.path() + '/' + *it, "/eMoviX/codecs" );
        }
    }

    if( writePlaylistFile() && writeMovixRcFile() ) {
        // add the two items that are not listed by the script
        createItem( d->movixRcFile->fileName(), "/eMoviX/movix" )->setK3bName( "movixrc" );
        createItem( d->playlistFile->fileName(), "/" )->setK3bName( "movix.list" );
        return true;
    }
    else
        return false;
}


K3b::FileItem* K3b::MovixDocPreparer::createItem( const QString& localPath, const QString& docPath )
{
    // make sure the path in the doc exists
    K3b::DirItem* dir = createDir( docPath );

    // create the file in dir
    K3b::FileItem* item = new K3b::FileItem( localPath, d->doc, dir );

    // remember the item to remove it becasue the dir cannot be removed
    if( dir == d->doc->root() )
        d->newMovixItems.push( item );

    return item;
}


K3b::DirItem* K3b::MovixDocPreparer::createDir( const QString& docPath )
{
    QStringList docPathSections = docPath.split( '/', QString::SkipEmptyParts );
    K3b::DirItem* dir = d->doc->root();
    for( QStringList::ConstIterator it = docPathSections.constBegin(); it != docPathSections.constEnd(); ++it ) {
        K3b::DataItem* next = dir->find( *it );
        if( !next )
            dir = new K3b::DirItem( *it, d->doc, dir );
        else if( next->isDir() )
            dir = static_cast<K3b::DirItem*>( next );
        else {
            kError() << "(K3b::MovixDocPreparer) found non-dir item where a dir was needed." << endl;
            return 0;
        }
    }

    // remember the dir to remove it
    if( dir != d->doc->root() ) {
        K3b::DirItem* delDir = dir;
        while( delDir->parent() != d->doc->root() )
            delDir = delDir->parent();
        if( d->newMovixItems.lastIndexOf( delDir ) == -1 )
            d->newMovixItems.push( delDir );
    }

    return dir;
}

#include "k3bmovixdocpreparer.moc"
