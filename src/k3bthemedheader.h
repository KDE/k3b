/* 
 *
 * Copyright (C) 2006-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_THEMED_HEADER_H_
#define _K3B_THEMED_HEADER_H_

#include "k3bthememanager.h"

#include <QFrame>

namespace K3b {
    class TitleLabel;
}
class QLabel;

namespace K3b {
class ThemedHeader : public QFrame
{
    Q_OBJECT

public:
    explicit ThemedHeader( QWidget* parent = 0 );
    ThemedHeader( const QString& title, const QString& subtitle, QWidget* parent = 0 );
    ~ThemedHeader() override;

public Q_SLOTS:
    void setTitle( const QString& title, const QString& subtitle = QString() );
    void setSubTitle( const QString& subtitle );
    void setAlignment( Qt::Alignment alignment );
    void setLeftPixmap( Theme::PixmapType );
    void setRightPixmap( Theme::PixmapType );

protected:
    bool event( QEvent* event ) override;

private Q_SLOTS:
    void slotThemeChanged();

private:
    void init();

    TitleLabel* m_titleLabel;
    QLabel* m_leftLabel;
    QLabel* m_rightLabel;
    Theme::PixmapType m_leftPix;
    Theme::PixmapType m_rightPix;
};
}

#endif
