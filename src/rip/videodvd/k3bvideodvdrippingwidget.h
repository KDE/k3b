/*
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_VIDEODVD_RIPPING_WIDGET_H_
#define _K3B_VIDEODVD_RIPPING_WIDGET_H_

#include "ui_base_k3bvideodvdrippingwidget.h"

#include "k3bvideodvdtitletranscodingjob.h"

#include <KIO/Global>

class QTimer;

namespace K3b {
    class VideoDVDRippingWidget : public QWidget, public Ui::base_K3bVideoDVDRippingWidget
    {
        Q_OBJECT

    public:
        explicit VideoDVDRippingWidget( QWidget* parent );
        ~VideoDVDRippingWidget() override;

        VideoDVDTitleTranscodingJob::VideoCodec selectedVideoCodec() const;
        VideoDVDTitleTranscodingJob::AudioCodec selectedAudioCodec() const;
        int selectedAudioBitrate() const;
        QSize selectedPictureSize() const;

        void setSelectedVideoCodec( VideoDVDTitleTranscodingJob::VideoCodec codec );
        void setSelectedAudioCodec( VideoDVDTitleTranscodingJob::AudioCodec codec );
        void setSelectedAudioBitrate( int bitrate );
        void setSelectedPictureSize( const QSize& );

        void setNeededSize( KIO::filesize_t );

    Q_SIGNALS:
        void changed();

    private Q_SLOTS:
        void slotUpdateFreeTempSpace();
        void slotSeeSpecialStrings();
        void slotAudioCodecChanged( int codec );
        void slotVideoSizeChanged( int sizeIndex );
        void slotCustomPictureSize();

    private:
        QTimer* m_freeSpaceUpdateTimer;
        qint64 m_neededSize;

        QSize m_customVideoSize;
    };
}

#endif
