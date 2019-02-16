/*
 *
 * Copyright (C) 2003-2010 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2010 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef _K3B_JOB_PROGRESSDIALOG_H_
#define _K3B_JOB_PROGRESSDIALOG_H_


#include "k3bdebuggingoutputfile.h"
#include "k3bdebuggingoutputcache.h"
#include "k3bjobhandler.h"

#include <QElapsedTimer>
#include <QDialog>

class KSqueezedTextLabel;
class QCloseEvent;
class QFrame;
class QGridLayout;
class QKeyEvent;
class QLabel;
class QProgressBar;
class QShowEvent;
class QElapsedTimer;

namespace K3b {
    class Job;
    class ThemedLabel;

    class JobProgressDialog : public QDialog, public JobHandler
    {
        Q_OBJECT

    public:
        explicit JobProgressDialog( QWidget* parent = 0,
                           bool showSubProgress = true );
        ~JobProgressDialog() override;

        virtual void setJob( Job* job );
        void setExtraInfo( QWidget *extra );

        /**
         * This will show the dialog and then start the given job or
         * if job == 0 the job set with setJob
         * Use instead of exec()
         */
        int startJob( Job* job = 0 );

        QSize sizeHint() const override;

        /**
         * @reimplemented from JobHandler
         */
        Device::MediaType waitForMedium( Device::Device*,
                                         Device::MediaStates mediaState = Device::STATE_EMPTY,
                                         Device::MediaTypes mediaType = Device::MEDIA_WRITABLE_CD,
                                         const K3b::Msf& minMediaSize = K3b::Msf(),
                                         const QString& message = QString() ) override;

        /**
         * @reimplemented from JobHandler
         */
        bool questionYesNo( const QString& text,
                            const QString& caption = QString(),
                            const KGuiItem& buttonYes = KStandardGuiItem::yes(),
                            const KGuiItem& buttonNo = KStandardGuiItem::no() ) override;

        /**
         * reimplemented from JobHandler
         */
        void blockingInformation( const QString& text,
                                  const QString& caption = QString() ) override;

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
         * \reimpl from QDialog
         */
        void reject() override;

	void slotShowDebuggingOutput();

        void slotProgress( int );

        virtual void slotThemeChanged();

    protected:
        bool event( QEvent* event ) override;
        void showEvent( QShowEvent* e ) override;
        void closeEvent( QCloseEvent* e ) override;
        void keyPressEvent( QKeyEvent* e ) override;

        void setupGUI();

        ThemedLabel* m_labelJob;
        ThemedLabel* m_labelJobDetails;
        ThemedLabel* m_labelTask;
        QLabel* m_labelRemainingTime;
        QLabel* m_labelElapsedTime;
        KSqueezedTextLabel* m_labelSubTask;
        QLabel* m_labelSubProcessedSize;
        QProgressBar* m_progressSubPercent;
        QLabel* m_labelProcessedSize;
        QProgressBar* m_progressPercent;
        QFrame* m_frameExtraInfo;
        ThemedLabel* m_pixLabel;
        QPushButton* m_cancelButton;
        QPushButton* m_showDbgOutButton;
        QPushButton* m_closeButton;

        QGridLayout* m_frameExtraInfoLayout;

    private:
        class Private;
        Private* d;

        Job* m_job;
        QElapsedTimer m_timer;
        qint64 m_lastProgressUpdateTime;

        DebuggingOutputFile m_logFile;
        DebuggingOutputCache m_logCache;

        bool m_bCanceled;

        QString m_plainCaption;
    };
}

#endif
