/*
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/


#ifndef _K3B_WELCOME_WIDGET_H_
#define _K3B_WELCOME_WIDGET_H_

#include <QList>
#include <QMap>
#include <QImage>
#include <QPixmap>
#include <QWidget>

class KConfigGroup;
class QDragEnterEvent;
class QDropEvent;
class QMouseEvent;
class QPaintEvent;
class QResizeEvent;
class QTextDocument;


namespace K3b {

    class FlatButton;
    class MainWindow;

    class WelcomeWidget : public QWidget
    {
        Q_OBJECT

    public:
        explicit WelcomeWidget( MainWindow* mainWindow, QWidget* parent = 0 );
        ~WelcomeWidget() override;

        void loadConfig( const KConfigGroup& c );
        void saveConfig( KConfigGroup c );

        int heightForWidth( int width ) const override;

        void addAction( QAction* );
        void removeAction( QAction* );
        void removeButton( FlatButton* );
        void rebuildGui();
        void rebuildGui( const QList<QAction*>& );

    protected:
        bool event( QEvent* event ) override;
        void resizeEvent( QResizeEvent* ) override;
        void paintEvent( QPaintEvent* ) override;
        void dropEvent( QDropEvent* event ) override;
        void dragEnterEvent( QDragEnterEvent* event ) override;
        void mousePressEvent ( QMouseEvent* e ) override;

    private Q_SLOTS:
        void slotThemeChanged();
        void slotMoreActions();

    private:
        void repositionButtons();
        void updateBgPix();
        
        MainWindow* m_mainWindow;

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
    };

} // namespace K3b

#endif
