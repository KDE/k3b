/*
    SPDX-FileCopyrightText: 2004-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_AUDIO_CONVERTING_OPTION_WIDGET_H_
#define _K3B_AUDIO_CONVERTING_OPTION_WIDGET_H_

#include "ui_base_k3baudiorippingoptionwidget.h"

#include <KIO/Global>
#include <QCheckBox>

class KConfigGroup;

namespace K3b {
    class AudioEncoder;


    /**
     * Internally used by AudioConvertingDialog
     */
    class AudioConvertingOptionWidget : public QWidget, public Ui::base_K3bAudioRippingOptionWidget
    {
        Q_OBJECT

    public:
        explicit AudioConvertingOptionWidget( QWidget* parent );
        ~AudioConvertingOptionWidget() override;

        void setBaseDir( const QString& path );

        void setNeededSize( KIO::filesize_t );

        /**
         * @returns 0 if wave is selected
         */
        AudioEncoder* encoder() const;
        QString extension() const;

        QString baseDir() const;

        bool createPlaylist() const { return m_checkCreatePlaylist->isChecked(); }
        bool playlistRelativePath() const { return m_checkPlaylistRelative->isChecked(); }
        bool createSingleFile() const { return m_checkSingleFile->isChecked(); }
        bool createCueFile() const { return m_checkWriteCueFile->isChecked(); }

    public Q_SLOTS:
        void loadConfig( const KConfigGroup& );
        void saveConfig( KConfigGroup );

    Q_SIGNALS:
        void changed();

    private Q_SLOTS:
        void slotConfigurePlugin();
        void slotUpdateFreeTempSpace();
        void slotEncoderChanged();

    private:
        class Private;
        Private* d;
    };
}

#endif
