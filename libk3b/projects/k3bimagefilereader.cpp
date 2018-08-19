/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bimagefilereader.h"

#include <QDebug>
#include <QFile>



class K3b::ImageFileReader::Private
{
public:
    Private()
        : isValid(false) {
    }

    QString filename;
    QString imageFilename;
    bool isValid;
};


K3b::ImageFileReader::ImageFileReader()
{
    d = new Private();
}


K3b::ImageFileReader::~ImageFileReader()
{
    delete d;
}


void K3b::ImageFileReader::openFile( const QString& filename )
{
    d->filename = filename;
    d->imageFilename = QString();
    setValid(false);

    if( !filename.isEmpty() )
        readFile();
}


void K3b::ImageFileReader::setValid( bool b )
{
    d->isValid = b;
}


void K3b::ImageFileReader::setImageFilename( const QString& filename )
{
    d->imageFilename = filename;
}


bool K3b::ImageFileReader::isValid() const
{
    return d->isValid;
}


QString K3b::ImageFileReader::filename() const
{
    return d->filename;
}


QString K3b::ImageFileReader::imageFilename() const
{
    return d->imageFilename;
}
