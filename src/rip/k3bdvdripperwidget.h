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
class QToolButton;
class QCheckBox;
class KLineEdit;
class QString;
class QLabel;
class K3bBurnProgressDialog;
class K3bDvdRippingProcess;
class K3bDvdCopy;
class KDiskFreeSp;
class KProcess;
class K3bDvdFillDisplay;
class K3bDivxCodecData;
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

public slots:
    void slotEncodingFinished( bool );
protected:
    void closeEvent( QCloseEvent *e);

private:
    QString m_device;
    QString m_editDir;
    bool m_finalClose;
    QLabel *m_labelSummaryName;
    QLabel *m_hardDiskSpace;
    QCheckBox *m_checkOpenEncoding;
    QCheckBox *m_checkStartEncoding;
    QToolButton* m_buttonStaticDir;
    KLineEdit *m_editStaticRipPath;
    long m_bytes;
    DvdTitle m_ripTitles;
    K3bBurnProgressDialog *m_ripDialog;
    K3bDvdRippingProcess *m_ripProcess;
    K3bDvdCopy *m_ripJob;
    bool m_enoughSpace;
    double m_vobSize;
    double m_titleSize;
    bool m_supportSizeDetection;
    bool m_detectTitleSizeDone;
    bool m_startEncoding;
    bool m_openEncoding;
    //K3bDivxCodecData *m_data;


    K3bDvdFillDisplay *m_fillDisplay;
    void setupGui();
    bool createDirs();
    bool createDirectory( const QString& );
    void checkSize(  );
    void prepareDataForEncoding();
    void openEncodingDialog();
private slots:
    void rip();
    void slotFindStaticDir();
    void slotRipJobDeleted();
    void slotSetDependDirs( const QString& );
    void slotFreeTempSpace( const QString & mountPoint, unsigned long kBSize,
        unsigned long kBUsed, unsigned long kBAvail );
    void slotParseError( KProcess *p, char *text, int len );
    void slotCheckOpenEncoding(int);
    void slotOpenEncoding( bool );
    void slotCheckStartEncoding(int);
    void slotStartEncoding( bool );
};

#endif
