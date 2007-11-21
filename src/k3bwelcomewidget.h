/* 
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef _K3B_WELCOME_WIDGET_H_
#define _K3B_WELCOME_WIDGET_H_

#include <q3scrollview.h>
#include <q3ptrlist.h>
#include <qmap.h>
#include <qimage.h>
//Added by qt3to4:
#include <QPixmap>
#include <QResizeEvent>
#include <QShowEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QPaintEvent>

#include <kurl.h>
#include <kaction.h>
#include <kconfiggroup.h>

class K3bMainWindow;
class QDropEvent;
class QDragEnterEvent;
class K3bFlatButton;
class QPaintEvent;
class QResizeEvent;
class Q3SimpleRichText;
class KConfigBase;
class QMouseEvent;
class QShowEvent;


class K3bWelcomeWidget : public Q3ScrollView
{
  Q_OBJECT

 public:
  K3bWelcomeWidget( K3bMainWindow*, QWidget* parent = 0 );
  ~K3bWelcomeWidget();

  void loadConfig( const KConfigGroup& c );
  void saveConfig( KConfigGroup& c );

  class Display;

 public slots:
  void slotMoreActions();

 protected:
  void resizeEvent( QResizeEvent* );
  void showEvent( QShowEvent* );
  void contentsMousePressEvent( QMouseEvent* e );

 private:
  void fixSize();

  K3bMainWindow* m_mainWindow;
  Display* main;
};


class K3bWelcomeWidget::Display : public QWidget
{
  Q_OBJECT

 public:
  Display( K3bWelcomeWidget* parent );
  ~Display();

  QSize minimumSizeHint() const;
  QSizePolicy sizePolicy () const;
  int heightForWidth ( int w ) const;

  void addAction( KAction* );
  void removeAction( KAction* );
  void removeButton( K3bFlatButton* );
  void rebuildGui();
  void rebuildGui( const QList<QAction*>& );

 signals:
  void dropped( const KUrl::List& );

 protected:
  void resizeEvent( QResizeEvent* );
  void paintEvent( QPaintEvent* );
  void dropEvent( QDropEvent* event );
  void dragEnterEvent( QDragEnterEvent* event );

 private slots:
  void slotThemeChanged();

 private:
  void repositionButtons();
  void updateBgPix();

  Q3SimpleRichText* m_header;
  Q3SimpleRichText* m_infoText;

  QSize m_buttonSize;
  int m_cols;
  int m_rows;

  QList<QAction*> m_actions;
  Q3PtrList<K3bFlatButton> m_buttons;
  QMap<K3bFlatButton*, QAction*> m_buttonMap;

  K3bFlatButton* m_buttonMore;

  bool m_infoTextVisible;

  QPixmap m_bgPixmap;
  QImage m_bgImage;

  friend class K3bWelcomeWidget;
};

#endif
