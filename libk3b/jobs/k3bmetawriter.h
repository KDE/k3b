/*
 * Copyright (C) 2009 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_META_WRITER_H_
#define _K3B_META_WRITER_H_

#include "k3babstractwriter.h"
#include "k3b_export.h"

// TODO: - handle media and project size.
//       - Use the MetaWriter in all projects

namespace K3b {
    /**
     * Meta writer which wraps around the cdrecord, cdrdao, and growisofs writers.
     * Its main use is to provide one consistent interface and keep all writing mode
     * selection and media requesting in one class.
     *
     * It provides the following features:
     * \li select writing app automatically
     * \li select writing mode automatically
     * \li wait for a medium and emit info messages about what is being written
     * \li restrict and modify the supported media types according to the set writing mode
     *    (Caution: this could lead to failures since the project size if not checked)
     */
    class LIBK3B_EXPORT MetaWriter : public AbstractWriter
    {
        Q_OBJECT

    public:
        MetaWriter( Device::Device*, JobHandler* hdl, QObject* parent = 0 );
        ~MetaWriter() override;

        QIODevice* ioDevice() const override;

        /**
         * \return The writing app used after starting the job.
         */
        K3b::WritingApp usedWritingApp() const;

        /**
         * \return The writing mode used after starting the job.
         */
        K3b::WritingMode usedWritingMode() const;

    public Q_SLOTS:
        void start() override;
        void cancel() override;

        /**
         * Be aware that the toc that is set here is not necessarily the final toc of the medium.
         * It rather represents one session to be written.
         */
        void setSessionToWrite( const Device::Toc& toc, const QStringList& images = QStringList() );
        void setSupportedWritingMedia( Device::MediaTypes types );
        void setWritingApp( WritingApp app );
        void setWritingMode( WritingMode mode );
        void setCueFile( const QString& s);
        void setClone( bool b );
        void setMultiSession( bool b );
        void setCdText( const Device::CdText& cdtext );
        void setLayerBreak( qint64 lb );
        void setHideFirstTrack( bool b );

    private Q_SLOTS:
        void slotWritingJobFinished( bool success );

    private:
        bool ensureSettingsIntegrity();
        bool determineUsedAppAndMode();
        bool setupCdrecordJob();
        bool setupCdrskinJob();
        bool setupCdrdaoJob();
        bool setupGrowisofsob();
        bool startTrackWriting();

        void informUser();

        class Private;
        Private* const d;
    };
}

#endif
