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
#include <qvbox.h>
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
class QCheckBox;
class QListViewItem;
class KLineEdit;
class K3bCddb;
class K3bPatternParser;
/**
  *@author Sebastian Trueg
  */

class K3bRipperWidget : public QWidget {
   Q_OBJECT

public: 
    K3bRipperWidget(QString device, K3bCddb *cddb, K3bCdView *parent, const char *name=0);
    ~K3bRipperWidget();

    QGroupBox* GroupBox3;
    KComboBox* m_comboSource;
    QPushButton* m_buttonRefresh;
    KListView* m_viewTracks;
    QLabel* TextLabel2;
    QPushButton* m_buttonStart;
    QPushButton* m_buttonStaticDir;
    QPushButton* m_buttonPattern;
    KLineEdit *m_editStaticRipPath;
    KProgress *m_progress;
    K3bCdView *m_cdview;
    QCheckBox *m_useStatic;
    QCheckBox *m_usePattern;
    long m_bytes;
    void addTrack(QListViewItem *item );
    void setTrackNumbers(QArray<int> tracks);
    void setFileList(QStringList files);
    void setFilePatternList(QStringList );
    void setDirPatternList( QStringList  );
    void refresh();
    void init();
    void setData( QStringList files, QArray<int> tracks, QArray<long> size);
    void setReplacements(bool dir, QString sdir, bool file, QString sfile, bool mixed );


public slots:
    void waitForClose();
    void slotRippingFinished();

protected:
    QGridLayout* Form1Layout;
    QGridLayout* GroupBox3Layout;

    void closeEvent( QCloseEvent *e);

private:
    QString m_device;
    QArray<int> m_tracks;
    QArray<long> m_size;
    QStringList m_list;
    QStringList m_titles;
    QString m_album;
    QString m_testItemPattern;
    QLabel *m_labelSummaryName;
    QString m_editFile;
    QString m_editDir;
    bool m_finalClose;
    // default read from settings
    bool m_useCustomDir;
    bool m_useFilePattern;
    bool m_useConfigDirectoryPattern;
    // locally set by filenamepatterndialog
    bool m_spaceDir;
    bool m_spaceFile;
    bool m_parseMixed;

    QStringList m_filePatternList;
    QStringList m_dirPatternList;
    K3bCddaCopy *m_copy;
    K3bCddb *m_cddb;
    K3bPatternParser *m_parser;
    void setupGui();
    QString prepareDirectory( QListViewItem *item);
    QString getRealDirectory( int index, QListViewItem *item);

private slots:
    void rip();
    void useStatic();
    void usePattern();
    void showPatternDialog();
    void slotFindStaticDir();
};

#endif
