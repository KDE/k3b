/* 
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
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


#ifndef K3BSPLASH_H
#define K3BSPLASH_H

#include <QWidget>

class QLabel;
class QMouseEvent;
class QPaintEvent;
class QString;


namespace K3b {
class Splash : public QWidget
{
    Q_OBJECT

public:
    explicit Splash( QWidget* parent = 0 );
    ~Splash() override;

public Q_SLOTS:
    void show();
    void hide();
    void addInfo( const QString& );

protected:
    void mousePressEvent( QMouseEvent* ) override;
    //  void paintEvent( QPaintEvent* );

private:
    QLabel* m_infoBox;
};
}

#endif
