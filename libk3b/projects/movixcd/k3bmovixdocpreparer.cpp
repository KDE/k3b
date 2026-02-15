/*
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
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
#include "k3b_i18n.h"

#include <KIO/Global>

#include <QDebug>
#include <QDir>
#include <QStack>
#include <QTemporaryFile>
#include <QTextStream>


class K3b::MovixDocPreparer::Private
{
public:
    Private()
        : doc(nullptr),
          playlistFile(nullptr),
          isolinuxConfigFile(nullptr),
          movixRcFile(nullptr),
          isolinuxDir(nullptr),
          movixDir(nullptr),
          mplayerDir(nullptr),
          playlistFileItem(nullptr),
          structuresCreated(false) {
    }

    K3b::MovixDoc* doc;
    const K3b::MovixBin* eMovixBin;

    QTemporaryFile* playlistFile;
    QTemporaryFile* isolinuxConfigFile;
    QTemporaryFile* movixRcFile;

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
    qDebug();
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
    qDebug();
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
    qDebug();
    // remove movix files from doc
    // the dataitems do the cleanup in the doc
    delete d->movixDir;
    delete d->isolinuxDir;
    delete d->mplayerDir;
    delete d->playlistFileItem;

    d->movixDir = nullptr;
    d->isolinuxDir = nullptr;
    d->mplayerDir = nullptr;
    d->playlistFileItem = nullptr;

    while( !d->newMovixItems.empty() ) {
        delete d->newMovixItems.pop();
    }

    // remove all the temp files
    delete d->playlistFile;
    delete d->isolinuxConfigFile;
    delete d->movixRcFile;

    d->playlistFile = nullptr;
    d->isolinuxConfigFile = nullptr;
    d->movixRcFile = nullptr;

    d->structuresCreated = false;
}


bool K3b::MovixDocPreparer::writePlaylistFile()
{
    delete d->playlistFile;
    d->playlistFile = new QTemporaryFile();
    d->playlistFile->open();

    QTextStream s( d->playlistFile );

    QList<K3b::MovixFileItem*> movixFileItems = d->doc->movixFileItems();

    Q_FOREACH( K3b::MovixFileItem* item, movixFileItems ) {
        s << "/cdrom/";
        s << item->writtenName();
        s << Qt::endl;
    }

    d->playlistFile->close();
    return true;
}


bool K3b::MovixDocPreparer::writeIsolinuxConfigFile( const QString& originalPath )
{
    delete d->isolinuxConfigFile;
    d->isolinuxConfigFile = new QTemporaryFile();
    d->isolinuxConfigFile->open();

    QTextStream s( d->isolinuxConfigFile );

    // now open the default isolinux.cfg and copy everything except the first line which contains
    // the default boot label
    QFile f( originalPath );
    if( f.open( QIODevice::ReadOnly ) ) {

        QTextStream isolinuxConfigOrig( &f );

        if(  d->doc->defaultBootLabel() != i18n("default") ) {
            isolinuxConfigOrig.readLine(); // skip first line
            s << "default " << d->doc->defaultBootLabel()  << Qt::endl;
        }

        QString line = isolinuxConfigOrig.readLine();
        while( !line.isNull() ) {
            s << line << Qt::endl;
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
    d->movixRcFile = new QTemporaryFile();
    d->movixRcFile->open();

    QTextStream s( d->movixRcFile );

    if( !d->doc->additionalMPlayerOptions().isEmpty() )
        s << "extra-mplayer-options=" << d->doc->additionalMPlayerOptions() << Qt::endl;
    if( !d->doc->unwantedMPlayerOptions().isEmpty() )
        s << "unwanted-mplayer-options=" << d->doc->unwantedMPlayerOptions() << Qt::endl;
    s << "loop=" << d->doc->loopPlaylist() << Qt::endl;
    if( d->doc->shutdown() )
        s << "shut=y" << Qt::endl;
    if( d->doc->reboot() )
        s << "reboot=y" << Qt::endl;
    if( d->doc->ejectDisk() )
        s << "eject=y" << Qt::endl;
    if( d->doc->randomPlay() )
        s << "random=y" << Qt::endl;
    if( d->doc->noDma() )
        s << "dma=n" << Qt::endl;

    d->movixRcFile->close();
    return true;
}


bool K3b::MovixDocPreparer::addMovixFiles()
{
    // first of all we create the directories
    d->isolinuxDir = new K3b::DirItem( "isolinux" );
    d->movixDir = new K3b::DirItem( "movix" );
    if (d->doc == nullptr)
        return false;
    d->doc->root()->addDataItem( d->isolinuxDir );
    d->doc->root()->addDataItem( d->movixDir );
    K3b::DirItem* kernelDir = d->doc->addEmptyDir( "kernel", d->isolinuxDir );

    // add the linux kernel
    kernelDir->addDataItem( new K3b::FileItem( d->eMovixBin->path() + "/isolinux/kernel/vmlinuz", *d->doc ) );

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
        d->isolinuxDir->addDataItem( new K3b::FileItem( path, *d->doc ) );
    }

    const QStringList& movixFiles = d->eMovixBin->movixFiles();
    for( QStringList::const_iterator it = movixFiles.constBegin();
         it != movixFiles.constEnd(); ++it ) {
        QString path = d->eMovixBin->path() + "/movix/" + *it;
        d->movixDir->addDataItem( new K3b::FileItem( path, *d->doc ) );
    }

    // add doku files
    QString path = d->eMovixBin->languageDir( d->doc->bootMessageLanguage() );
    QDir dir(path);
    QStringList helpFiles = dir.entryList(QDir::Files);
    for( QStringList::const_iterator it = helpFiles.constBegin();
         it != helpFiles.constEnd(); ++it ) {
        // some emovix installations include backup-files, no one's perfect ;)
        if( !(*it).endsWith( '~' ) )
            d->isolinuxDir->addDataItem( new K3b::FileItem( path + '/' + *it, *d->doc ) );
    }


    // add subtitle font dir
    if( !d->doc->subtitleFontset().isEmpty() &&
        d->doc->subtitleFontset() != i18n("none") ) {
        d->mplayerDir = new K3b::DirItem( "mplayer" );
        d->doc->root()->addDataItem( d->mplayerDir );

        QString fontPath = d->eMovixBin->subtitleFontDir( d->doc->subtitleFontset() );
        QFileInfo fontType( fontPath );
        if( fontType.isDir() ) {
            K3b::DirItem* fontDir = new K3b::DirItem( "font" );
            d->mplayerDir->addDataItem( fontDir );
            QDir dir( fontPath );
            QStringList fontFiles = dir.entryList( QDir::Files );
            for( QStringList::const_iterator it = fontFiles.constBegin();
                 it != fontFiles.constEnd(); ++it ) {
                fontDir->addDataItem( new K3b::FileItem( fontPath + '/' + *it, *d->doc ) );
            }
        }
        else {
            // just a ttf file
            // needs to be named: subfont.ttf and needs to be placed in mplayer/
            // instead of mplayer/font
            d->mplayerDir->addDataItem( new K3b::FileItem( fontPath, *d->doc, "subfont.ttf" ) );
        }
    }


    // add movix-config-file and boot-config file
    if( writeMovixRcFile() &&
        writeIsolinuxConfigFile( d->eMovixBin->path() + "/isolinux/isolinux.cfg" ) &&
        writePlaylistFile() ) {

        d->movixDir->addDataItem( new K3b::FileItem( d->movixRcFile->fileName(), *d->doc, "movixrc" ) );
        d->isolinuxDir->addDataItem( new K3b::FileItem( d->isolinuxConfigFile->fileName(), *d->doc, "isolinux.cfg" ) );
        d->playlistFileItem = new K3b::FileItem( d->playlistFile->fileName(), *d->doc, "movix.list" );
        d->doc->root()->addDataItem( d->playlistFileItem );

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

    if (d->doc == nullptr)
        return false;
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
    K3b::FileItem* item = new K3b::FileItem( localPath, *d->doc );
    dir->addDataItem( item );

    // remember the item to remove it because the dir cannot be removed
    if( dir == d->doc->root() )
        d->newMovixItems.push( item );

    return item;
}


K3b::DirItem* K3b::MovixDocPreparer::createDir( const QString& docPath )
{
    QStringList docPathSections = docPath.split( '/', Qt::SkipEmptyParts );
    K3b::DirItem* dir = d->doc->root();
    for( QStringList::ConstIterator it = docPathSections.constBegin(); it != docPathSections.constEnd(); ++it ) {
        K3b::DataItem* next = dir->find( *it );
        if( !next ) {
            DirItem* newDir = new K3b::DirItem( *it );
            dir->addDataItem( newDir );
            dir = newDir;
        } else if( next->isDir() ) {
            dir = static_cast<K3b::DirItem*>( next );
        } else {
            qCritical() << "(K3b::MovixDocPreparer) found non-dir item where a dir was needed." << Qt::endl;
            return nullptr;
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

#include "moc_k3bmovixdocpreparer.cpp"
