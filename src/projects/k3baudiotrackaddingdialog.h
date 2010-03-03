/*
 *
 * Copyright (C) 2006-2010 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2010 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_AUDIO_TRACK_ADDING_DIALOG_H_
#define _K3B_AUDIO_TRACK_ADDING_DIALOG_H_

#include "k3bjobhandler.h"
#include <kdialog.h>
#include <kurl.h>
#include <qstringlist.h>


class QLabel;

namespace K3b {
    class BusyWidget;
    class AudioTrack;
    class AudioDataSource;
    class AudioDoc;
    class AudioFileAnalyzerJob;

    class AudioTrackAddingDialog : public KDialog, public JobHandler
    {
        Q_OBJECT

    public:
        AudioTrackAddingDialog( const KUrl::List& urls,
                                AudioDoc* doc,
                                AudioTrack* afterTrack = 0,
                                AudioTrack* parentTrack = 0,
                                AudioDataSource* afterSource = 0,
                                QWidget* parent = 0 );
        ~AudioTrackAddingDialog();

        /**
         * shows AudioTrackAddingDialog in non-blocking fashion
         * (doesn't wait till dialog is closed)
         */
        static void addUrls( const KUrl::List& urls,
                            AudioDoc* doc,
                            AudioTrack* afterTrack = 0,
                            AudioTrack* parentTrack = 0,
                            AudioDataSource* afterSource = 0,
                            QWidget* parent = 0 );

    private Q_SLOTS:
        void slotAddUrls();
        void slotAnalysingFinished( bool );
        void slotCancel();

    private:
        /**
         * @reimplemented from JobHandler
         */
        Device::MediaType waitForMedium( Device::Device*,
                                         Device::MediaStates = Device::STATE_EMPTY,
                                         Device::MediaTypes = Device::MEDIA_WRITABLE_CD,
                                         const K3b::Msf& = K3b::Msf(),
                                         const QString& = QString() ) { return Device::MEDIA_UNKNOWN; }

        /**
         * @reimplemented from JobHandler
         */
        bool questionYesNo( const QString&,
                            const QString& = QString(),
                            const KGuiItem& = KStandardGuiItem::yes(),
                            const KGuiItem& = KStandardGuiItem::no() ) { return false; }

        /**
         * @reimplemented from JobHandler
         */
        void blockingInformation( const QString&,
                                  const QString& = QString() ) {}

        BusyWidget* m_busyWidget;
        QLabel* m_infoLabel;

        QStringList m_unreadableFiles;
        QStringList m_notFoundFiles;
        QStringList m_nonLocalFiles;
        QStringList m_unsupportedFiles;

        KUrl::List m_urls;

        AudioDoc* m_doc;
        AudioTrack* m_trackAfter;
        AudioTrack* m_parentTrack;
        AudioDataSource* m_sourceAfter;

        KUrl m_cueUrl;

        bool m_bCanceled;

        AudioFileAnalyzerJob* m_analyserJob;
    };
}

#endif
