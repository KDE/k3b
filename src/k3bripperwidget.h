/***************************************************************************
                          k3bripperwidget.h  -  description
                             -------------------
    begin                : Tue Mar 27 2001
    copyright            : (C) 2001 by Sebastian Trueg
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

#ifndef K3BRIPPERWIDGET_H
#define K3BRIPPERWIDGET_H

#include <qwidget.h>
#include <qvariant.h>
#include <qarray.h>
#include <qthread.h>

class K3bCddaCopy;
class K3bCdView;
class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class KComboBox;
class KListView;
class KProgress;
class QGroupBox;
class QLabel;
class QLineEdit;
class QListViewItem;
class QPushButton;
class QString;
class QStringList;
class QCloseEvent;

/**
  *@author Sebastian Trueg
  */

class K3bRipperWidget : public QWidget {
   Q_OBJECT

public: 
    K3bRipperWidget(QString device, K3bCdView *parent, const char *name=0);
    ~K3bRipperWidget();

    QGroupBox* GroupBox3;
    KComboBox* m_comboSource;
    QPushButton* m_buttonRefresh;
    KListView* m_viewTracks;
    QLabel* TextLabel2;
    QLineEdit* m_editRipPath;
    QPushButton* m_buttonStart;
    KProgress *m_progress;
    K3bCdView *m_cdview;
    long m_bytes;
    void addTrack(QListViewItem *item);
    void setTrackNumbers(QArray<int> tracks);
    void setFileList(QStringList files);

protected:
    QGridLayout* Form1Layout;
    QGridLayout* GroupBox3Layout;

    void closeEvent( QCloseEvent *e);

private:
    QString m_device;
    QArray<int> m_tracks;
    QStringList m_list;
    K3bCddaCopy *m_copy;
private slots:
    void rip();
    void waitForClose();
};

#endif
