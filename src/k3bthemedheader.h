/* 

    SPDX-FileCopyrightText: 2006-2008 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
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
