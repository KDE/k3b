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


#ifndef _K3B_JOB_PROGRESSDIALOG_H_
#define _K3B_JOB_PROGRESSDIALOG_H_

#include <kdialog.h>

#include "k3bdebuggingoutputfile.h"
#include "k3bdebuggingoutputcache.h"

#include <k3bjobhandler.h>

#include <qdatetime.h>
#include <qfile.h>
#include <QGridLayout>
#include <QLabel>
#include <QFrame>
#include <QKeyEvent>
#include <QCloseEvent>

class K3ListView;
class QFrame;
class QLabel;
class QProgressBar;
class QTimer;
class KSqueezedTextLabel;
class QCloseEvent;
class QKeyEvent;

namespace K3b {
    class Job;
    class JobProgressOSD;
    class ThemedLabel;

    class JobProgressDialog : public KDialog, public JobHandler
    {
        Q_OBJECT

    public:
        JobProgressDialog( QWidget* parent = 0,
                           bool showSubProgress = true );
        virtual ~JobProgressDialog();

        virtual void setJob( Job* job );
        void setExtraInfo( QWidget *extra );

        /**
         * reimplemented for internal reasons
         */
        void show();

        /**
         * reimplemented for internal reasons
         */
        void hide();

        /**
         * This will show the dialog and then start the given job or
         * if job == 0 the job set with setJob
         * Use instead of exec()
         */
        int startJob( Job* job = 0 );

        QSize sizeHint() const;

        /**
         * @reimplemented from JobHandler
         */
        int waitForMedia( Device::Device*,
                          Device::MediaStates mediaState = Device::STATE_EMPTY,
                          Device::MediaTypes mediaType = Device::MEDIA_WRITABLE_CD,
                          const QString& message = QString() );

        /**
         * @reimplemented from JobHandler
         */
        bool questionYesNo( const QString& text,
                            const QString& caption = QString(),
                            const QString& yesText = QString(),
                            const QString& noText = QString() );

        /**
         * reimplemented from JobHandler
         */
        void blockingInformation( const QString& text,
                                  const QString& caption = QString() );

    protected Q_SLOTS:
        virtual void slotProcessedSize( int processed, int size );
        virtual void slotProcessedSubSize( int processed, int size );
        virtual void slotInfoMessage( const QString& infoString, int type );
        virtual void slotDebuggingOutput( const QString&, const QString& );
        virtual void slotNewSubTask(const QString& name);
        virtual void slotNewTask(const QString& name);
        virtual void slotFinished(bool);
        virtual void slotCanceled();
        virtual void slotStarted();

        /**
         * \reimpl from KDialog
         */
        void slotButtonClicked( int button );
        void slotUpdateTime();
        void slotShowDebuggingOutput();

        void slotProgress( int );

        virtual void slotThemeChanged();

    protected:
        void closeEvent( QCloseEvent* );
        void keyPressEvent( QKeyEvent* e );

        void setupGUI();

        ThemedLabel* m_labelJob;
        ThemedLabel* m_labelJobDetails;
        K3ListView* m_viewInfo;
        ThemedLabel* m_labelTask;
        ThemedLabel* m_labelElapsedTime;
        KSqueezedTextLabel* m_labelSubTask;
        QLabel* m_labelSubProcessedSize;
        QProgressBar* m_progressSubPercent;
        QLabel* m_labelProcessedSize;
        QProgressBar* m_progressPercent;
        QFrame* m_frameExtraInfo;
        ThemedLabel* m_pixLabel;

        QGridLayout* m_frameExtraInfoLayout;

    private:
        class Private;
        Private* d;

        Job* m_job;
        QTimer* m_timer;
        QDateTime m_startTime;
        QDateTime m_lastProgressUpdateTime;

        DebuggingOutputFile m_logFile;
        DebuggingOutputCache m_logCache;

        bool m_bCanceled;

        QString m_plainCaption;

        JobProgressOSD* m_osd;
    };
}

#endif
