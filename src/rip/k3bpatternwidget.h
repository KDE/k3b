/***************************************************************************
                          k3bpatternwidget.h  -  description
                             -------------------
    begin                : Sun Dec 2 2001
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

#ifndef K3BPATTERNWIDGET_H
#define K3BPATTERNWIDGET_H

#include <qwidget.h>
#include <qvbox.h>
#include <kdialogbase.h>
class QCheckBox;
class QString;
class QLabel;
class QListViewItem;
class KComboBox;
class QGroupBox;
class QRadioButton;
class QVButtonGroup;
class KLineEdit;
class KioTree;
class KURL;

/**
  *@author Sebastian Trueg
  */

class K3bPatternWidget : public QWidget  {
   Q_OBJECT
public:
    K3bPatternWidget(QWidget *parent=0, const char *name=0);
    ~K3bPatternWidget();
    void init( QString& album, QListViewItem *item=0 );
    void apply();
    void readSettings();
    QString getFilePattern();
    QString getDirPattern();
    bool getSpaceReplaceDir();
    bool getSpaceReplaceFile();
    bool getParseMixed();
    QString getReplaceCharFile();
    QString getReplaceCharDir();
private:
    //QCheckBox *m_pattern;
    //QCheckBox *m_usePattern;
    QGroupBox *m_groupPatternDir;
    QLabel *m_labelSummaryDirectory;
    QRadioButton *m_radioDir1Artist;
    QRadioButton *m_radioDir1Album;
    QRadioButton *m_radioDir1None;
    QRadioButton *m_radioDir2Artist;
    QRadioButton *m_radioDir2Album;
    QRadioButton *m_radioDir2None;
    //QCheckBox *m_alwaysUseDir;
    QCheckBox *m_spaceReplaceDir;
    KioTree *m_kioTree;
    QVButtonGroup *m_dirs1;
    QVButtonGroup *m_dirs2;
    QString m_basePath;
    // filename pattern gui
    //QCheckBox *m_alwaysUseFile;
    QCheckBox *m_spaceReplaceFile;
    QCheckBox *m_checkSlashFile;
    KComboBox *m_comboFile1;
    KComboBox *m_comboFile2;
    KComboBox *m_comboFile3;
    QString m_testNumber;
    QString m_testArtist;
    QString m_testAlbum;
    QString m_testTitle;
    QString m_finalPatternFile[6];
    QString m_finalPatternDir[3];
    QLabel *m_labelSummaryFilename;
    QStringList m_list;
    KLineEdit *m_editSpaceFile1;
    KLineEdit *m_editSpaceFile2;
    KLineEdit *m_editDir;
    KLineEdit *m_editFile;
    QGroupBox *m_groupPatternFile;
    bool m_useCddb;
    void initCddb();
    void setup();
    void setFinalPatternFile(int, int);
    void showFinalFilePattern();
    int searchComboIndex(QStringList, QString, KComboBox* );
    QString getDirPattern( QButtonGroup *bg);

private slots:
    void slotFile1(int index );
    void slotFile2(int index );
    void slotFile3(int index );
    void slotFileCustom1(const QString &);
    void slotFileCustom2(const QString &);
    void slotFileCustom3(const QString &);
    void slotFileSpace1(const QString &);
    void slotFileSpace2(const QString &);
    void slotFileReplaceSpace();
    void slotEnableFilePattern(int state);
    void slotShowFinalDirPattern( );
    void slotDirTree( const KURL& );
	
};

#endif
