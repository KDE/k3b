/***************************************************************************
                          k3bfilmview.h  -  description
                             -------------------
    begin                : Fri Feb 22 2002
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

#ifndef K3BFILMVIEW_H
#define K3BFILMVIEW_H

#include <qwidget.h>
#include <qvaluelist.h>

class QString;
class QLabel;
class KProcess;
class KListView;
class K3bTcWrapper;
class K3bDvdContent;
/**
  *@author Sebastian Trueg
  */

class K3bFilmView : public QWidget  {
    Q_OBJECT
public: 
    K3bFilmView(QWidget *parent=0, const char *name=0);
    ~K3bFilmView();
    void setDevice( const QString& device );
    void show();
private:
    QString m_device;
    K3bTcWrapper *m_tcWrapper;
    KListView *m_chapterView;
    KListView *m_titleView;
    KListView *m_audioView;
    QLabel *m_input, *m_mode, *m_res, *m_aspect, *m_time;
    QLabel *m_video, *m_audio, *m_frames, *m_framerate;
    typedef QValueList<K3bDvdContent> DvdTitle;
    DvdTitle m_dvdTitles;

    void setupGui();

signals:
    void notSupportedDisc( const QString& device );

private slots:
    void slotDvdChecked( bool successful );
    void slotNotSupportedDisc();
    void slotTitleSelected( int row );
};

#endif
