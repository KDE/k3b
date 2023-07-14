/*
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bvideodvdimager.h"
#include "k3bvideodvddoc.h"
#include "k3bdiritem.h"
#include "k3bfileitem.h"
#include "k3bprocess.h"
#include "k3bglobals.h"
#include "k3bisooptions.h"
#include "k3b_i18n.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QList>
#include <QTemporaryDir>
#include <QTextStream>

#include <unistd.h>


class K3b::VideoDvdImager::Private
{
public:
    K3b::VideoDvdDoc* doc;

    QScopedPointer<QTemporaryDir> tempDir;
};


K3b::VideoDvdImager::VideoDvdImager( K3b::VideoDvdDoc* doc, K3b::JobHandler* jh, QObject* parent )
    : K3b::IsoImager( doc, jh, parent ),
      d( new Private )
{
    d->doc = doc;
}


K3b::VideoDvdImager::~VideoDvdImager()
{
}


void K3b::VideoDvdImager::start()
{
    fixVideoDVDSettings();
    K3b::IsoImager::start();
}


void K3b::VideoDvdImager::init()
{
    fixVideoDVDSettings();
    K3b::IsoImager::init();
}


void K3b::VideoDvdImager::fixVideoDVDSettings()
{
    // Video DVD defaults, we cannot set these in K3b::VideoDvdDoc since they
    // will be overwritten in the burn dialog unless we create some K3b::VideoDVDIsoOptions
    // class with different defaults. But since the whole Video DVD project is a hack we
    // go the easy road.
    K3b::IsoOptions o = d->doc->isoOptions();
    o.setISOLevel(1);
    o.setISOallow31charFilenames(false);
    o.setCreateJoliet(false);
    o.setJolietLong(false);
    o.setCreateRockRidge(false);
    o.setCreateUdf(true);
    d->doc->setIsoOptions( o );
}


void K3b::VideoDvdImager::calculateSize()
{
    fixVideoDVDSettings();
    K3b::IsoImager::calculateSize();
}


int K3b::VideoDvdImager::writePathSpec()
{
    //
    // Create a temp dir and link all contents of the VIDEO_TS dir to make mkisofs
    // able to handle the VideoDVD stuff.
    //
    // mkisofs is not able to create VideoDVDs from graft-points.
    //
    // We do this here since K3b::IsoImager::start calls cleanup which deletes the temp files
    //
    d->tempDir.reset(new QTemporaryDir(QDir::tempPath() + "/k3bVideoDvdXXXXXX"));
    if (!d->tempDir->isValid()) {
        emit infoMessage(xi18n("Unable to create Invalid temporary folder <filename>%1</filename>.",
                         d->tempDir->path()), MessageError);
        return -1;
    }

    const auto videoDir =
#if QT_VERSION < 0x050900
        d->tempDir->path() + "/VIDEO_TS";
#else
        d->tempDir->filePath("VIDEO_TS");
#endif
    if (!QDir().mkpath(videoDir)) {
        emit infoMessage(xi18n("Unable to create temporary folder <filename>%1</filename>.",
                         videoDir), MessageError);
        return -1;
    }

    Q_FOREACH(const K3b::DataItem* item, d->doc->videoTsDir()->children()) {
        if (item->isDir()) {
            emit infoMessage(xi18n("Found invalid entry in the VIDEO_TS folder <filename>%1</filename>.",
                             item->k3bName()), MessageError);
            return -1;
        }

        // convert to upper case names
        if (::symlink(QFile::encodeName(item->localPath()),
                      QFile::encodeName(videoDir + '/' + item->k3bName().toUpper())) == -1) {
            emit infoMessage(xi18n("Unable to link temporary file in folder <filename>%1</filename>.",
                             d->tempDir->path()), MessageError);
            return -1;
        }
    }


    return K3b::IsoImager::writePathSpec();
}


int K3b::VideoDvdImager::writePathSpecForDir( K3b::DirItem* dirItem, QTextStream& stream )
{
    //
    // We handle the VIDEO_TS dir differently since otherwise mkisofs is not able to
    // open the VideoDVD structures (see addMkisofsParameters)
    //
    if( dirItem == d->doc->videoTsDir() ) {
        return 0;
    }

    int num = 0;
    Q_FOREACH( K3b::DataItem* item, dirItem->children() ) {
        num++;

        if( item->isDir() ) {
            // we cannot add the video_ts dir twice
            if( item != d->doc->videoTsDir() ) {
                stream << escapeGraftPoint( item->writtenPath() )
                       << "="
                       << escapeGraftPoint( dummyDir( static_cast<K3b::DirItem*>(item) ) ) << "\n";
            }

            int x = writePathSpecForDir( dynamic_cast<K3b::DirItem*>(item), stream );
            if( x >= 0 )
                num += x;
            else
                return -1;
        }
        else {
            writePathSpecForFile( static_cast<K3b::FileItem*>(item), stream );
        }
    }

    return num;
}


bool K3b::VideoDvdImager::addMkisofsParameters( bool printSize )
{
    // Here is another bad design: we assume that K3b::IsoImager::start does not add additional
    // parameters to the process. :(
    if( K3b::IsoImager::addMkisofsParameters( printSize ) ) {
        *m_process << "-dvd-video";
        *m_process << "-f"; // follow symlinks
        *m_process << (d->tempDir ? d->tempDir->path() : QString());
        return true;
    }
    else
        return false;
}


void K3b::VideoDvdImager::cleanup()
{
    d->tempDir.reset();

    K3b::IsoImager::cleanup();
}


void K3b::VideoDvdImager::slotReceivedStderr( const QString& line )
{
    if( line.contains( "Unable to make a DVD-Video image" ) ) {
        emit infoMessage( i18n("The project does not contain all necessary Video DVD files."), MessageWarning );
        emit infoMessage( i18n("The resulting DVD will most likely not be playable on a Hifi DVD player."), MessageWarning );
    }
    else
        K3b::IsoImager::slotReceivedStderr( line );
}

#include "moc_k3bvideodvdimager.cpp"
