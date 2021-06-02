/*

    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <config-k3b.h>

#include "k3bclonetocreader.h"
#include "k3bdeviceglobals.h"
#include "k3bglobals.h"

#include <QDebug>
#include <QFile>
#include <QFileInfo>


class K3b::CloneTocReader::Private
{
public:
    Private()
        : size(0) {
    }

    K3b::Msf size;
    QString tocFile;
};



K3b::CloneTocReader::CloneTocReader( const QString& filename )
    : K3b::ImageFileReader()
{
    d = new Private;
    openFile( filename );
}


K3b::CloneTocReader::~CloneTocReader()
{
    delete d;
}


K3b::Msf K3b::CloneTocReader::imageSize() const
{
    return d->size;
}


void K3b::CloneTocReader::readFile()
{
    // first of all we check if we find the image file which contains the data for this toc
    // cdrecord always uses this strange file naming:
    //   somedata
    //   somedata.toc

    // filename should always be the toc file
    if( filename().right( 4 ) == ".toc" )
        d->tocFile = filename();
    else
        d->tocFile = filename() + ".toc";

    // now get rid of the ".toc" extension
    QString imageFileName = d->tocFile.left( d->tocFile.length()-4 );
    if( !QFile::exists( imageFileName ) ) {
        qDebug() << "(K3b::CloneTocReader) could not find image file " << imageFileName;
        return;
    }

    setImageFilename( imageFileName );

    d->size = 0;

    QFile f( d->tocFile );
    if( f.open( QIODevice::ReadOnly ) ) {
        //
        // Inspired by clone.c from the cdrecord sources
        //
        char buffer[2048];
        int read = f.read( buffer, 2048 );
        f.close();

        if( read == 2048 ) {
            qDebug() << "(K3b::CloneTocReader) TOC too large.";
            return;
        }

        // the toc starts with a tocheader
        struct tocheader {
            unsigned char len[2];
            unsigned char first; // first session
            unsigned char last; // last session
        };

        struct tocheader* th = (struct tocheader*)buffer;
        int dataLen = K3b::Device::from2Byte( th->len ) + 2;  // the len field does not include it's own length

        if( th->first != 1 ) {
            qDebug() << "(K3b::CloneTocReader) first session != 1";
            return;
        }

        // the following bytes are multiple instances of
        struct ftrackdesc {
            unsigned char sess_number;
#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
            unsigned char adr      : 4;
            unsigned char control  : 4;
#else
            unsigned char control  : 4;
            unsigned char adr      : 4;
#endif
            unsigned char track;
            unsigned char point;
            unsigned char amin;
            unsigned char asec;
            unsigned char aframe;
            unsigned char res7;
            unsigned char pmin;
            unsigned char psec;
            unsigned char pframe;
        };

        for( int i = 4; i < dataLen; i += 11) {
            struct ftrackdesc* ft = (struct ftrackdesc*)&buffer[i];

            if( ft->sess_number != 1 ) {
                qDebug() << "(K3b::CloneTocReader} session number != 1";
                return;
            }

            // now we check some of the values
            if( ft->point >= 0x1 && ft->point <= 0x63 ) {
                if( ft->adr == 1 ) {
                    // check track starttime
                    if( ft->psec > 60 || ft->pframe > 75 ) {
                        qDebug() << "(K3b::CloneTocReader) invalid track start: "
                                 << (int)ft->pmin << "."
                                 << (int)ft->psec << "."
                                 << (int)ft->pframe << endl;
                        return;
                    }
                }
            }
            else {
                switch( ft->point ) {
                case 0xa0:
                    if( ft->adr != 1 ) {
                        qDebug() << "(K3b::CloneTocReader) adr != 1";
                        return;
                    }

                    // disk type in psec
                    if( ft->psec != 0x00 && ft->psec != 0x10 && ft->psec != 0x20 ) {
                        qDebug() << "(K3b::CloneTocReader) invalid disktype: " << ft->psec;
                        return;
                    }

                    if( ft->pmin != 1 ) {
                        qDebug() << "(K3b::CloneTocReader) first track number != 1 ";
                        return;
                    }

                    if( ft->pframe != 0x0 ) {
                        qDebug() << "(K3b::CloneTocReader) found data when there should be 0x0";
                        return;
                    }
                    break;

                case  0xa1:
                    if( ft->adr != 1 ) {
                        qDebug() << "(K3b::CloneTocReader) adr != 1";
                        return;
                    }

                    if( !(ft->pmin >= 1) ) {
                        qDebug() << "(K3b::CloneTocReader) last track number needs to be >= 1.";
                        return;
                    }
                    if( ft->psec != 0x0 || ft->pframe != 0x0 ) {
                        qDebug() << "(K3b::CloneTocReader) found data when there should be 0x0";
                        return;
                    }
                    break;

                case 0xa2:
                    if( ft->adr != 1 ) {
                        qDebug() << "(K3b::CloneTocReader) adr != 1";
                        return;
                    }

                    // start of the leadout = size of the image
                    // subtract 2 seconds since in cdrecord other than in K3b lba 0 = msf 2:00
                    // (the cdrecord way is actually more accurate but we use k3b::Msf for many
                    // things and it is simpler this way.)
                    d->size = K3b::Msf( ft->pmin, ft->psec, ft->pframe ) - K3b::Msf( 0, 2, 0 );

                    // leadout... no check so far...
                    break;

                default:
                    if( ft->adr != 5 ) {
                        qDebug() << "(K3b::CloneTocReader) adr != 5";
                        return;
                    }
                    break;
                }
            }
        }

        if( d->size.rawBytes() != KIO::filesize_t(QFileInfo(imageFileName).size()) ) {
            qDebug() << "(K3b::CloneTocReader) image file size invalid.";
            return;
        }

        // ok, could be a cdrecord toc file
        setValid(true);
    }
    else {
        qDebug() << "(K3b::CloneTocReader) could not open file " << d->tocFile;
    }
}
