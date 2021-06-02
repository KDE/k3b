/* 

    SPDX-FileCopyrightText: 2006 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/

#ifndef _K3B_THEMED_LABEL_H_
#define _K3B_THEMED_LABEL_H_

#include "k3bthememanager.h"

#include <KSqueezedTextLabel>

namespace K3b {

class ThemedLabel : public KSqueezedTextLabel
{
    Q_OBJECT

public:
    explicit ThemedLabel( QWidget* parent = 0 );
    explicit ThemedLabel( const QString& text, QWidget* parent = 0 );
    explicit ThemedLabel( Theme::PixmapType, QWidget* parent = 0 );

protected:
    bool event( QEvent* event ) override;

public Q_SLOTS:
    void setThemePixmap( Theme::PixmapType );

private Q_SLOTS:
    void slotThemeChanged();

private:
    int m_themePixmapCode;
};

}

#endif
