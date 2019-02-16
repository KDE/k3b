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

#ifndef _K3B_GROWISOFS_WRITER_H_
#define _K3B_GROWISOFS_WRITER_H_

#include "k3babstractwriter.h"

#include <QProcess>


namespace K3b {
    namespace Device {
        class Device;
    }

    class GrowisofsWriter : public AbstractWriter
    {
        Q_OBJECT

    public:
        GrowisofsWriter( Device::Device*, JobHandler*,
                         QObject* parent = 0 );
        ~GrowisofsWriter() override;

        bool active() const override;

        /**
         * Write to the writer process.
         * FIXME: make this an overloaded method from AbstractWriter
         */
        qint64 write( const char* data, qint64 maxSize );

        QIODevice* ioDevice() const override;

    public Q_SLOTS:
        void start() override;
        void cancel() override;

        void setWritingMode( K3b::WritingMode mode );

        /**
         * If true the growisofs parameter -M is used in favor of -Z.
         */
        void setMultiSession( bool b );

        /**
         * Only used in DAO mode and only supported with growisofs >= 5.15
         * @param size size in blocks
         */
        void setTrackSize( long size );

        /**
         * Use this in combination with setTrackSize when writing double layer media.
         * @param lb The number of data sectors in the first layer. It needs to be less or equal
         *           to tracksize/2. The writer will pad the second layer with zeros if
         *           break < tracksize/2.
         *           If set to 0 this setting will be ignored.
         */
        void setLayerBreak( long lb );

        /**
         * Close the DVD to enable max DVD compatibility (uses the growisofs --dvd-compat parameter)
         * This will also be used in DAO mode and when the layerBreak has been set.
         */
        void setCloseDvd( bool );

        /**
         * set this to QString() or an empty string to let the writer
         * read it's data from fd()
         */
        void setImageToWrite( const QString& );

        /**
         * While reading the image from stdin growisofs needs
         * a valid -C parameter for multisession.
         */
        void setMultiSessionInfo( const QString& );

    protected:
        bool prepareProcess();

    protected Q_SLOTS:
        void slotReceivedStderr( const QString& );
        void slotProcessExited( int, QProcess::ExitStatus );
        void slotThroughput( int t );
        void slotFlushingCache();

    private:
        class Private;
        Private* d;
    };
}

#endif
