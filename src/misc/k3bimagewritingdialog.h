/* 
 *
 * $Id$
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef _K3B_IMAGEWRITINGDIALOG_H_
#define _K3B_IMAGEWRITINGDIALOG_H_

#include <k3binteractiondialog.h>

class QCheckBox;
class K3bWriterSelectionWidget;
class QLabel;
class KUrl;
class K3bDataModeWidget;
class K3bWritingModeWidget;
class K3bTempDirSelectionWidget;
class KUrlRequester;
class K3bListView;
class QSpinBox;
class QComboBox;
class K3bIso9660;
class K3bCueFileParser;
class QDragEnterEvent;
class QDropEvent;
class K3ListView;
class Q3ListViewItem;
class QPoint;
class KComboBox;


/**
  *@author Sebastian Trueg
  */
class K3bImageWritingDialog : public K3bInteractionDialog
{
    Q_OBJECT;

public: 
    K3bImageWritingDialog( QWidget* = 0 );
    ~K3bImageWritingDialog();

    void setImage( const KUrl& url );

protected Q_SLOTS:
    void slotStartClicked();

    void slotMd5JobPercent( int );
    void slotMd5JobFinished( bool );
    void slotContextMenu( K3ListView*, Q3ListViewItem*, const QPoint& pos );

    void slotUpdateImage( const QString& );

protected:
    void loadUserDefaults( const KConfigGroup& );
    void saveUserDefaults( KConfigGroup& );
    void loadK3bDefaults();

    void calculateMd5Sum( const QString& );
    void dragEnterEvent( QDragEnterEvent* );
    void dropEvent( QDropEvent* );

    void init();

    void toggleAll();

private:
    enum {
        IMAGE_UNKNOWN,
        IMAGE_ISO,
        IMAGE_CUE_BIN,
        IMAGE_AUDIO_CUE,
        IMAGE_CDRDAO_TOC,
        IMAGE_CDRECORD_CLONE
    };

    void setupGui();
    void createIso9660InfoItems( K3bIso9660* );
    void createCdrecordCloneItems( const QString&, const QString& );
    void createCueBinItems( const QString&, const QString& );
    void createAudioCueItems( const K3bCueFileParser& cp );
    int currentImageType();
    QString imagePath() const;

    K3bWriterSelectionWidget* m_writerSelectionWidget;
    QCheckBox* m_checkDummy;
    QCheckBox* m_checkNoFix;
    QCheckBox* m_checkCacheImage;
    QCheckBox* m_checkVerify;
    K3bDataModeWidget* m_dataModeWidget;
    K3bWritingModeWidget* m_writingModeWidget;
    QSpinBox* m_spinCopies;

    KUrlRequester* m_editImagePath;
    KComboBox* m_comboRecentImages;
    QComboBox* m_comboImageType;

    K3bListView* m_infoView;
    K3bTempDirSelectionWidget* m_tempDirSelectionWidget;

    class Private;
    Private* d;
};

#endif
