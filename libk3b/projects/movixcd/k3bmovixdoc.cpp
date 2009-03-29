/*
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *           (C) 2009 Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
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


#include "k3bmovixdoc.h"
#include "k3bmovixjob.h"
#include "k3bmovixfileitem.h"

#include <k3bdiritem.h>
#include <k3bfileitem.h>
#include <k3bglobals.h>

#include <klocale.h>
#include <kdebug.h>
#include <kurl.h>
#include <kinputdialog.h>
#include <kmessagebox.h>
#include <kapplication.h>

#include <qdom.h>
#include <qfileinfo.h>
#include <kglobal.h>


K3b::MovixDoc::MovixDoc( QObject* parent )
    : K3b::DataDoc( parent )
{
}


K3b::MovixDoc::~MovixDoc()
{
}


K3b::BurnJob* K3b::MovixDoc::newBurnJob( K3b::JobHandler* hdl, QObject* parent )
{
    return new K3b::MovixJob( this, hdl, parent );
}


QString K3b::MovixDoc::typeString() const
{
    return QString::fromLatin1("movix");
}


bool K3b::MovixDoc::newDocument()
{
    m_loopPlaylist = 1;
    m_ejectDisk = false;
    m_reboot = false;
    m_shutdown = false;
    m_randomPlay = false;

    return K3b::DataDoc::newDocument();
}


int K3b::MovixDoc::indexOf( K3b::MovixFileItem* item )
{
    return m_movixFiles.lastIndexOf(item)+1;
}


void K3b::MovixDoc::addUrls( const KUrl::List& urls )
{
    addUrlsAt(urls, -1);
}

void K3b::MovixDoc::addUrlsAt( const KUrl::List& urls, int pos )
{
    QList<K3b::MovixFileItem*> items;

    for( KUrl::List::ConstIterator it = urls.begin(); it != urls.end(); ++it ) {
        KUrl url = K3b::convertToLocalUrl( *it );

        QFileInfo f( url.toLocalFile() );
        if( !f.isFile() || !url.isLocalFile() )
            continue;

        QString newName = f.fileName();
        if( nameAlreadyInDir( newName, root() ) )
        {
            bool ok = true;
            do
            {
                newName = KInputDialog::getText( i18n("Enter New Filename"),
                                                 i18n("A file with that name already exists. Please enter a new name:"),
                                                 newName, &ok, 0 );
            } while( ok && nameAlreadyInDir( newName, root() ) );

            if( !ok )
                continue;
        }

        items.append(new MovixFileItem( f.absoluteFilePath(), this, root(), newName ));
    }

    addMovixItems( items, pos );
}


void K3b::MovixDoc::addMovixItems( QList<K3b::MovixFileItem*>& items, int pos )
{
    if( pos < 0 || pos > (int)m_movixFiles.count() )
        pos = m_movixFiles.count();

    emit aboutToAddMovixItems( pos, items.count());

    foreach (K3b::MovixFileItem* newItem, items)
    {
        m_movixFiles.insert( pos, newItem );
        pos++;
    }

    emit addedMovixItems();
}

void K3b::MovixDoc::removeMovixItem( K3b::MovixFileItem* item)
{
    while( m_movixFiles.contains( item ) )
    {
        int removedPos = m_movixFiles.lastIndexOf( item );

        emit aboutToRemoveMovixItems( removedPos, 1);

        K3b::MovixFileItem *removedItem = m_movixFiles.takeAt( removedPos );
        delete removedItem;

        emit removedMovixItems();
    }
}


void K3b::MovixDoc::moveMovixItem( K3b::MovixFileItem* item, K3b::MovixFileItem* itemAfter )
{
    if( item == itemAfter )
        return;

    // take the current item
    int removedPos = m_movixFiles.lastIndexOf( item );

    emit aboutToRemoveMovixItems( removedPos, 1);

    item = m_movixFiles.takeAt( removedPos );

    emit removedMovixItems();

    // if after == 0 lastIndexOf returnes -1
    int pos = m_movixFiles.lastIndexOf( itemAfter ) + 1;

    emit aboutToAddMovixItems( pos, 1 );

    m_movixFiles.insert( pos, item );

    emit addedMovixItems();

    setModified(true);
}


void K3b::MovixDoc::addSubTitleItem( K3b::MovixFileItem* item, const KUrl& url )
{
    QFileInfo f( url.toLocalFile() );
    if( !f.isFile() || !url.isLocalFile() )
        return;

    if( item->subTitleItem() )
        removeSubTitleItem( item );

    // check if there already is a file named like we want to name the subTitle file
    QString name = K3b::MovixFileItem::subTitleFileName( item->k3bName() );

    if( nameAlreadyInDir( name, root() ) ) {
        KMessageBox::error( 0, i18n("Could not rename subtitle file. File with requested name %1 already exists.",name) );
        return;
    }

    K3b::MovixSubtitleItem* subItem = new K3b::MovixSubtitleItem( f.absoluteFilePath(), this, root(), item, name );
    item->setSubTitleItem( subItem );

    emit subTitleItemAdded( item );

    setModified(true);
}


void K3b::MovixDoc::removeSubTitleItem( K3b::MovixFileItem* item )
{
    if( item->subTitleItem() ) {
        emit subTitleItemRemoved( item );

        delete item->subTitleItem();
        item->setSubTitleItem(0);

        setModified(true);
    }
}


bool K3b::MovixDoc::loadDocumentData( QDomElement* rootElem )
{
    if( !root() )
        newDocument();

    QDomNodeList nodes = rootElem->childNodes();

    if( nodes.item(0).nodeName() != "general" ) {
        kDebug() << "(K3b::MovixDoc) could not find 'general' section.";
        return false;
    }
    if( !readGeneralDocumentData( nodes.item(0).toElement() ) )
        return false;


    // parse options
    // -----------------------------------------------------------------
    if( nodes.item(1).nodeName() != "data_options" ) {
        kDebug() << "(K3b::MovixDoc) could not find 'data_options' section.";
        return false;
    }
    if( !loadDocumentDataOptions( nodes.item(1).toElement() ) )
        return false;
    // -----------------------------------------------------------------



    // parse header
    // -----------------------------------------------------------------
    if( nodes.item(2).nodeName() != "data_header" ) {
        kDebug() << "(K3b::MovixDoc) could not find 'data_header' section.";
        return false;
    }
    if( !loadDocumentDataHeader( nodes.item(2).toElement() ) )
        return false;
    // -----------------------------------------------------------------



    // parse movix options
    // -----------------------------------------------------------------
    if( nodes.item(3).nodeName() != "movix_options" ) {
        kDebug() << "(K3b::MovixDoc) could not find 'movix_options' section.";
        return false;
    }

    // load the options
    QDomNodeList optionList = nodes.item(3).childNodes();
    for( int i = 0; i < optionList.count(); i++ ) {

        QDomElement e = optionList.item(i).toElement();
        if( e.isNull() )
            return false;

        if( e.nodeName() == "shutdown")
            setShutdown( e.attributeNode( "activated" ).value() == "yes" );
        else if( e.nodeName() == "reboot")
            setReboot( e.attributeNode( "activated" ).value() == "yes" );
        else if( e.nodeName() == "eject_disk")
            setEjectDisk( e.attributeNode( "activated" ).value() == "yes" );
        else if( e.nodeName() == "random_play")
            setRandomPlay( e.attributeNode( "activated" ).value() == "yes" );
        else if( e.nodeName() == "no_dma")
            setNoDma( e.attributeNode( "activated" ).value() == "yes" );
        else if( e.nodeName() == "subtitle_fontset")
            setSubtitleFontset( e.text() );
        else if( e.nodeName() == "boot_message_language")
            setBootMessageLanguage( e.text() );
        else if( e.nodeName() == "audio_background")
            setAudioBackground( e.text() );
        else if( e.nodeName() == "keyboard_language")
            setKeyboardLayout( e.text() );
        else if( e.nodeName() == "codecs")
            setCodecs( e.text().split( ',' ) );
        else if( e.nodeName() == "default_boot_label")
            setDefaultBootLabel( e.text() );
        else if( e.nodeName() == "additional_mplayer_options")
            setAdditionalMPlayerOptions( e.text() );
        else if( e.nodeName() == "unwanted_mplayer_options")
            setUnwantedMPlayerOptions( e.text() );
        else if( e.nodeName() == "loop_playlist")
            setLoopPlaylist( e.text().toInt() );
        else
            kDebug() << "(K3b::MovixDoc) unknown movix option: " << e.nodeName();
    }
    // -----------------------------------------------------------------

    // parse files
    // -----------------------------------------------------------------
    if( nodes.item(4).nodeName() != "movix_files" ) {
        kDebug() << "(K3b::MovixDoc) could not find 'movix_files' section.";
        return false;
    }

    // load file items
    QDomNodeList fileList = nodes.item(4).childNodes();
    for( int i = 0; i < fileList.count(); i++ ) {

        QDomElement e = fileList.item(i).toElement();
        if( e.isNull() )
            return false;

        if( e.nodeName() == "file" ) {
            if( !e.hasAttribute( "name" ) ) {
                kDebug() << "(K3b::MovixDoc) found file tag without name attribute.";
                return false;
            }

            QDomElement urlElem = e.firstChild().toElement();
            if( urlElem.isNull() ) {
                kDebug() << "(K3b::MovixDoc) found file tag without url child.";
                return false;
            }

            // emit the signal telling the item is going to be added
            emit aboutToAddMovixItems( m_movixFiles.count(), 1 );

            // create the item
            K3b::MovixFileItem* newK3bItem = new K3b::MovixFileItem( urlElem.text(),
                                                                 this,
                                                                 root(),
                                                                 e.attributeNode("name").value() );
            m_movixFiles.append( newK3bItem );

            // tell the item was already added
            emit addedMovixItems();

            // subtitle file?
            QDomElement subTitleElem = e.childNodes().item(1).toElement();
            if( !subTitleElem.isNull() && subTitleElem.nodeName() == "subtitle_file" ) {
                urlElem = subTitleElem.firstChild().toElement();
                if( urlElem.isNull() ) {
                    kDebug() << "(K3b::MovixDoc) found subtitle_file tag without url child.";
                    return false;
                }

                QString name = K3b::MovixFileItem::subTitleFileName( newK3bItem->k3bName() );
                K3b::MovixSubtitleItem* subItem = new K3b::MovixSubtitleItem( urlElem.text(), this, root(), newK3bItem, name );
                newK3bItem->setSubTitleItem( subItem );

                emit subTitleItemAdded( newK3bItem );
            }
        }
        else {
            kDebug() << "(K3b::MovixDoc) found " << e.nodeName() << " node where 'file' was expected.";
            return false;
        }
    }
    // -----------------------------------------------------------------

    return true;
}


bool K3b::MovixDoc::saveDocumentData( QDomElement* docElem )
{
    QDomDocument doc = docElem->ownerDocument();

    saveGeneralDocumentData( docElem );

    QDomElement optionsElem = doc.createElement( "data_options" );
    saveDocumentDataOptions( optionsElem );

    QDomElement headerElem = doc.createElement( "data_header" );
    saveDocumentDataHeader( headerElem );

    QDomElement movixOptElem = doc.createElement( "movix_options" );
    QDomElement movixFilesElem = doc.createElement( "movix_files" );


    // save the movix options
    QDomElement propElem = doc.createElement( "shutdown" );
    propElem.setAttribute( "activated", shutdown() ? "yes" : "no" );
    movixOptElem.appendChild( propElem );

    propElem = doc.createElement( "reboot" );
    propElem.setAttribute( "activated", reboot() ? "yes" : "no" );
    movixOptElem.appendChild( propElem );

    propElem = doc.createElement( "eject_disk" );
    propElem.setAttribute( "activated", ejectDisk() ? "yes" : "no" );
    movixOptElem.appendChild( propElem );

    propElem = doc.createElement( "random_play" );
    propElem.setAttribute( "activated", randomPlay() ? "yes" : "no" );
    movixOptElem.appendChild( propElem );

    propElem = doc.createElement( "no_dma" );
    propElem.setAttribute( "activated", noDma() ? "yes" : "no" );
    movixOptElem.appendChild( propElem );

    propElem = doc.createElement( "subtitle_fontset" );
    propElem.appendChild( doc.createTextNode( subtitleFontset() ) );
    movixOptElem.appendChild( propElem );

    propElem = doc.createElement( "boot_message_language" );
    propElem.appendChild( doc.createTextNode( bootMessageLanguage() ) );
    movixOptElem.appendChild( propElem );

    propElem = doc.createElement( "audio_background" );
    propElem.appendChild( doc.createTextNode( audioBackground() ) );
    movixOptElem.appendChild( propElem );

    propElem = doc.createElement( "keyboard_language" );
    propElem.appendChild( doc.createTextNode( keyboardLayout() ) );
    movixOptElem.appendChild( propElem );

    propElem = doc.createElement( "codecs" );
    propElem.appendChild( doc.createTextNode( codecs().join(",") ) );
    movixOptElem.appendChild( propElem );

    propElem = doc.createElement( "default_boot_label" );
    propElem.appendChild( doc.createTextNode( defaultBootLabel() ) );
    movixOptElem.appendChild( propElem );

    propElem = doc.createElement( "additional_mplayer_options" );
    propElem.appendChild( doc.createTextNode( additionalMPlayerOptions() ) );
    movixOptElem.appendChild( propElem );

    propElem = doc.createElement( "unwanted_mplayer_options" );
    propElem.appendChild( doc.createTextNode( unwantedMPlayerOptions() ) );
    movixOptElem.appendChild( propElem );

    propElem = doc.createElement( "loop_playlist" );
    propElem.appendChild( doc.createTextNode( QString::number(loopPlaylist()) ) );
    movixOptElem.appendChild( propElem );


    // save the movix items
    Q_FOREACH( K3b::MovixFileItem* item, m_movixFiles ) {
        QDomElement topElem = doc.createElement( "file" );
        topElem.setAttribute( "name", item->k3bName() );
        QDomElement urlElem = doc.createElement( "url" );
        urlElem.appendChild( doc.createTextNode( item->localPath() ) );
        topElem.appendChild( urlElem );
        if( item->subTitleItem() ) {
            QDomElement subElem = doc.createElement( "subtitle_file" );
            urlElem = doc.createElement( "url" );
            urlElem.appendChild( doc.createTextNode( item->subTitleItem()->localPath() ) );
            subElem.appendChild( urlElem );
            topElem.appendChild( subElem );
        }

        movixFilesElem.appendChild( topElem );
    }

    docElem->appendChild( optionsElem );
    docElem->appendChild( headerElem );
    docElem->appendChild( movixOptElem );
    docElem->appendChild( movixFilesElem );

    return true;
}

#include "k3bmovixdoc.moc"
