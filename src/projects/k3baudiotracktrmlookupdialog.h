/*

    SPDX-FileCopyrightText: 2005-2008 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/

#ifndef _K3B_AUDIOTRACK_TRM_LOOKUP_DIALOG_H_
#define _K3B_AUDIOTRACK_TRM_LOOKUP_DIALOG_H_

#include <QList>
#include <QLabel>
#include <QDialog>

class QLabel;
namespace K3b {
    class AudioTrack;
}
namespace K3b {
    class MusicBrainzJob;
}
namespace K3b {
    class BusyWidget;
}
class QEventLoop;
class QPushButton;


namespace K3b {
    class AudioTrackTRMLookupDialog : public QDialog
    {
        Q_OBJECT

    public:
        explicit AudioTrackTRMLookupDialog( QWidget* parent = 0 );
        ~AudioTrackTRMLookupDialog() override;

        /**
         * This will show the dialog and start the lookup
         */
        int lookup( const QList<AudioTrack*>& tracks );

    public Q_SLOTS:
        void reject() override;

    private Q_SLOTS:
        void slotMbJobFinished( bool );
        void slotMbJobInfoMessage( const QString&, int );
        void slotTrackFinished( K3b::AudioTrack* track, bool success );

    private:
        QLabel* m_infoLabel;
        BusyWidget* m_busyWidget;
        QPushButton* m_cancelButton;
        MusicBrainzJob* m_mbJob;
        QEventLoop* m_loop;
    };
}

#endif
