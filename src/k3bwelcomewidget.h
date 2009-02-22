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


#ifndef _K3B_WELCOME_WIDGET_H_
#define _K3B_WELCOME_WIDGET_H_

#include <QList>
#include <QMap>
#include <QImage>
#include <QPixmap>
#include <QScrollArea>

#include <KUrl>

namespace K3b {
    class FlatButton;
}
namespace K3b {
    class MainWindow;
}
class KConfigGroup;
class QDragEnterEvent;
class QDropEvent;
class QMouseEvent;
class QPaintEvent;
class QResizeEvent;
class QShowEvent;
class QTextDocument;


namespace K3b {
class WelcomeWidget : public QScrollArea
{
    Q_OBJECT

public:
    WelcomeWidget( MainWindow*, QWidget* parent = 0 );
    ~WelcomeWidget();

    void loadConfig( const KConfigGroup& c );
    void saveConfig( KConfigGroup c );

    class Display;

public Q_SLOTS:
    void slotMoreActions();

protected:
    void resizeEvent( QResizeEvent* );
    void showEvent( QShowEvent* );
    void mousePressEvent ( QMouseEvent* e );

private:
    void fixSize();

    MainWindow* m_mainWindow;
    Display* main;
};
}


namespace K3b {
class WelcomeWidget::Display : public QWidget
{
    Q_OBJECT

public:
    Display( WelcomeWidget* parent );
    ~Display();

    QSize minimumSizeHint() const;
    QSizePolicy sizePolicy () const;
    int heightForWidth ( int w ) const;

    void addAction( QAction* );
    void removeAction( QAction* );
    void removeButton( FlatButton* );
    void rebuildGui();
    void rebuildGui( const QList<QAction*>& );

Q_SIGNALS:
    void dropped( const KUrl::List& );

protected:
    void resizeEvent( QResizeEvent* );
    void paintEvent( QPaintEvent* );
    void dropEvent( QDropEvent* event );
    void dragEnterEvent( QDragEnterEvent* event );

private Q_SLOTS:
    void slotThemeChanged();

private:
    void repositionButtons();
    void updateBgPix();

    QTextDocument* m_header;
    QTextDocument* m_infoText;

    QSize m_buttonSize;
    int m_cols;
    int m_rows;

    QList<QAction*> m_actions;
    QList<FlatButton*> m_buttons;
    QMap<FlatButton*, QAction*> m_buttonMap;

    FlatButton* m_buttonMore;

    bool m_infoTextVisible;

    QPixmap m_bgPixmap;
    QImage m_bgImage;

    friend class K3b::WelcomeWidget;
};
}

#endif
