/***************************************************************************
                          k3bsetup2page.h
                                   -
                       Abstract base class for all pages
                             -------------------
    begin                : Sun Aug 25 13:19:59 CEST 2002
    copyright            : (C) 2002 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef K3BSETUP2_PAGE_H
#define K3BSETUP2_PAGE_H

#include <qwidget.h>

class KConfig;
class K3bListView;
class QPixmap;
class QLabel;
class K3bSetup2PixmapHeader;


class K3bSetup2Page : public QWidget
{
  Q_OBJECT

 public:
  K3bSetup2Page( K3bListView*, const QString& header, QWidget* parent = 0, const char* name = 0 );
  virtual ~K3bSetup2Page();

  void setPixmap( const QPixmap& );

 public slots:
  virtual void load( KConfig* );
  virtual bool save( KConfig* );

 protected:
  QWidget* mainWidget() const { return m_mainWidget; }
  K3bListView* taskView() const { return m_taskView; }

 private:
  QWidget* m_mainWidget;
  K3bListView* m_taskView;
  QLabel* m_pixmapLabel;
  K3bSetup2PixmapHeader* m_headerLabel;
};

#endif
