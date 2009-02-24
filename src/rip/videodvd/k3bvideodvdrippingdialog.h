/*
 *
 * Copyright (C) 2006-2009 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_VIDEODVD_RIPPING_DIALOG_H_
#define _K3B_VIDEODVD_RIPPING_DIALOG_H_

#include <k3binteractiondialog.h>
#include <k3bvideodvd.h>
#include "k3bvideodvdrippingjob.h"

#include <qmap.h>

class Q3CheckListItem;

namespace K3b {
    class VideoDVDRippingWidget;

    class VideoDVDRippingDialog : public InteractionDialog
    {
        Q_OBJECT

    public:
        VideoDVDRippingDialog( const VideoDVD::VideoDVD& dvd,
                               const QList<int>& titles,
                               QWidget *parent = 0 );
        ~VideoDVDRippingDialog();

        void setBaseDir( const QString& path );

        enum FileNamingPattern {
            PATTERN_TITLE_NUMBER         = 't',
            PATTERN_VOLUME_ID            = 'i',
            PATTERN_BEAUTIFIED_VOLUME_ID = 'b',
            PATTERN_LANGUAGE_CODE        = 'l',
            PATTERN_LANGUAGE_NAME        = 'n',
            PATTERN_AUDIO_FORMAT         = 'a',
            PATTERN_AUDIO_CHANNELS       = 'c',
            PATTERN_ORIG_VIDEO_SIZE      = 'v',
            PATTERN_VIDEO_SIZE           = 's',
            PATTERN_ASPECT_RATIO         = 'r',
            PATTERN_CURRENT_DATE         = 'd'
        };

    private Q_SLOTS:
        void slotStartClicked();
        void slotUpdateFilenames();
        void slotUpdateFilesizes();
        void slotUpdateVideoSizes();

    private:
        void populateTitleView( const QList<int>& titles );

        QString createFilename( const VideoDVDRippingJob::TitleRipInfo& info, const QString& pattern ) const;

        void loadSettings( const KConfigGroup& );
        void saveSettings( KConfigGroup );

        VideoDVDRippingWidget* m_w;

        VideoDVD::VideoDVD m_dvd;
        QMap<Q3CheckListItem*, VideoDVDRippingJob::TitleRipInfo> m_titleRipInfos;

        class AudioStreamViewItem;

        class Private;
        Private* d;
    };
}

#endif
