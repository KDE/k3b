/***************************************************************************
                          k3bdvdripperwidget.h  -  description
                             -------------------
    begin                : Sun Mar 3 2002
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

#ifndef K3BDVDRIPPERWIDGET_H
#define K3BDVDRIPPERWIDGET_H

#include <kdialogbase.h>

class K3bDvdContent;
class QPushButton;
class QCheckBox;
class KLineEdit;
class QString;
class QLabel;
class K3bBurnProgressDialog;
class K3bDvdRippingProcess;
class K3bDvdCopy;
/**
  *@author Sebastian Trueg
  */

class K3bDvdRipperWidget : public KDialogBase  {
   Q_OBJECT
public: 
    K3bDvdRipperWidget(const QString& device, QWidget *parent=0, const char *name=0);
    ~K3bDvdRipperWidget();
    typedef QValueList<K3bDvdContent> DvdTitle;
    void init( const DvdTitle& titles);

protected:
    void closeEvent( QCloseEvent *e);

private:
    QString m_device;
    QString m_editDir;
    bool m_finalClose;
    QLabel *m_labelSummaryName;
    QCheckBox *m_normalize;
    QPushButton* m_buttonStart;
    QPushButton* m_buttonStaticDir;
    QPushButton* m_buttonStaticDirVob;
    QPushButton* m_buttonStaticDirTmp;
    QPushButton* m_buttonPattern;
    KLineEdit *m_editStaticRipPath;
    KLineEdit *m_editStaticRipPathVob;
    KLineEdit *m_editStaticRipPathTmp;
    long m_bytes;
    DvdTitle m_ripTitles;
    K3bBurnProgressDialog *m_ripDialog;
    K3bDvdRippingProcess *m_ripProcess;
    K3bDvdCopy *m_ripJob;
    void setupGui();

private slots:
    void rip();
    void slotFindStaticDir();
    void slotFindStaticDirVob();
    void slotFindStaticDirTmp();
    void slotRipJobDeleted();
};

#endif
