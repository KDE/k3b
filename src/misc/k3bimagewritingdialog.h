/*
 *
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

class QSpinBox;
class QComboBox;
class QCheckBox;
class QLabel;
class KUrl;
class KUrlRequester;
class QDragEnterEvent;
class QDropEvent;
class K3ListView;
class Q3ListViewItem;
class QPoint;
class KComboBox;

namespace K3b {
    class WriterSelectionWidget;
    class DataModeWidget;
    class WritingModeWidget;
    class TempDirSelectionWidget;
    class ListView;
    class Iso9660;
    class CueFileParser;


    /**
     *@author Sebastian Trueg
     */
    class ImageWritingDialog : public InteractionDialog
    {
        Q_OBJECT

    public:
        ImageWritingDialog( QWidget* = 0 );
        ~ImageWritingDialog();

        void setImage( const KUrl& url );

    protected Q_SLOTS:
        void slotStartClicked();

        void slotMd5JobPercent( int );
        void slotMd5JobFinished( bool );
        void slotContextMenu( K3ListView*, Q3ListViewItem*, const QPoint& pos );

        void slotUpdateImage( const QString& );

    protected:
        void loadSettings( const KConfigGroup& );
        void saveSettings( KConfigGroup );

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
        void createIso9660InfoItems( Iso9660* );
        void createCdrecordCloneItems( const QString&, const QString& );
        void createCueBinItems( const QString&, const QString& );
        void createAudioCueItems( const CueFileParser& cp );
        int currentImageType();
        QString imagePath() const;

        WriterSelectionWidget* m_writerSelectionWidget;
        QCheckBox* m_checkDummy;
        QCheckBox* m_checkNoFix;
        QCheckBox* m_checkCacheImage;
        QCheckBox* m_checkVerify;
        DataModeWidget* m_dataModeWidget;
        WritingModeWidget* m_writingModeWidget;
        QSpinBox* m_spinCopies;

        KUrlRequester* m_editImagePath;
        KComboBox* m_comboRecentImages;
        QComboBox* m_comboImageType;

        ListView* m_infoView;
        TempDirSelectionWidget* m_tempDirSelectionWidget;

        class Private;
        Private* d;
    };
}

#endif
