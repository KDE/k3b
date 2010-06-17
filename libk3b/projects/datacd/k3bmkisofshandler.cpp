/*
 *
 * Copyright (C) 2005-2008 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bmkisofshandler.h"

#include "k3bexternalbinmanager.h"
#include "k3bcore.h"
#include "k3bjob.h"

#include <kdebug.h>
#include <klocale.h>

#include <cmath>



class K3b::MkisofsHandler::Private
{
public:
    const K3b::ExternalBin* mkisofsBin;
    double firstProgressValue;
    bool readError;
};


K3b::MkisofsHandler::MkisofsHandler()
{
    d = new Private;
    d->mkisofsBin = 0;
}


K3b::MkisofsHandler::~MkisofsHandler()
{
    delete d;
}


bool K3b::MkisofsHandler::mkisofsReadError() const
{
    return d->readError;
}


const K3b::ExternalBin* K3b::MkisofsHandler::initMkisofs()
{
    d->mkisofsBin = k3bcore->externalBinManager()->binObject( "mkisofs" );

    if( d->mkisofsBin ) {
        if( !d->mkisofsBin->copyright().isEmpty() )
            handleMkisofsInfoMessage( i18n("Using %1 %2 – Copyright © %3",
                                           QString("mkisofs"),
                                           d->mkisofsBin->version(),
                                           d->mkisofsBin->copyright()),
                                      K3b::Job::MessageInfo );

        d->firstProgressValue = -1;
        d->readError = false;
    }
    else {
        kDebug() << "(K3b::MkisofsHandler) could not find mkisofs executable";
        handleMkisofsInfoMessage( i18n("Mkisofs executable not found."), K3b::Job::MessageError );
    }

    return d->mkisofsBin;
}


void K3b::MkisofsHandler::parseMkisofsOutput( const QString& line )
{
    if( !line.isEmpty() ) {
        if( line.startsWith( d->mkisofsBin->path() ) ) {
            // error or warning
            QString errorLine = line.mid( d->mkisofsBin->path().length() + 2 );
            if( errorLine.startsWith( "Input/output error. Cannot read from" ) ) {
                handleMkisofsInfoMessage( i18n("Read error from file '%1'", errorLine.mid( 38, errorLine.length()-40 ) ),
                                          K3b::Job::MessageError );
                d->readError = true;
            }
            else if( errorLine.startsWith( "Value too large for defined data type" ) ) {
                handleMkisofsInfoMessage( i18n("Used version of mkisofs does not have large file support."), K3b::Job::MessageError );
                handleMkisofsInfoMessage( i18n("Files bigger than 2 GB cannot be handled."), K3b::Job::MessageError );
                d->readError = true;
            }
        }
        else if( line.contains( "done, estimate" ) ) {
            int p = parseMkisofsProgress( line );
            if( p != -1 )
                handleMkisofsProgress( p );
        }
        else if( line.contains( "extents written" ) ) {
            handleMkisofsProgress( 100 );
        }
        else if( line.startsWith( "Incorrectly encoded string" ) ) {
            handleMkisofsInfoMessage( i18n("Encountered an incorrectly encoded filename '%1'",
                                           line.section( QRegExp("[\\(\\)]"), 1, 1 )), K3b::Job::MessageError );
            handleMkisofsInfoMessage( i18n("This may be caused by a system update which changed the local character set."), K3b::Job::MessageError );
            handleMkisofsInfoMessage( i18n("You may use convmv (http://j3e.de/linux/convmv/) to fix the filename encoding."), K3b::Job::MessageError );
            d->readError = true;
        }
        else if( line.endsWith( "has not an allowable size." ) ) {
            handleMkisofsInfoMessage( i18n("The boot image has an invalid size."), K3b::Job::MessageError );
            d->readError = true;
        }
        else if( line.endsWith( "has multiple partitions." ) ) {
            handleMkisofsInfoMessage( i18n("The boot image contains multiple partitions.."), K3b::Job::MessageError );
            handleMkisofsInfoMessage( i18n("A hard-disk boot image has to contain a single partition."), K3b::Job::MessageError );
            d->readError = true;
        }
        else {
            kDebug() << "(mkisofs) " << line;
        }
    }
}


int K3b::MkisofsHandler::parseMkisofsProgress( const QString& line )
{
    //
    // in multisession mode mkisofs' progress does not start at 0 but at (X+Y)/X
    // where X is the data already on the cd and Y the data to create
    // This is not very dramatic but kind or ugly.
    // We just save the first emitted progress value and to some math ;)
    //

    QString perStr = line;
    perStr.truncate( perStr.indexOf('%') );
    bool ok;
    double p = perStr.toDouble( &ok );
    if( !ok ) {
        kDebug() << "(K3b::MkisofsHandler) Parsing did not work for " << perStr;
        return -1;
    }
    else {
        if( d->firstProgressValue < 0 )
            d->firstProgressValue = p;

        return( (int)::ceil( (p - d->firstProgressValue)*100.0/(100.0 - d->firstProgressValue) ) );
    }
}
