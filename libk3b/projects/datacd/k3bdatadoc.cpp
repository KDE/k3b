/*
    SPDX-FileCopyrightText: 2011 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/


#include "k3bdatadoc.h"
#include "k3bfileitem.h"
#include "k3bdataitem.h"
#include "k3bdiritem.h"
#include "k3bsessionimportitem.h"
#include "k3bdatajob.h"
#include "k3bbootitem.h"
#include "k3bspecialdataitem.h"
#include "k3bfilecompilationsizehandler.h"
#include "k3bmkisofshandler.h"
#include "k3bcore.h"
#include "k3bglobals.h"
#include "k3bmsf.h"
#include "k3biso9660.h"
#include "k3bisooptions.h"
#include "k3bdevicehandler.h"
#include "k3bdevice.h"
#include "k3btoc.h"
#include "k3btrack.h"
#include "k3bmultichoicedialog.h"
#include "k3bvalidators.h"
#include "k3bglobalsettings.h"
#include "k3b_i18n.h"

#include <KConfig>
#include <KMessageBox>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QTimer>
#include <QApplication>
#include <QDomElement>

#include <string.h>
#include <stdlib.h>
#include <ctype.h>


class K3b::DataDoc::Private
{
public:
    Private()
    :
        oldSessionSize( 0 ),
        root( 0 ),
        dataMode( 0 ),
        verifyData( false ),
        importedSession( -1 ),
        bootCataloge( 0 ),
        bExistingItemsReplaceAll( false ),
        bExistingItemsIgnoreAll( false ),
        needToCutFilenames( false )
    {
        sizeHandler = new K3b::FileCompilationSizeHandler();
    }

    ~Private()
    {
        delete root;
        delete sizeHandler;
        //  delete oldSessionSizeHandler;
    }

    FileCompilationSizeHandler* sizeHandler;

    //  FileCompilationSizeHandler* oldSessionSizeHandler;
    KIO::filesize_t oldSessionSize;

    QStringList notFoundFiles;
    QStringList noPermissionFiles;

    RootItem* root;

    int dataMode;

    bool verifyData;

    IsoOptions isoOptions;

    MultiSessionMode multisessionMode;
    QList<DataItem*> oldSession;
    int importedSession;

    // boot cd stuff
    DataItem* bootCataloge;
    QList<BootItem*> bootImages;

    bool bExistingItemsReplaceAll;
    bool bExistingItemsIgnoreAll;

    bool needToCutFilenames;
    QList<DataItem*> needToCutFilenameItems;
};


/**
 * There are two ways to fill a data project with files and folders:
 * \li Use the addUrl and addUrlsT methods
 * \li or create your own K3b::DirItems and K3b::FileItems. The doc will be properly updated
 *     by the constructors of the items.
 */
K3b::DataDoc::DataDoc( QObject* parent )
    : K3b::Doc( parent ),
      d( new Private )
{
}


K3b::DataDoc::~DataDoc()
{
    delete d;
}


bool K3b::DataDoc::newDocument()
{
    clear();
    if ( !d->root )
        d->root = new K3b::RootItem( *this );

    d->bExistingItemsReplaceAll = d->bExistingItemsIgnoreAll = false;

    d->multisessionMode = AUTO;
    d->dataMode = K3b::DataModeAuto;

    d->isoOptions = K3b::IsoOptions();

    return K3b::Doc::newDocument();
}


void K3b::DataDoc::clear()
{
    clearImportedSession();
    d->importedSession = -1;
    d->oldSessionSize = 0;
    d->bootCataloge = 0;
    if( d->root ) {
        while( !d->root->children().isEmpty() )
            removeItem( d->root->children().first() );
    }
    d->sizeHandler->clear();
    emit importedSessionChanged( importedSession() );
}


QString K3b::DataDoc::name() const
{
    return d->isoOptions.volumeID();
}


const K3b::IsoOptions& K3b::DataDoc::isoOptions() const
{
    return d->isoOptions;
}


void K3b::DataDoc::setIsoOptions( const K3b::IsoOptions& isoOptions )
{
    d->isoOptions = isoOptions;
    emit changed();
}


void K3b::DataDoc::setVolumeID( const QString& v )
{
    d->isoOptions.setVolumeID( v );
    emit changed();
    emit volumeIdChanged();
}


void K3b::DataDoc::addUrls( const QList<QUrl>& urls )
{
    addUrlsToDir( urls, root() );
}


void K3b::DataDoc::addUrlsToDir( const QList<QUrl>& l, K3b::DirItem* dir )
{
    if( !dir )
        dir = root();

    QList<QUrl> urls = K3b::convertToLocalUrls(l);

    for( QList<QUrl>::ConstIterator it = urls.constBegin(); it != urls.constEnd(); ++it ) {
        const QUrl& url = *it;
        QFileInfo f( url.toLocalFile() );
        QString k3bname = f.absoluteFilePath().section( '/', -1 );

        // filenames cannot end in backslashes (mkisofs problem. See comments in k3bisoimager.cpp (escapeGraftPoint()))
        while( k3bname[k3bname.length()-1] == '\\' )
            k3bname.truncate( k3bname.length()-1 );

        // backup dummy name
        if( k3bname.isEmpty() )
            k3bname = '1';

        K3b::DirItem* newDirItem = 0;

        // rename the new item if an item with that name already exists
        int cnt = 0;
        bool ok = false;
        while( !ok ) {
            ok = true;
            QString name( k3bname );
            if( cnt > 0 )
                name += QString("_%1").arg(cnt);
            if( K3b::DataItem* oldItem = dir->find( name ) ) {
                if( f.isDir() && oldItem->isDir() ) {
                    // ok, just reuse the dir
                    newDirItem = static_cast<K3b::DirItem*>(oldItem);
                }
                // directories cannot replace files in an old session (I think)
                // and also directories can for sure never be replaced (only be reused as above)
                // so we always rename if the old item is a dir.
                else if( !oldItem->isFromOldSession() ||
                         f.isDir() ||
                         oldItem->isDir() ) {
                    ++cnt;
                    ok = false;
                }
            }
        }
        if( cnt > 0 )
            k3bname += QString("_%1").arg(cnt);

        // QFileInfo::exists and QFileInfo::isReadable return false for broken symlinks :(
        if( f.isDir() && !f.isSymLink() ) {
            if( !newDirItem ) {
                newDirItem = new K3b::DirItem( k3bname );
                newDirItem->setLocalPath( url.toLocalFile() ); // HACK: see k3bdiritem.h
                dir->addDataItem( newDirItem );
            }

            // recursively add all the files in the directory
            QStringList dlist = QDir( f.absoluteFilePath() ).entryList( QDir::AllEntries|QDir::System|QDir::Hidden|QDir::NoDotAndDotDot );
            QList<QUrl> newUrls;
            for( QStringList::ConstIterator it = dlist.constBegin(); it != dlist.constEnd(); ++it )
                newUrls.append( QUrl::fromLocalFile( f.absoluteFilePath() + '/' + *it ) );
            addUrlsToDir( newUrls, newDirItem );
        }
        else if( f.isSymLink() || f.isFile() ) {
            dir->addDataItem( new FileItem( url.toLocalFile(), *this, k3bname ) );
        }
    }

    emit changed();

    setModified( true );
}


bool K3b::DataDoc::nameAlreadyInDir( const QString& name, K3b::DirItem* dir )
{
    if( !dir )
        return false;
    else
        return ( dir->find( name ) != 0 );
}


K3b::DirItem* K3b::DataDoc::addEmptyDir( const QString& name, K3b::DirItem* parent )
{
    if( parent != 0 ) {
        K3b::DirItem* item = new K3b::DirItem( name );
        parent->addDataItem( item );

        setModified( true );

        return item;
    } else {
        return 0;
    }
}


KIO::filesize_t K3b::DataDoc::size() const
{
    if( d->isoOptions.doNotCacheInodes() )
        return root()->blocks().mode1Bytes() + d->oldSessionSize;
    else
        return d->sizeHandler->blocks( d->isoOptions.followSymbolicLinks() ||
                                      !d->isoOptions.createRockRidge() ).mode1Bytes();
}


KIO::filesize_t K3b::DataDoc::burningSize() const
{
    return size() - d->oldSessionSize; //d->oldSessionSizeHandler->size();
}


K3b::Msf K3b::DataDoc::length() const
{
    // 1 block consists of 2048 bytes real data
    // and 1 block equals to 1 audio frame
    // so this is the way to calculate:

    return K3b::Msf( size() / 2048 );
}


K3b::Msf K3b::DataDoc::burningLength() const
{
    return K3b::Msf( burningSize() / 2048 );
}


bool K3b::DataDoc::loadDocumentData( QDomElement* rootElem )
{
    if( !root() )
        newDocument();

    QDomNodeList nodes = rootElem->childNodes();

    if( nodes.item(0).nodeName() != "general" ) {
        qDebug() << "(K3b::DataDoc) could not find 'general' section.";
        return false;
    }
    if( !readGeneralDocumentData( nodes.item(0).toElement() ) )
        return false;


    // parse options
    // -----------------------------------------------------------------
    if( nodes.item(1).nodeName() != "options" ) {
        qDebug() << "(K3b::DataDoc) could not find 'options' section.";
        return false;
    }
    if( !loadDocumentDataOptions( nodes.item(1).toElement() ) )
        return false;
    // -----------------------------------------------------------------



    // parse header
    // -----------------------------------------------------------------
    if( nodes.item(2).nodeName() != "header" ) {
        qDebug() << "(K3b::DataDoc) could not find 'header' section.";
        return false;
    }
    if( !loadDocumentDataHeader( nodes.item(2).toElement() ) )
        return false;
    // -----------------------------------------------------------------



    // parse files
    // -----------------------------------------------------------------
    if( nodes.item(3).nodeName() != "files" ) {
        qDebug() << "(K3b::DataDoc) could not find 'files' section.";
        return false;
    }

    if( d->root == 0 )
        d->root = new K3b::RootItem( *this );

    QDomNodeList filesList = nodes.item(3).childNodes();
    for( int i = 0; i < filesList.count(); i++ ) {

        QDomElement e = filesList.item(i).toElement();
        if( !loadDataItem( e, root() ) )
            return false;
    }

    // -----------------------------------------------------------------

    //
    // Old versions of K3b do not properly save the boot catalog location
    // and name. So to ensure we have one around even if loading an old project
    // file we create a default one here.
    //
    if( !d->bootImages.isEmpty() && !d->bootCataloge )
        createBootCatalogeItem( d->bootImages.first()->parent() );


    informAboutNotFoundFiles();

    return true;
}


bool K3b::DataDoc::loadDocumentDataOptions( QDomElement elem )
{
    QDomNodeList headerList = elem.childNodes();
    for( int i = 0; i < headerList.count(); i++ ) {

        QDomElement e = headerList.item(i).toElement();
        if( e.isNull() )
            return false;

        if( e.nodeName() == "rock_ridge")
            d->isoOptions.setCreateRockRidge( e.attributeNode( "activated" ).value() == "yes" );

        else if( e.nodeName() == "joliet")
            d->isoOptions.setCreateJoliet( e.attributeNode( "activated" ).value() == "yes" );

        else if( e.nodeName() == "udf")
            d->isoOptions.setCreateUdf( e.attributeNode( "activated" ).value() == "yes" );

        else if( e.nodeName() == "joliet_allow_103_characters")
            d->isoOptions.setJolietLong( e.attributeNode( "activated" ).value() == "yes" );

        else if( e.nodeName() == "iso_allow_lowercase")
            d->isoOptions.setISOallowLowercase( e.attributeNode( "activated" ).value() == "yes" );

        else if( e.nodeName() == "iso_allow_period_at_begin")
            d->isoOptions.setISOallowPeriodAtBegin( e.attributeNode( "activated" ).value() == "yes" );

        else if( e.nodeName() == "iso_allow_31_char")
            d->isoOptions.setISOallow31charFilenames( e.attributeNode( "activated" ).value() == "yes" );

        else if( e.nodeName() == "iso_omit_version_numbers")
            d->isoOptions.setISOomitVersionNumbers( e.attributeNode( "activated" ).value() == "yes" );

        else if( e.nodeName() == "iso_omit_trailing_period")
            d->isoOptions.setISOomitTrailingPeriod( e.attributeNode( "activated" ).value() == "yes" );

        else if( e.nodeName() == "iso_max_filename_length")
            d->isoOptions.setISOmaxFilenameLength( e.attributeNode( "activated" ).value() == "yes" );

        else if( e.nodeName() == "iso_relaxed_filenames")
            d->isoOptions.setISOrelaxedFilenames( e.attributeNode( "activated" ).value() == "yes" );

        else if( e.nodeName() == "iso_no_iso_translate")
            d->isoOptions.setISOnoIsoTranslate( e.attributeNode( "activated" ).value() == "yes" );

        else if( e.nodeName() == "iso_allow_multidot")
            d->isoOptions.setISOallowMultiDot( e.attributeNode( "activated" ).value() == "yes" );

        else if( e.nodeName() == "iso_untranslated_filenames")
            d->isoOptions.setISOuntranslatedFilenames( e.attributeNode( "activated" ).value() == "yes" );

        else if( e.nodeName() == "follow_symbolic_links")
            d->isoOptions.setFollowSymbolicLinks( e.attributeNode( "activated" ).value() == "yes" );

        else if( e.nodeName() == "create_trans_tbl")
            d->isoOptions.setCreateTRANS_TBL( e.attributeNode( "activated" ).value() == "yes" );

        else if( e.nodeName() == "hide_trans_tbl")
            d->isoOptions.setHideTRANS_TBL( e.attributeNode( "activated" ).value() == "yes" );

        else if( e.nodeName() == "iso_level")
            d->isoOptions.setISOLevel( e.text().toInt() );

        else if( e.nodeName() == "discard_symlinks")
            d->isoOptions.setDiscardSymlinks( e.attributeNode( "activated" ).value() == "yes" );

        else if( e.nodeName() == "discard_broken_symlinks")
            d->isoOptions.setDiscardBrokenSymlinks( e.attributeNode( "activated" ).value() == "yes" );

        else if( e.nodeName() == "preserve_file_permissions")
            d->isoOptions.setPreserveFilePermissions( e.attributeNode( "activated" ).value() == "yes" );

        else if( e.nodeName() == "do_not_cache_inodes" )
            d->isoOptions.setDoNotCacheInodes( e.attributeNode( "activated" ).value() == "yes" );

        else if( e.nodeName() == "whitespace_treatment" ) {
            if( e.text() == "strip" )
                d->isoOptions.setWhiteSpaceTreatment( K3b::IsoOptions::strip );
            else if( e.text() == "extended" )
                d->isoOptions.setWhiteSpaceTreatment( K3b::IsoOptions::extended );
            else if (e.text() == "replace")
                d->isoOptions.setWhiteSpaceTreatment( K3b::IsoOptions::replace );
            else
                d->isoOptions.setWhiteSpaceTreatment( K3b::IsoOptions::noChange );
        }

        else if( e.nodeName() == "whitespace_replace_string")
            d->isoOptions.setWhiteSpaceTreatmentReplaceString( e.text() );

        else if( e.nodeName() == "data_track_mode" ) {
            if( e.text() == "mode1" )
                d->dataMode = K3b::DataMode1;
            else if( e.text() == "mode2" )
                d->dataMode = K3b::DataMode2;
            else
                d->dataMode = K3b::DataModeAuto;
        }

        else if( e.nodeName() == "multisession" ) {
            QString mode = e.text();
            if( mode == "start" )
                setMultiSessionMode( START );
            else if( mode == "continue" )
                setMultiSessionMode( CONTINUE );
            else if( mode == "finish" )
                setMultiSessionMode( FINISH );
            else if( mode == "none" )
                setMultiSessionMode( NONE );
            else
                setMultiSessionMode( AUTO );
        }

        else if( e.nodeName() == "verify_data" )
            setVerifyData( e.attributeNode( "activated" ).value() == "yes" );

        else
            qDebug() << "(K3b::DataDoc) unknown option entry: " << e.nodeName();
    }

    return true;
}


bool K3b::DataDoc::loadDocumentDataHeader( QDomElement headerElem )
{
    QDomNodeList headerList = headerElem.childNodes();
    for( int i = 0; i < headerList.count(); i++ ) {

        QDomElement e = headerList.item(i).toElement();
        if( e.isNull() )
            return false;

        if( e.nodeName() == "volume_id" )
            d->isoOptions.setVolumeID( e.text() );

        else if( e.nodeName() == "application_id" )
            d->isoOptions.setApplicationID( e.text() );

        else if( e.nodeName() == "publisher" )
            d->isoOptions.setPublisher( e.text() );

        else if( e.nodeName() == "preparer" )
            d->isoOptions.setPreparer( e.text() );

        else if( e.nodeName() == "volume_set_id" )
            d->isoOptions.setVolumeSetId( e.text() );

        else if( e.nodeName() == "volume_set_size" )
            d->isoOptions.setVolumeSetSize( e.text().toInt() );

        else if( e.nodeName() == "volume_set_number" )
            d->isoOptions.setVolumeSetNumber( e.text().toInt() );

        else if( e.nodeName() == "system_id" )
            d->isoOptions.setSystemId( e.text() );

        else
            qDebug() << "(K3b::DataDoc) unknown header entry: " << e.nodeName();
    }

    return true;
}


bool K3b::DataDoc::loadDataItem( QDomElement& elem, K3b::DirItem* parent )
{
    if( !parent )
        return false;

    K3b::DataItem* newItem = 0;

    if( elem.nodeName() == "file" ) {
        QDomElement urlElem = elem.firstChild().toElement();
        if( urlElem.isNull() ) {
            qDebug() << "(K3b::DataDoc) file-element without url!";
            return false;
        }

        QFileInfo f( urlElem.text() );

        // We cannot use exists() here since this always disqualifies broken symlinks
        if( !f.isFile() && !f.isSymLink() )
            d->notFoundFiles.append( urlElem.text() );

        // broken symlinks are not readable according to QFileInfo which is wrong in our case
        else if( f.isFile() && !f.isReadable() )
            d->noPermissionFiles.append( urlElem.text() );

        else if( !elem.attribute( "bootimage" ).isEmpty() ) {
            K3b::BootItem* bootItem = new K3b::BootItem( urlElem.text(),
                                                         *this,
                                                         elem.attributeNode( "name" ).value() );
            parent->addDataItem( bootItem );
            if( elem.attribute( "bootimage" ) == "floppy" )
                bootItem->setImageType( K3b::BootItem::FLOPPY );
            else if( elem.attribute( "bootimage" ) == "harddisk" )
                bootItem->setImageType( K3b::BootItem::HARDDISK );
            else
                bootItem->setImageType( K3b::BootItem::NONE );
            bootItem->setNoBoot( elem.attribute( "no_boot" ) == "yes" );
            bootItem->setBootInfoTable( elem.attribute( "boot_info_table" ) == "yes" );
            bootItem->setLoadSegment( elem.attribute( "load_segment" ).toInt() );
            bootItem->setLoadSize( elem.attribute( "load_size" ).toInt() );

            newItem = bootItem;
        }

        else {
            newItem = new K3b::FileItem( urlElem.text(),
                                         *this,
                                         elem.attributeNode( "name" ).value() );
            parent->addDataItem( newItem );
        }
    }
    else if( elem.nodeName() == "special" ) {
        if( elem.attributeNode( "type" ).value() == "boot cataloge" )
            createBootCatalogeItem( parent )->setK3bName( elem.attributeNode( "name" ).value() );
    }
    else if( elem.nodeName() == "directory" ) {
        // This is for the VideoDVD project which already contains the *_TS folders
        K3b::DirItem* newDirItem = 0;
        if( K3b::DataItem* item = parent->find( elem.attributeNode( "name" ).value() ) ) {
            if( item->isDir() ) {
                newDirItem = static_cast<K3b::DirItem*>(item);
            }
            else {
                qCritical() << "(K3b::DataDoc) INVALID DOCUMENT: item " << item->k3bPath() << " saved twice" << Qt::endl;
                return false;
            }
        }

        if( !newDirItem ) {
            newDirItem = new K3b::DirItem( elem.attributeNode( "name" ).value() );
            parent->addDataItem( newDirItem );
        }
        QDomNodeList childNodes = elem.childNodes();
        for( int i = 0; i < childNodes.count(); i++ ) {

            QDomElement e = childNodes.item(i).toElement();
            if( !loadDataItem( e, newDirItem ) )
                return false;
        }

        newItem = newDirItem;
    }
    else {
        qDebug() << "(K3b::DataDoc) wrong tag in files-section: " << elem.nodeName();
        return false;
    }

    // load the sort weight
    if( newItem )
        newItem->setSortWeight( elem.attribute( "sort_weight", "0" ).toInt() );

    return true;
}


bool K3b::DataDoc::saveDocumentData( QDomElement* docElem )
{
    QDomDocument doc = docElem->ownerDocument();

    saveGeneralDocumentData( docElem );

    // all options
    // ----------------------------------------------------------------------
    QDomElement optionsElem = doc.createElement( "options" );
    saveDocumentDataOptions( optionsElem );
    docElem->appendChild( optionsElem );
    // ----------------------------------------------------------------------

    // the header stuff
    // ----------------------------------------------------------------------
    QDomElement headerElem = doc.createElement( "header" );
    saveDocumentDataHeader( headerElem );
    docElem->appendChild( headerElem );


    // now do the "real" work: save the entries
    // ----------------------------------------------------------------------
    QDomElement topElem = doc.createElement( "files" );

    Q_FOREACH( K3b::DataItem* item, root()->children() ) {
        saveDataItem( item, &doc, &topElem );
    }

    docElem->appendChild( topElem );
    // ----------------------------------------------------------------------

    return true;
}


void K3b::DataDoc::saveDocumentDataOptions( QDomElement& optionsElem )
{
    QDomDocument doc = optionsElem.ownerDocument();

    QDomElement topElem = doc.createElement( "rock_ridge" );
    topElem.setAttribute( "activated", isoOptions().createRockRidge() ? "yes" : "no" );
    optionsElem.appendChild( topElem );

    topElem = doc.createElement( "joliet" );
    topElem.setAttribute( "activated", isoOptions().createJoliet() ? "yes" : "no" );
    optionsElem.appendChild( topElem );

    topElem = doc.createElement( "udf" );
    topElem.setAttribute( "activated", isoOptions().createUdf() ? "yes" : "no" );
    optionsElem.appendChild( topElem );

    topElem = doc.createElement( "joliet_allow_103_characters" );
    topElem.setAttribute( "activated", isoOptions().jolietLong() ? "yes" : "no" );
    optionsElem.appendChild( topElem );

    topElem = doc.createElement( "iso_allow_lowercase" );
    topElem.setAttribute( "activated", isoOptions().ISOallowLowercase() ? "yes" : "no" );
    optionsElem.appendChild( topElem );

    topElem = doc.createElement( "iso_allow_period_at_begin" );
    topElem.setAttribute( "activated", isoOptions().ISOallowPeriodAtBegin() ? "yes" : "no" );
    optionsElem.appendChild( topElem );

    topElem = doc.createElement( "iso_allow_31_char" );
    topElem.setAttribute( "activated", isoOptions().ISOallow31charFilenames() ? "yes" : "no" );
    optionsElem.appendChild( topElem );

    topElem = doc.createElement( "iso_omit_version_numbers" );
    topElem.setAttribute( "activated", isoOptions().ISOomitVersionNumbers() ? "yes" : "no" );
    optionsElem.appendChild( topElem );

    topElem = doc.createElement( "iso_omit_trailing_period" );
    topElem.setAttribute( "activated", isoOptions().ISOomitTrailingPeriod() ? "yes" : "no" );
    optionsElem.appendChild( topElem );

    topElem = doc.createElement( "iso_max_filename_length" );
    topElem.setAttribute( "activated", isoOptions().ISOmaxFilenameLength() ? "yes" : "no" );
    optionsElem.appendChild( topElem );

    topElem = doc.createElement( "iso_relaxed_filenames" );
    topElem.setAttribute( "activated", isoOptions().ISOrelaxedFilenames() ? "yes" : "no" );
    optionsElem.appendChild( topElem );

    topElem = doc.createElement( "iso_no_iso_translate" );
    topElem.setAttribute( "activated", isoOptions().ISOnoIsoTranslate() ? "yes" : "no" );
    optionsElem.appendChild( topElem );

    topElem = doc.createElement( "iso_allow_multidot" );
    topElem.setAttribute( "activated", isoOptions().ISOallowMultiDot() ? "yes" : "no" );
    optionsElem.appendChild( topElem );

    topElem = doc.createElement( "iso_untranslated_filenames" );
    topElem.setAttribute( "activated", isoOptions().ISOuntranslatedFilenames() ? "yes" : "no" );
    optionsElem.appendChild( topElem );

    topElem = doc.createElement( "follow_symbolic_links" );
    topElem.setAttribute( "activated", isoOptions().followSymbolicLinks() ? "yes" : "no" );
    optionsElem.appendChild( topElem );

    topElem = doc.createElement( "create_trans_tbl" );
    topElem.setAttribute( "activated", isoOptions().createTRANS_TBL() ? "yes" : "no" );
    optionsElem.appendChild( topElem );

    topElem = doc.createElement( "hide_trans_tbl" );
    topElem.setAttribute( "activated", isoOptions().hideTRANS_TBL() ? "yes" : "no" );
    optionsElem.appendChild( topElem );

    topElem = doc.createElement( "iso_level" );
    topElem.appendChild( doc.createTextNode( QString::number(isoOptions().ISOLevel()) ) );
    optionsElem.appendChild( topElem );

    topElem = doc.createElement( "discard_symlinks" );
    topElem.setAttribute( "activated", isoOptions().discardSymlinks() ? "yes" : "no" );
    optionsElem.appendChild( topElem );

    topElem = doc.createElement( "discard_broken_symlinks" );
    topElem.setAttribute( "activated", isoOptions().discardBrokenSymlinks() ? "yes" : "no" );
    optionsElem.appendChild( topElem );

    topElem = doc.createElement( "preserve_file_permissions" );
    topElem.setAttribute( "activated", isoOptions().preserveFilePermissions() ? "yes" : "no" );
    optionsElem.appendChild( topElem );

    topElem = doc.createElement( "do_not_cache_inodes" );
    topElem.setAttribute( "activated", isoOptions().doNotCacheInodes() ? "yes" : "no" );
    optionsElem.appendChild( topElem );


    topElem = doc.createElement( "whitespace_treatment" );
    switch( isoOptions().whiteSpaceTreatment() ) {
    case K3b::IsoOptions::strip:
        topElem.appendChild( doc.createTextNode( "strip" ) );
        break;
    case K3b::IsoOptions::extended:
        topElem.appendChild( doc.createTextNode( "extended" ) );
        break;
    case K3b::IsoOptions::replace:
        topElem.appendChild( doc.createTextNode( "replace" ) );
        break;
    default:
        topElem.appendChild( doc.createTextNode( "noChange" ) );
        break;
    }
    optionsElem.appendChild( topElem );

    topElem = doc.createElement( "whitespace_replace_string" );
    topElem.appendChild( doc.createTextNode( isoOptions().whiteSpaceTreatmentReplaceString() ) );
    optionsElem.appendChild( topElem );

    topElem = doc.createElement( "data_track_mode" );
    if( d->dataMode == K3b::DataMode1 )
        topElem.appendChild( doc.createTextNode( "mode1" ) );
    else if( d->dataMode == K3b::DataMode2 )
        topElem.appendChild( doc.createTextNode( "mode2" ) );
    else
        topElem.appendChild( doc.createTextNode( "auto" ) );
    optionsElem.appendChild( topElem );


    // save multisession
    topElem = doc.createElement( "multisession" );
    switch( d->multisessionMode ) {
    case START:
        topElem.appendChild( doc.createTextNode( "start" ) );
        break;
    case CONTINUE:
        topElem.appendChild( doc.createTextNode( "continue" ) );
        break;
    case FINISH:
        topElem.appendChild( doc.createTextNode( "finish" ) );
        break;
    case NONE:
        topElem.appendChild( doc.createTextNode( "none" ) );
        break;
    default:
        topElem.appendChild( doc.createTextNode( "auto" ) );
        break;
    }
    optionsElem.appendChild( topElem );

    topElem = doc.createElement( "verify_data" );
    topElem.setAttribute( "activated", verifyData() ? "yes" : "no" );
    optionsElem.appendChild( topElem );
    // ----------------------------------------------------------------------
}


void K3b::DataDoc::saveDocumentDataHeader( QDomElement& headerElem )
{
    QDomDocument doc = headerElem.ownerDocument();

    QDomElement topElem = doc.createElement( "volume_id" );
    topElem.appendChild( doc.createTextNode( isoOptions().volumeID() ) );
    headerElem.appendChild( topElem );

    topElem = doc.createElement( "volume_set_id" );
    topElem.appendChild( doc.createTextNode( isoOptions().volumeSetId() ) );
    headerElem.appendChild( topElem );

    topElem = doc.createElement( "volume_set_size" );
    topElem.appendChild( doc.createTextNode( QString::number(isoOptions().volumeSetSize()) ) );
    headerElem.appendChild( topElem );

    topElem = doc.createElement( "volume_set_number" );
    topElem.appendChild( doc.createTextNode( QString::number(isoOptions().volumeSetNumber()) ) );
    headerElem.appendChild( topElem );

    topElem = doc.createElement( "system_id" );
    topElem.appendChild( doc.createTextNode( isoOptions().systemId() ) );
    headerElem.appendChild( topElem );

    topElem = doc.createElement( "application_id" );
    topElem.appendChild( doc.createTextNode( isoOptions().applicationID() ) );
    headerElem.appendChild( topElem );

    topElem = doc.createElement( "publisher" );
    topElem.appendChild( doc.createTextNode( isoOptions().publisher() ) );
    headerElem.appendChild( topElem );

    topElem = doc.createElement( "preparer" );
    topElem.appendChild( doc.createTextNode( isoOptions().preparer() ) );
    headerElem.appendChild( topElem );
    // ----------------------------------------------------------------------
}


void K3b::DataDoc::saveDataItem( K3b::DataItem* item, QDomDocument* doc, QDomElement* parent )
{
    if( K3b::FileItem* fileItem = dynamic_cast<K3b::FileItem*>( item ) ) {
        if( d->oldSession.contains( fileItem ) ) {
            qDebug() << "(K3b::DataDoc) ignoring fileitem " << fileItem->k3bName() << " from old session while saving...";
        }
        else {
            QDomElement topElem = doc->createElement( "file" );
            topElem.setAttribute( "name", fileItem->k3bName() );
            QDomElement subElem = doc->createElement( "url" );
            subElem.appendChild( doc->createTextNode( fileItem->localPath() ) );
            topElem.appendChild( subElem );

            if( item->sortWeight() != 0 )
                topElem.setAttribute( "sort_weight", QString::number(item->sortWeight()) );

            parent->appendChild( topElem );

            // add boot options as attributes to preserve compatibility to older K3b versions
            if( K3b::BootItem* bootItem = dynamic_cast<K3b::BootItem*>( fileItem ) ) {
                if( bootItem->imageType() == K3b::BootItem::FLOPPY )
                    topElem.setAttribute( "bootimage", "floppy" );
                else if( bootItem->imageType() == K3b::BootItem::HARDDISK )
                    topElem.setAttribute( "bootimage", "harddisk" );
                else
                    topElem.setAttribute( "bootimage", "none" );

                topElem.setAttribute( "no_boot", bootItem->noBoot() ? "yes" : "no" );
                topElem.setAttribute( "boot_info_table", bootItem->bootInfoTable() ? "yes" : "no" );
                topElem.setAttribute( "load_segment", QString::number( bootItem->loadSegment() ) );
                topElem.setAttribute( "load_size", QString::number( bootItem->loadSize() ) );
            }
        }
    }
    else if( item == d->bootCataloge ) {
        QDomElement topElem = doc->createElement( "special" );
        topElem.setAttribute( "name", d->bootCataloge->k3bName() );
        topElem.setAttribute( "type", "boot cataloge" );

        parent->appendChild( topElem );
    }
    else if( K3b::DirItem* dirItem = dynamic_cast<K3b::DirItem*>( item ) ) {
        QDomElement topElem = doc->createElement( "directory" );
        topElem.setAttribute( "name", dirItem->k3bName() );

        if( item->sortWeight() != 0 )
            topElem.setAttribute( "sort_weight", QString::number(item->sortWeight()) );

        Q_FOREACH( K3b::DataItem* item, dirItem->children() ) {
            saveDataItem( item, doc, &topElem );
        }

        parent->appendChild( topElem );
    }
}


void K3b::DataDoc::removeItem( K3b::DataItem* item )
{
    if( !item )
        return;

    if( item->isRemoveable() ) {
        delete item;
    }
    else
        qDebug() << "(K3b::DataDoc) tried to remove non-removable entry!";
}


void K3b::DataDoc::removeItems( K3b::DirItem* parent, int start, int count )
{
    if( parent ) {
        parent->removeDataItems( start, count );
    }
}


void K3b::DataDoc::beginInsertItems( DirItem* parent, int start, int end )
{
    emit itemsAboutToBeInserted( parent, start, end );
}


void K3b::DataDoc::endInsertItems( DirItem* parent, int start, int end )
{
    for( int i = start; i <= end; ++i ) {
        DataItem* item = parent->children().at( i );
        // update the project size
        if( !item->isFromOldSession() )
            d->sizeHandler->addFile( item );

        // update the boot item list
        if( item->isBootItem() )
            d->bootImages.append( static_cast<K3b::BootItem*>( item ) );
    }

    emit itemsInserted( parent, start, end );
    emit changed();
}


void K3b::DataDoc::beginRemoveItems( DirItem* parent, int start, int end )
{
    emit itemsAboutToBeRemoved( parent, start, end );

    for( int i = start; i <= end; ++i ) {
        DataItem* item = parent->children().at( i );
        // update the project size
        if( !item->isFromOldSession() )
            d->sizeHandler->removeFile( item );

        // update the boot item list
        if( item->isBootItem() ) {
            d->bootImages.removeAll( static_cast<K3b::BootItem*>( item ) );
            if( d->bootImages.isEmpty() ) {
                delete d->bootCataloge;
                d->bootCataloge = 0;
            }
        }
    }
}


void K3b::DataDoc::endRemoveItems( DirItem* parent, int start, int end )
{
    emit itemsRemoved( parent, start, end );
    emit changed();
}


void K3b::DataDoc::moveItem( K3b::DataItem* item, K3b::DirItem* newParent )
{
    if( !item || !newParent ) {
        qDebug() << "(K3b::DataDoc) item or parentitem was NULL while moving.";
        return;
    }

    if( !item->isMoveable() ) {
        qDebug() << "(K3b::DataDoc) item is not movable! ";
        return;
    }

    item->reparent( newParent );
}


void K3b::DataDoc::moveItems( const QList<K3b::DataItem*>& itemList, K3b::DirItem* newParent )
{
    if( !newParent ) {
        qDebug() << "(K3b::DataDoc) tried to move items to nowhere...!";
        return;
    }

    Q_FOREACH( K3b::DataItem* item, itemList ) {
        // check if newParent is subdir of item
        if( K3b::DirItem* dirItem = dynamic_cast<K3b::DirItem*>( item ) ) {
            if( dirItem->isSubItem( newParent ) ) {
                continue;
            }
        }

        if( item->isMoveable() )
            item->reparent( newParent );
    }
}


K3b::BurnJob* K3b::DataDoc::newBurnJob( K3b::JobHandler* hdl, QObject* parent )
{
    return new K3b::DataJob( this, hdl, parent );
}


QString K3b::DataDoc::treatWhitespace( const QString& path )
{

    // TODO:
    // It could happen that two files with different names
    // will have the same name after the treatment
    // Perhaps we should add a number at the end or something
    // similar (s.a.)


    if( isoOptions().whiteSpaceTreatment() != K3b::IsoOptions::noChange ) {
        QString result = path;

        if( isoOptions().whiteSpaceTreatment() == K3b::IsoOptions::replace ) {
            result.replace( ' ', isoOptions().whiteSpaceTreatmentReplaceString() );
        }
        else if( isoOptions().whiteSpaceTreatment() == K3b::IsoOptions::strip ) {
            result.remove( ' ' );
        }
        else if( isoOptions().whiteSpaceTreatment() == K3b::IsoOptions::extended ) {
            result.truncate(0);
            for( int i = 0; i < path.length(); i++ ) {
                if( path[i] == ' ' ) {
                    if( path[i+1] != ' ' )
                        result.append( path[++i].toUpper() );
                }
                else
                    result.append( path[i] );
            }
        }

        qDebug() << "(K3b::DataDoc) converted " << path << " to " << result;
        return result;
    }
    else
        return path;
}


void K3b::DataDoc::prepareFilenames()
{
    d->needToCutFilenames = false;
    d->needToCutFilenameItems.clear();

    //
    // if joliet is used cut the names and rename if necessary
    // 64 characters for standard joliet and 103 characters for long joliet names
    //
    // Rockridge supports the full 255 UNIX chars and in case Rockridge is disabled we leave
    // it to mkisofs for now since handling all the options to alter the ISO9660 standard it just
    // too much.
    //

    K3b::DataItem* item = root();
    int maxlen = ( isoOptions().jolietLong() ? 103 : 64 );
    while( (item = item->nextSibling()) ) {
        item->setWrittenName( treatWhitespace( item->k3bName() ) );

        if( isoOptions().createJoliet() && item->writtenName().length() > maxlen ) {
            d->needToCutFilenames = true;
            item->setWrittenName( K3b::cutFilename( item->writtenName(), maxlen ) );
            d->needToCutFilenameItems.append( item );
        }

        // TODO: check the Joliet charset
    }

    //
    // 3. check if a directory contains items with the same name
    //
    prepareFilenamesInDir( root() );
}


void K3b::DataDoc::prepareFilenamesInDir( K3b::DirItem* dir )
{
    if( !dir )
        return;

    QList<K3b::DataItem*> sortedChildren;
    QList<K3b::DataItem*> children( dir->children() );
    QList<K3b::DataItem*>::const_iterator it = children.constEnd();
    while ( it != children.constBegin() ) {
        --it;
        K3b::DataItem* item = *it;

        if( item->isDir() )
            prepareFilenamesInDir( dynamic_cast<K3b::DirItem*>( item ) );

        // insertion sort
        int i = 0;
        while( i < sortedChildren.count() && item->writtenName() > sortedChildren.at(i)->writtenName() )
            ++i;

        sortedChildren.insert( i, item );
    }

    if( isoOptions().createJoliet() || isoOptions().createRockRidge() ) {
        QList<K3b::DataItem*> sameNameList;
        while( !sortedChildren.isEmpty() ) {

            sameNameList.clear();

            do {
                sameNameList.append( sortedChildren.takeFirst() );
            } while( !sortedChildren.isEmpty() &&
                     sortedChildren.first()->writtenName() == sameNameList.first()->writtenName() );

            if( sameNameList.count() > 1 ) {
                // now we need to rename the items
                unsigned int maxlen = 255;
                if( isoOptions().createJoliet() ) {
                    if( isoOptions().jolietLong() )
                        maxlen = 103;
                    else
                        maxlen = 64;
                }

                int cnt = 1;
                Q_FOREACH( K3b::DataItem* item, sameNameList ) {
                    item->setWrittenName( K3b::appendNumberToFilename( item->writtenName(), cnt++, maxlen ) );
                }
            }
        }
    }
}


bool K3b::DataDoc::needToCutFilenames() const
{
    return d->needToCutFilenames;
}


QList<K3b::DataItem*> K3b::DataDoc::needToCutFilenameItems() const
{
    return d->needToCutFilenameItems;
}


void K3b::DataDoc::informAboutNotFoundFiles()
{
    if( !d->notFoundFiles.isEmpty() ) {
        KMessageBox::informationList( qApp->activeWindow(), i18n("Could not find the following files:"),
                                      d->notFoundFiles, i18n("Not Found") );
        d->notFoundFiles.clear();
    }

    if( !d->noPermissionFiles.isEmpty() ) {
        KMessageBox::informationList( qApp->activeWindow(), i18n("No permission to read the following files:"),
                                      d->noPermissionFiles, i18n("No Read Permission") );

        d->noPermissionFiles.clear();
    }
}


K3b::DataDoc::MultiSessionMode K3b::DataDoc::multiSessionMode() const
{
    return d->multisessionMode;
}


void K3b::DataDoc::setMultiSessionMode( K3b::DataDoc::MultiSessionMode mode )
{
    if( d->multisessionMode == NONE || d->multisessionMode == START )
        clearImportedSession();

    d->multisessionMode = mode;
}


int K3b::DataDoc::dataMode() const
{
    return d->dataMode;
}


void K3b::DataDoc::setDataMode( int m )
{
    d->dataMode = m;
}


void K3b::DataDoc::setVerifyData( bool b )
{
    d->verifyData = b;
}


bool K3b::DataDoc::verifyData() const
{
    return d->verifyData;
}


bool K3b::DataDoc::importSession( K3b::Device::Device* device, int session )
{
    K3b::Device::DiskInfo diskInfo = device->diskInfo();
    // DVD+RW media is reported as non-appendable
    if( !diskInfo.appendable() &&
        !(diskInfo.mediaType() & (K3b::Device::MEDIA_DVD_PLUS_RW|K3b::Device::MEDIA_DVD_RW_OVWR)) )
        return false;

    K3b::Device::Toc toc = device->readToc();
    if( toc.isEmpty() ||
        toc.last().type() != K3b::Device::Track::TYPE_DATA )
        return false;

    long startSec = toc.last().firstSector().lba();
    if ( session > 0 ) {
        for ( K3b::Device::Toc::const_iterator it = toc.constBegin(); it != toc.constEnd(); ++it ) {
            if ( ( *it ).session() == session ) {
                startSec = ( *it ).firstSector().lba();
                break;
            }
        }
    }
    K3b::Iso9660 iso( device, startSec );

    if( iso.open() ) {
        // remove previously imported sessions
        clearImportedSession();

        // set multisession option
        if( d->multisessionMode != FINISH && d->multisessionMode != AUTO )
            d->multisessionMode = CONTINUE;

        // since in iso9660 it is possible that two files share it's data
        // simply summing the file sizes could result in wrong values
        // that's why we use the size from the toc. This is more accurate
        // anyway since there might be files overwritten or removed
        d->oldSessionSize = toc.last().lastSector().mode1Bytes();
        d->importedSession = session;

        qDebug() << "(K3b::DataDoc) imported session size: " << KIO::convertSize(d->oldSessionSize);

        // the track size for DVD+RW media and DVD-RW Overwrite media has nothing to do with the filesystem
        // size. in that case we need to use the filesystem's size (which is ok since it's one track anyway,
        // no real multisession)
        if( diskInfo.mediaType() & (K3b::Device::MEDIA_DVD_PLUS_RW|K3b::Device::MEDIA_DVD_RW_OVWR) ) {
            d->oldSessionSize = iso.primaryDescriptor().volumeSpaceSize
                               * iso.primaryDescriptor().logicalBlockSize;
        }

        // import some former settings
        d->isoOptions.setCreateRockRidge( iso.firstRRDirEntry() != 0 );
        d->isoOptions.setCreateJoliet( iso.firstJolietDirEntry() != 0 );
        d->isoOptions.setVolumeID( iso.primaryDescriptor().volumeId );
        // TODO: also import some other pd fields

        const K3b::Iso9660Directory* rootDir = iso.firstRRDirEntry();
        // J�rg Schilling says that it is impossible to import the joliet tree for multisession
//     if( !rootDir )
//       rootDir = iso.firstJolietDirEntry();
        if( !rootDir )
            rootDir = iso.firstIsoDirEntry();

        if( rootDir ) {
            createSessionImportItems( rootDir, root() );
            emit changed();
            emit importedSessionChanged( importedSession() );
            return true;
        }
        else {
            qDebug() << "(K3b::DataDoc::importSession) Could not find primary volume desc.";
            return false;
        }
    }
    else {
        qDebug() << "(K3b::DataDoc) unable to read toc.";
        return false;
    }
}


void K3b::DataDoc::createSessionImportItems( const K3b::Iso9660Directory* importDir, K3b::DirItem* parent )
{
    if( !parent )
        return;

    Q_ASSERT(importDir);
    QStringList entries = importDir->entries();
    entries.removeAll( "." );
    entries.removeAll( ".." );
    for( QStringList::const_iterator it = entries.constBegin();
         it != entries.constEnd(); ++it ) {
        if( const K3b::Iso9660Entry* entry = importDir->entry( *it ) ) {
            K3b::DataItem* oldItem = parent->find( entry->name() );
            if( entry->isDirectory() ) {
                K3b::DirItem* dir = 0;
                if( oldItem && oldItem->isDir() ) {
                    dir = (K3b::DirItem*)oldItem;
                }
                else {
                    // we overwrite without warning!
                    if( oldItem )
                        removeItem( oldItem );
                    dir = new K3b::DirItem( entry->name() );
                    parent->addDataItem( dir );
                }

                dir->setRemoveable(false);
                dir->setRenameable(false);
                dir->setMoveable(false);
                dir->setHideable(false);
                dir->setWriteToCd(false);
                dir->setExtraInfo( i18n("From previous session") );
                d->oldSession.append( dir );

                createSessionImportItems( static_cast<const K3b::Iso9660Directory*>(entry), dir );
            }
            else {
                const K3b::Iso9660File* file = static_cast<const K3b::Iso9660File*>(entry);

                // we overwrite without warning!
                if( oldItem )
                    removeItem( oldItem );

                K3b::SessionImportItem* item = new K3b::SessionImportItem( file );
                item->setExtraInfo( i18n("From previous session") );
                parent->addDataItem( item );
                d->oldSession.append( item );
            }
        }
    }
}


void K3b::DataDoc::clearImportedSession()
{
    //  d->oldSessionSizeHandler->clear();
    d->importedSession = -1;
    d->oldSessionSize = 0;

    while( !d->oldSession.isEmpty() ) {
        K3b::DataItem* item = d->oldSession.takeFirst();

        if( item->isDir() ) {
            K3b::DirItem* dir = (K3b::DirItem*)item;
            if( dir->numDirs() + dir->numFiles() == 0 ) {
                // this imported dir is not needed anymore
                // since it is empty
                delete item;
            }
            else {
                Q_FOREACH( K3b::DataItem* item, dir->children() ) {
                    if( !d->oldSession.contains( item ) ) {
                        // now the dir becomes a totally normal dir
                        dir->setRemoveable(true);
                        dir->setRenameable(true);
                        dir->setMoveable(true);
                        dir->setHideable(true);
                        dir->setWriteToCd(true);
                        dir->setExtraInfo( "" );
                        break;
                    }
                }
            }
        }
        else {
            delete item;
        }
    }

    d->multisessionMode = AUTO;

    emit changed();
    emit importedSessionChanged( importedSession() );
}

QList<K3b::BootItem*> K3b::DataDoc::bootImages()
{
    return d->bootImages;
}


K3b::DataItem* K3b::DataDoc::bootCataloge()
{
    return d->bootCataloge;
}


K3b::DirItem* K3b::DataDoc::bootImageDir()
{
    K3b::DataItem* b = d->root->find( "boot" );
    if( !b ) {
        b = new K3b::DirItem( "boot" );
        d->root->addDataItem( b );
        setModified( true );
    }

    // if we cannot create the dir because there is a file named boot just use the root dir
    if( !b->isDir() )
        return d->root;
    else
        return static_cast<K3b::DirItem*>(b);
}


K3b::BootItem* K3b::DataDoc::createBootItem( const QString& filename, K3b::DirItem* dir )
{
    if( !dir )
        dir = bootImageDir();

    K3b::BootItem* boot = new K3b::BootItem( filename, *this );
    dir->addDataItem( boot );

    if( !d->bootCataloge )
        createBootCatalogeItem(dir);

    return boot;
}


K3b::DataItem* K3b::DataDoc::createBootCatalogeItem( K3b::DirItem* dir )
{
    if( !d->bootCataloge ) {
        QString newName = "boot.catalog";
        int i = 0;
        while( dir->alreadyInDirectory( "boot.catalog" ) ) {
            ++i;
            newName = QString( "boot%1.catalog" ).arg(i);
        }

        K3b::SpecialDataItem* b = new K3b::SpecialDataItem( 0, newName );
        dir->addDataItem( b );
        d->bootCataloge = b;
        d->bootCataloge->setRemoveable(false);
        d->bootCataloge->setHideable(false);
        d->bootCataloge->setWriteToCd(false);
        d->bootCataloge->setExtraInfo( i18n("El Torito boot catalog file") );
        b->setSpecialType( i18n("Boot catalog") );
    }
    else
        d->bootCataloge->reparent( dir );

    return d->bootCataloge;
}


QList<K3b::DataItem*> K3b::DataDoc::findItemByLocalPath( const QString& path ) const
{
    Q_UNUSED( path );
    return QList<K3b::DataItem*>();
}


int K3b::DataDoc::importedSession() const
{
    return ( d->oldSession.isEmpty() ? -1 : d->importedSession );
}


K3b::Device::MediaTypes K3b::DataDoc::supportedMediaTypes() const
{
    return Device::MEDIA_WRITABLE;
}


K3b::RootItem* K3b::DataDoc::root() const
{
    return d->root;
}


