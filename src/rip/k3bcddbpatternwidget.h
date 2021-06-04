/*
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/


#ifndef _K3B_CDDB_PATTERN_WIDGET_H_
#define _K3B_CDDB_PATTERN_WIDGET_H_

#include "ui_base_k3bcddbpatternwidget.h"
#include <KConfigGroup>

namespace K3b {
    class CddbPatternWidget : public QWidget, public Ui::base_K3bCddbPatternWidget
    {
        Q_OBJECT

    public:
        explicit CddbPatternWidget( QWidget* parent = 0 );
        ~CddbPatternWidget() override;

        QString filenamePattern() const;
        QString playlistPattern() const;
        QString blankReplaceString() const;
        bool replaceBlanks() const;

    Q_SIGNALS:
        void changed();

    public Q_SLOTS:
        void loadConfig( const KConfigGroup & );
        void saveConfig( KConfigGroup );

    private Q_SLOTS:
        void slotSeeSpecialStrings();
        void slotSeeConditionalInclusion();
    };
}

#endif
