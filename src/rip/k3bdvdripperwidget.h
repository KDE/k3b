/* 
 *
 * $Id$
 * Copyright (C) 2002 Thomas Froescher Trueg <tfroescher@k3b.org>
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef K3BDVDRIPPERWIDGET_H
#define K3BDVDRIPPERWIDGET_H

#include <k3binteractiondialog.h>

class K3bDvdContent;
class QCheckBox;
class KURLRequester;
class QString;
class QLabel;
class K3bDvdRippingProcess;
class KDiskFreeSp;
class KProcess;
class K3bDvdFillDisplay;
class K3bDivxCodecData;
class K3bJobProgressDialog;
class K3bDvdCopy;
class KComboBox;
class K3bDvdExtraRipStatus;


/**
  *@author Thomas Froescher
  */
class K3bDvdRipperWidget : public K3bInteractionDialog
{
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
    //QToolButton* m_buttonStaticDir;
    KURLRequester *m_editStaticRipPath;
    long m_bytes;
    DvdTitle m_ripTitles;
    //K3bBurnProgressDialog *m_ripDialog;
    K3bDvdRippingProcess *m_ripProcess;
    //K3bDvdCopy *m_ripJob;
    int m_angle;
    QLabel *m_labelAngle;
    KComboBox *m_comboAngle;
    bool m_enoughSpace;
    double m_vobSize;
    double m_titleSize;
    bool m_supportSizeDetection;
    bool m_detectTitleSizeDone;
    bool m_startEncoding;
    bool m_openEncoding;
    //K3bDivxCodecData *m_data;

    K3bDvdCopy *m_ripJob;
    K3bJobProgressDialog *m_ripDialog;
    K3bDvdExtraRipStatus *m_ripStatus;
    
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
    void slotSetDependDirs( const QString& );
    void slotFreeTempSpace( const QString & mountPoint, unsigned long kBSize,
        unsigned long kBUsed, unsigned long kBAvail );
    void slotParseError( KProcess *p, char *text, int len );
    void slotCheckOpenEncoding(int);
    void slotOpenEncoding( bool );
    void slotCheckStartEncoding(int);
    void slotStartEncoding( bool );
    
    void slotLoadK3bDefaults();
    void slotLoadUserDefaults();
    void slotSaveUserDefaults();
};

#endif
