/*

    SPDX-FileCopyrightText: 2005 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_AUDIODATASOURCE_EDITWIDGET_H_
#define _K3B_AUDIODATASOURCE_EDITWIDGET_H_

#include "k3bmsf.h"

#include <QWidget>

namespace K3b {
    class AudioDataSource;
}
namespace K3b {
    class AudioEditorWidget;
}
namespace K3b {
    class MsfEdit;
}

/**
 * Widget to modify the start and end offset of a source or simply change
 * the length of a silence source.
 */
namespace K3b {
    class AudioDataSourceEditWidget : public QWidget
    {
        Q_OBJECT

    public:
        explicit AudioDataSourceEditWidget( QWidget* parent = 0 );
        ~AudioDataSourceEditWidget() override;

        K3b::Msf startOffset() const;

        /**
         * Highest value (meaning to use all the data up to the end of the source)
         * is source::originalLength().
         *
         * Be aware that this differs from AudioDataSource::endOffset() which
         * points after the last used sector for internal reasons.
         */
        K3b::Msf endOffset() const;

    public Q_SLOTS:
        void loadSource( K3b::AudioDataSource* );
        void saveSource();

        void setStartOffset( const K3b::Msf& );
        void setEndOffset( const K3b::Msf& );

    private Q_SLOTS:
        void slotRangeModified( int, const K3b::Msf&, const K3b::Msf& );
        void slotStartOffsetEdited( const K3b::Msf& );
        void slotEndOffsetEdited( const K3b::Msf& );

    private:
        AudioDataSource* m_source;
        int m_rangeId;

        AudioEditorWidget* m_editor;
        MsfEdit* m_editStartOffset;
        MsfEdit* m_editEndOffset;
    };
}

#endif
