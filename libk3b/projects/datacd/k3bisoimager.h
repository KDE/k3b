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

#ifndef K3B_ISO_IMAGER_H
#define K3B_ISO_IMAGER_H

#include "k3bjob.h"
#include "k3bmkisofshandler.h"
#include "k3bprocess.h"

#include <QStringList>

class QTextStream;
class QTemporaryFile;

namespace K3b {
    class DataDoc;
    class DirItem;
    class FileItem;

    class IsoImager : public Job, public MkisofsHandler
    {
        Q_OBJECT

    public:
        IsoImager( DataDoc*, JobHandler*, QObject* parent = 0 );
        virtual ~IsoImager();

        virtual bool active() const;

        int size() const { return m_mkisofsPrintSizeResult; }

        virtual bool hasBeenCanceled() const;

        QIODevice* ioDevice() const;

    public Q_SLOTS:
        /**
         * Starts the actual image creation. Always run init()
         * before starting the image creation
         */
        virtual void start();
        virtual void cancel();

        /**
         * Initialize the image creator. This calculates the image size and performs
         * some checks on the project.
         *
         * The initialization process also finishes with the finished() signal just
         * like a normal job operation. Get the calculated image size via size()
         */
        virtual void init();

        /**
         * Only calculates the size of the image without the additional checks in
         * init()
         *
         * Use this if you need to recalculate the image size for example if the
         * multisession info changed.
         */
        virtual void calculateSize();

        /**
         * If dev == 0 IsoImager will ignore the data in the previous session.
         * This is usable for CD-Extra.
         */
        void setMultiSessionInfo( const QString&, Device::Device* dev = 0 );

        QString multiSessionInfo() const;
        Device::Device* multiSessionImportDevice() const;

        Device::Device* device() const { return m_device; }
        DataDoc* doc() const { return m_doc; }

    protected:
        virtual void handleMkisofsProgress( int );
        virtual void handleMkisofsInfoMessage( const QString&, int );

        virtual bool addMkisofsParameters( bool printSize = false );

        /**
         * calls writePathSpec, writeRRHideFile, and writeJolietHideFile
         */
        bool prepareMkisofsFiles();

        /**
         * The dummy dir is used to create dirs on the iso-filesystem.
         *
         * @return an empty dummy dir for use with DirItems.
         */
        QString dummyDir( DirItem* );

        void outputData();
        void initVariables();
        virtual void cleanup();
        void clearDummyDirs();

        /**
         * @returns The number of entries written or -1 on error
         */
        virtual int writePathSpec();
        bool writeRRHideFile();
        bool writeJolietHideFile();
        bool writeSortWeightFile();

        // used by writePathSpec
        virtual int writePathSpecForDir( DirItem* dirItem, QTextStream& stream );
        virtual void writePathSpecForFile( FileItem*, QTextStream& stream );
        QString escapeGraftPoint( const QString& str );

        QTemporaryFile* m_pathSpecFile;
        QTemporaryFile* m_rrHideFile;
        QTemporaryFile* m_jolietHideFile;
        QTemporaryFile* m_sortWeightFile;

        Process* m_process;

        bool m_canceled;

    protected Q_SLOTS:
        virtual void slotReceivedStderr( const QString& );
        virtual void slotProcessExited( int, QProcess::ExitStatus );

    private Q_SLOTS:
        void slotCollectMkisofsPrintSizeStderr( const QString& );
        void slotCollectMkisofsPrintSizeStdout( const QString& );
        void slotMkisofsPrintSizeFinished();
        void slotDataPreparationDone( bool success );

    private:
        void startSizeCalculation();

        class Private;
        Private* d;

        DataDoc* m_doc;

        bool m_noDeepDirectoryRelocation;

        bool m_importSession;
        QString m_multiSessionInfo;
        Device::Device* m_device;

        // used for mkisofs -print-size parsing
        QString m_collectedMkisofsPrintSizeStdout;
        QString m_collectedMkisofsPrintSizeStderr;
        int m_mkisofsPrintSizeResult;

        QStringList m_tempFiles;

        bool m_containsFilesWithMultibleBackslashes;

        // used to create a unique session id
        static int s_imagerSessionCounter;

        int m_sessionNumber;
    };
}

#endif
