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

#include "k3bdatapreparationjob.h"
#include "k3bdatadoc.h"
#include "k3bisooptions.h"
#include "k3bthreadjob.h"
#include "k3bthread.h"
#include "k3bdiritem.h"
#include "k3bfileitem.h"
#include "k3bglobals.h"
#include "k3b_i18n.h"

#include <KCoreAddons/KStringHandler>

#include <QFile>
#include <QFileInfo>
#include <QList>

namespace {
    QString createItemsString( const QList<K3b::DataItem*>& items, int max )
    {
        QString s;
        int cnt = 0;
        for( QList<K3b::DataItem*>::const_iterator it = items.constBegin();
             it != items.constEnd(); ++it ) {

            s += KStringHandler::csqueeze( (*it)->localPath(), 60 );

            ++cnt;
            if( cnt >= max || it == items.constEnd() )
                break;

            s += "<br>";
        }

        if( items.count() > max )
            s += "...";

        return s;
    }
}


class K3b::DataPreparationJob::Private
{
public:
    K3b::DataDoc* doc;

    QList<K3b::DataItem*> nonExistingItems;
    QString listOfRenamedItems;
    QList<K3b::DataItem*> folderSymLinkItems;
};



K3b::DataPreparationJob::DataPreparationJob( K3b::DataDoc* doc, K3b::JobHandler* hdl, QObject* parent )
    : K3b::ThreadJob( hdl, parent ),
      d( new Private() )
{
    d->doc = doc;
}


K3b::DataPreparationJob::~DataPreparationJob()
{
    delete d;
}


bool K3b::DataPreparationJob::run()
{
    // clean up
    d->nonExistingItems.clear();
    d->listOfRenamedItems.truncate(0);
    d->folderSymLinkItems.clear();

    // initialize filenames in the project
    d->doc->prepareFilenames();

    // create the message string for the renamed files
    if( d->doc->needToCutFilenames() ) {
        int maxlines = 10;
        QList<K3b::DataItem*>::const_iterator it;
        QList<K3b::DataItem*> items = d->doc->needToCutFilenameItems();
        for( it = items.constBegin();
             maxlines > 0 && it != items.constEnd();
             ++it, --maxlines ) {
            K3b::DataItem* item = *it;
            d->listOfRenamedItems += i18n("<em>%1</em> renamed to <em>%2</em>",
                                          KStringHandler::csqueeze( item->k3bName(), 30 ),
                                          KStringHandler::csqueeze( item->writtenName(), 30 ) );
            d->listOfRenamedItems += "<br>";
        }
        if( it != items.constEnd() )
            d->listOfRenamedItems += "...";
    }

    //
    // Check for missing files and folder symlinks
    //
    K3b::DataItem* item = d->doc->root();
    while( (item = item->nextSibling()) ) {

        if( item->isSymLink() ) {
            if( d->doc->isoOptions().followSymbolicLinks() ) {
                QFileInfo f( K3b::resolveLink( item->localPath() ) );
                if( !f.exists() ) {
                    d->nonExistingItems.append( item );
                }
                else if( f.isDir() ) {
                    d->folderSymLinkItems.append( item );
                }
            }
        }
        else if( item->isFile() && !QFile::exists( item->localPath() ) ) {
            d->nonExistingItems.append( item );
        }

        if( canceled() ) {
            return false;
        }
    }


    //
    // Now ask the user
    //
    if( !d->listOfRenamedItems.isEmpty() ) {
        if( !questionYesNo( "<p>" + i18n("Some filenames need to be shortened due to the %1 char restriction "
                                         "of the Joliet extensions. If the Joliet extensions are disabled filenames "
                                         "do not have to be shortened but long filenames will not be available on "
                                         "Windows systems.",
                                         d->doc->isoOptions().jolietLong() ? 103 : 64 )
                            + "<p>" + d->listOfRenamedItems,
                            i18n("Warning"),
                            KGuiItem( i18n("Shorten Filenames") ),
                            KGuiItem( i18n("Disable Joliet extensions") ) ) ) {
            // No -> disable joliet
            // for now we enable RockRidge to be sure we did not lie above (keep long filenames)
            K3b::IsoOptions op = d->doc->isoOptions();
            op.setCreateJoliet( false );
            op.setCreateRockRidge( true );
            d->doc->setIsoOptions( op );
            d->doc->prepareFilenames();
        }
    }

    //
    // The joliet extension encodes the volume desc in UCS-2, i.e. uses 16 bit for each char.
    // Thus, the max length here is 16.
    //
    if( d->doc->isoOptions().createJoliet() &&
        d->doc->isoOptions().volumeID().length() > 16 ) {
        if( !questionYesNo( "<p>" + i18n("The Joliet extensions (which are needed for long filenames on Windows systems) "
                                         "restrict the length of the volume descriptor (the name of the filesystem) "
                                         "to %1 characters. The selected descriptor '%2' is longer than that. Do you "
                                         "want it to be cut or do you want to go back and change it manually?",
                                         QString::number( 16 ), d->doc->isoOptions().volumeID() ),
                            i18n("Warning"),
                            KGuiItem( i18n("Cut volume descriptor in the Joliet tree") ),
                            KGuiItem( i18n("Cancel and go back") ) ) ) {
            cancel();
            return false;
        }
    }

    //
    // Check for missing files
    //
    if( !d->nonExistingItems.isEmpty() ) {
        if( questionYesNo( "<p>" + i18n("The following files could not be found. Do you want to remove them from the "
                                        "project and continue without adding them to the image?") +
                           "<p>" + createItemsString( d->nonExistingItems, 10 ),
                           i18n("Warning"),
                           KGuiItem( i18n("Remove missing files and continue") ),
                           KGuiItem( i18n("Cancel and go back") ) ) ) {
            for( QList<K3b::DataItem*>::const_iterator it = d->nonExistingItems.constBegin();
                 it != d->nonExistingItems.constEnd(); ++it ) {
                delete *it;
            }
        }
        else {
            cancel();
            return false;
        }
    }

    //
    // Warn about symlinks to folders
    //
    if( d->doc->isoOptions().followSymbolicLinks() && !d->folderSymLinkItems.isEmpty() ) {
        if( !questionYesNo( "<p>" + i18n("K3b is not able to follow symbolic links to folders after they have been added "
                                         "to the project. Do you want to continue "
                                         "without writing the symbolic links to the image?") +
                            "<p>" + createItemsString( d->folderSymLinkItems, 10 ),
                            i18n("Warning"),
                            KGuiItem( i18n("Discard symbolic links to folders") ),
                            KGuiItem( i18n("Cancel and go back") ) ) ) {
            cancel();
            return false;
        }
    }

    return true;
}


