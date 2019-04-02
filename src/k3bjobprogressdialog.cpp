/*
 *
 * Copyright (C) 2003-2010 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C)      2010 Michal Malek <michalm@jabster.pl>
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


#include "k3bjobprogressdialog.h"
#include "k3bapplication.h"
#include "k3bemptydiscwaiter.h"
#include "k3bdebuggingoutputdialog.h"
#include "k3bjobinterface.h"
#include "k3bthemedlabel.h"
#include "k3b.h"
#include "k3bjob.h"
#include "k3bdevice.h"
#include "k3bdevicemanager.h"
#include "k3bdeviceglobals.h"
#include "k3bglobals.h"
#include "k3bkjobbridge.h"
#include "k3bstdguiitems.h"
#include "k3bversion.h"
#include "k3bthememanager.h"

#include <KColorScheme>
#include <KConfig>
#include <KSharedConfig>
#include <KJobTrackerInterface>
#include <KFormat>
#include <KNotification>
#include <KSqueezedTextLabel>
#include <KLocalizedString>
#include <KIO/Global>
#include <KMessageBox>

#include <QDebug>
#include <QDateTime>
#include <QString>
#include <QCloseEvent>
#include <QIcon>
#include <QFont>
#include <QKeyEvent>
#include <QDialogButtonBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollBar>
#include <QTreeWidget>
#include <QVBoxLayout>


class K3b::JobProgressDialog::Private
{
public:
    int lastProgress;

    QFrame* headerFrame;
    QFrame* progressHeaderFrame;
    QTreeWidget* viewInfo;
};



K3b::JobProgressDialog::JobProgressDialog( QWidget* parent,
                                           bool showSubProgress )
    : QDialog( parent )
{
    d = new Private;
    setupGUI();

    if( !showSubProgress ) {
        m_progressSubPercent->hide();
    }

    m_job = 0;
}


/*
 *  Destroys the object and frees any allocated resources
 */
K3b::JobProgressDialog::~JobProgressDialog()
{
    qDebug();
    delete d;
}


void K3b::JobProgressDialog::setupGUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout( this );

    // header
    // ------------------------------------------------------------------------------------------
    QFrame* headerParentFrame = new QFrame( this );
    headerParentFrame->setFrameShape( QFrame::StyledPanel );
    headerParentFrame->setFrameShadow( QFrame::Sunken );
    headerParentFrame->setLineWidth( 1 );
    headerParentFrame->setLayout( new QVBoxLayout() );
    headerParentFrame->layout()->setSpacing(0);
    headerParentFrame->layout()->setMargin(0);

    d->headerFrame = new QFrame( headerParentFrame );
    d->headerFrame->setFrameStyle( QFrame::NoFrame );
    d->headerFrame->setAutoFillBackground( true );
    headerParentFrame->layout()->addWidget( d->headerFrame );

    QHBoxLayout* headerLayout = new QHBoxLayout( d->headerFrame );
    headerLayout->setContentsMargins( 0, 0, 0, 0 );
    headerLayout->setSpacing( 0 );
    m_pixLabel = new K3b::ThemedLabel( d->headerFrame );
    headerLayout->addWidget( m_pixLabel );

    m_labelJob = new K3b::ThemedLabel( d->headerFrame );
    //TODO fix me
    //m_labelJob->setMinimumVisibleText( 40 );
    QFont m_labelJob_font( m_labelJob->font() );
    m_labelJob_font.setPointSize( m_labelJob_font.pointSize() + 2 );
    m_labelJob_font.setBold( true );
    m_labelJob->setFont( m_labelJob_font );
    m_labelJob->setAlignment( Qt::AlignVCenter | Qt::AlignRight  );

    m_labelJobDetails = new K3b::ThemedLabel( d->headerFrame );
    m_labelJobDetails->setAlignment( Qt::AlignVCenter | Qt::AlignRight  );

    QVBoxLayout* jobLabelsLayout = new QVBoxLayout;
    jobLabelsLayout->addWidget( m_labelJob );
    jobLabelsLayout->addWidget( m_labelJobDetails );
    jobLabelsLayout->setContentsMargins( -1, -1, 10, -1 );
    headerLayout->addLayout( jobLabelsLayout );
    headerLayout->setAlignment( jobLabelsLayout, Qt::AlignVCenter );

    mainLayout->addWidget( headerParentFrame );
    // ------------------------------------------------------------------------------------------

    d->viewInfo = new QTreeWidget( this );
    d->viewInfo->setAllColumnsShowFocus( true );
    d->viewInfo->setHeaderHidden( true );
    d->viewInfo->setSortingEnabled( false );
    d->viewInfo->setRootIsDecorated( false );
    d->viewInfo->setSelectionMode( QAbstractItemView::NoSelection );
    d->viewInfo->setFocusPolicy( Qt::NoFocus );
    mainLayout->addWidget( d->viewInfo, 1 );


    // progress header
    // ------------------------------------------------------------------------------------------
    QFrame* progressHeaderParentFrame = new QFrame( this );
    progressHeaderParentFrame->setFrameShape( QFrame::StyledPanel );
    progressHeaderParentFrame->setFrameShadow( QFrame::Sunken );
    progressHeaderParentFrame->setLineWidth( 1 );
    progressHeaderParentFrame->setLayout( new QVBoxLayout() );
    progressHeaderParentFrame->layout()->setSpacing(0);
    progressHeaderParentFrame->layout()->setMargin(0);

    d->progressHeaderFrame = new QFrame( progressHeaderParentFrame );
    d->progressHeaderFrame->setFrameStyle( QFrame::NoFrame );
    d->progressHeaderFrame->setAutoFillBackground( true );
    progressHeaderParentFrame->layout()->addWidget( d->progressHeaderFrame );

    QHBoxLayout* progressHeaderLayout = new QHBoxLayout( d->progressHeaderFrame );
    progressHeaderLayout->setContentsMargins( 0, 0, 0, 0 );
    progressHeaderLayout->setSpacing( 0 );

    m_labelTask = new K3b::ThemedLabel( d->progressHeaderFrame );
    QFont m_labelTask_font( m_labelTask->font() );
    m_labelTask_font.setPointSize( m_labelTask_font.pointSize() + 2 );
    m_labelTask_font.setBold( true );
    m_labelTask->setFont( m_labelTask_font );

    m_labelRemainingTime = new QLabel( d->progressHeaderFrame );
    m_labelElapsedTime = new QLabel( d->progressHeaderFrame );

    QVBoxLayout* jobProgressLayout = new QVBoxLayout( d->progressHeaderFrame );
    jobProgressLayout->addWidget( m_labelTask );
    jobProgressLayout->addWidget( m_labelRemainingTime );
    jobProgressLayout->addWidget( m_labelElapsedTime );
    jobProgressLayout->setContentsMargins( 10, -1, -1, -1 );

    progressHeaderLayout->addLayout( jobProgressLayout );
    progressHeaderLayout->setAlignment( jobProgressLayout, Qt::AlignVCenter );
    progressHeaderLayout->addWidget( new K3b::ThemedLabel( K3b::Theme::PROGRESS_RIGHT, d->progressHeaderFrame ) );
    mainLayout->addWidget( progressHeaderParentFrame );
    // ------------------------------------------------------------------------------------------

    QHBoxLayout* layout3 = new QHBoxLayout;

    m_labelSubTask = new KSqueezedTextLabel( this );
    m_labelSubTask->setTextElideMode( Qt::ElideRight );
    layout3->addWidget( m_labelSubTask );

    m_labelSubProcessedSize = new QLabel( this );
    m_labelSubProcessedSize->setAlignment( Qt::AlignVCenter | Qt::AlignRight );
    layout3->addWidget( m_labelSubProcessedSize );
    mainLayout->addLayout( layout3 );

    m_progressSubPercent = new QProgressBar( this );
    mainLayout->addWidget( m_progressSubPercent );

    QHBoxLayout* layout4 = new QHBoxLayout;

    QLabel* textLabel5 = new QLabel( i18n("Overall progress:"), this );
    layout4->addWidget( textLabel5 );

    m_labelProcessedSize = new QLabel( this );
    m_labelProcessedSize->setAlignment( Qt::AlignVCenter | Qt::AlignRight );
    layout4->addWidget( m_labelProcessedSize );
    mainLayout->addLayout( layout4 );

    m_progressPercent = new QProgressBar( this );
    mainLayout->addWidget( m_progressPercent );

    m_frameExtraInfo = new QFrame( this );
    m_frameExtraInfo->setFrameShape( QFrame::NoFrame );
    m_frameExtraInfo->setFrameShadow( QFrame::Raised );
    m_frameExtraInfoLayout = new QGridLayout( m_frameExtraInfo );
    m_frameExtraInfoLayout->setContentsMargins( 0, 0, 0,0 );
    mainLayout->addWidget( m_frameExtraInfo );

    QDialogButtonBox* buttonBox = new QDialogButtonBox( this );
    m_cancelButton = buttonBox->addButton( QDialogButtonBox::Cancel );
    connect( m_cancelButton, SIGNAL(clicked()), this, SLOT(reject()) );

    m_showDbgOutButton = buttonBox->addButton( i18n("Show Debugging Output"), QDialogButtonBox::NoRole );
    connect( m_showDbgOutButton, SIGNAL(clicked()), this, SLOT(slotShowDebuggingOutput()) );

    m_closeButton = buttonBox->addButton( QDialogButtonBox::Close );
    connect( m_closeButton, SIGNAL(clicked()), this, SLOT(accept()) );

    mainLayout->addWidget( buttonBox );

    m_pixLabel->setThemePixmap( K3b::Theme::PROGRESS_WORKING );

    slotThemeChanged();

    if (k3bappcore) {
        connect( k3bappcore->themeManager(), SIGNAL(themeChanged()),
                 this, SLOT(slotThemeChanged()) );
    }
}


void K3b::JobProgressDialog::setExtraInfo( QWidget *extra )
{
    extra->setParent( m_frameExtraInfo );
    m_frameExtraInfoLayout->addWidget( extra, 0, 0 );
}


bool K3b::JobProgressDialog::event( QEvent *event )
{
    if( event->type() == QEvent::StyleChange ) {
        slotThemeChanged();
    }
    return QDialog::event( event );
}


void K3b::JobProgressDialog::showEvent( QShowEvent* e )
{
    if( !e->spontaneous() ) {
        if( KConfigGroup( KSharedConfig::openConfig(), "General Options" ).readEntry( "hide main window while writing", false ) ) {
            k3bappcore->k3bMainWindow()->hide();
        }
    }
    QDialog::showEvent( e );
}


void K3b::JobProgressDialog::closeEvent( QCloseEvent* e )
{
    if (m_closeButton->isVisible()) {
        QDialog::closeEvent(e);
        k3bappcore->k3bMainWindow()->show();

        if (!m_plainCaption.isEmpty())
            k3bappcore->k3bMainWindow()->setPlainCaption(m_plainCaption);

        accept();
    } else
        e->ignore();
}


void K3b::JobProgressDialog::slotProcessedSize( int processed, int size )
{
//#if KDE_IS_VERSION( 4, 3, 80 )
    m_labelProcessedSize->setText( i18nc( "%1 and %2 are byte sizes formatted via KFormat::formatByteSize",
                                          "%1 of %2",
                                          KFormat().formatByteSize( ( double )( ( qulonglong )processed*1024ULL*1024ULL ),
                                                                    1,
                                                                    KFormat::DefaultBinaryDialect,
                                                                    KFormat::UnitMegaByte ),
                                          KFormat().formatByteSize( ( double )( ( qulonglong )size*1024ULL*1024ULL ),
                                                                    1,
                                                                    KFormat::DefaultBinaryDialect,
                                                                    KFormat::UnitMegaByte ) ) );
//#else
//    m_labelProcessedSize->setText( i18nc( "%1 and %2 are byte sizes formatted via KFormat::formatByteSize",
//                                          "%1 of %2",
//                                          KFormat().formatByteSize( ( double )( ( qulonglong )processed*1024ULL*1024ULL ) ),
//                                          KFormat().formatByteSize( ( double )( ( qulonglong )size*1024ULL*1024ULL ) ) ) );
//#endif
}


void K3b::JobProgressDialog::slotProcessedSubSize( int processedTrackSize, int trackSize )
{
//#if KDE_IS_VERSION( 4, 3, 80 )
    m_labelSubProcessedSize->setText( i18nc( "%1 and %2 are byte sizes formatted via KFormat::formatByteSize",
                                             "%1 of %2",
                                             KFormat().formatByteSize( ( double )( ( qulonglong )processedTrackSize*1024ULL*1024ULL ),
                                                                       1,
                                                                       KFormat::DefaultBinaryDialect,
                                                                       KFormat::UnitMegaByte ),
                                             KFormat().formatByteSize( ( double )( ( qulonglong )trackSize*1024ULL*1024ULL ),
                                                                       1,
                                                                       KFormat::DefaultBinaryDialect,
                                                                       KFormat::UnitMegaByte ) ) );
//#else
//    m_labelSubProcessedSize->setText( i18nc( "%1 and %2 are byte sizes formatted via KFormat::formatByteSize",
//                                             "%1 of %2",
//                                             KFormat().formatByteSize( ( double )( ( qulonglong )processedTrackSize*1024ULL*1024ULL ) ),
//                                             KFormat().formatByteSize( ( double )( ( qulonglong )trackSize*1024ULL*1024ULL ) ) ) );
//#endif
}


void K3b::JobProgressDialog::slotInfoMessage( const QString& infoString, int type )
{
    QTreeWidgetItem* currentInfoItem = new QTreeWidgetItem( d->viewInfo );
    currentInfoItem->setText( 0, infoString );

    // set the icon
    switch( type ) {
    case K3b::Job::MessageError:
        currentInfoItem->setIcon( 0, QIcon::fromTheme( "dialog-error" ) );
        break;
    case K3b::Job::MessageWarning:
        currentInfoItem->setIcon( 0, QIcon::fromTheme( "dialog-warning" ) );
        break;
    case K3b::Job::MessageSuccess:
        currentInfoItem->setIcon( 0, QIcon::fromTheme( "dialog-ok" ) );
        break;
    case K3b::Job::MessageInfo:
    default:
        currentInfoItem->setIcon( 0, QIcon::fromTheme( "dialog-information" ) );
    }

    d->viewInfo->scrollToItem( currentInfoItem, QAbstractItemView::EnsureVisible );
}


void K3b::JobProgressDialog::slotFinished( bool success )
{
    qDebug() << "received finished signal!";

    m_logFile.close();

    const KColorScheme colorScheme( QPalette::Normal, KColorScheme::Window );
    QPalette taskPalette( m_labelTask->palette() );

    // Only show elapsed time at the end of the task
    // setVisible( false ) would move elapsed time one line up ...
    m_labelRemainingTime->setText( "" );
    m_labelElapsedTime->setText(i18nc("@info %1 is a duration formatted",
        "Elapsed time: %1", QTime::fromMSecsSinceStartOfDay(m_timer.elapsed()).toString("hh:mm:ss")));
    m_timer.invalidate();

    if( success ) {
        m_pixLabel->setThemePixmap( K3b::Theme::PROGRESS_SUCCESS );

        taskPalette.setColor( QPalette::WindowText,
                              colorScheme.foreground( KColorScheme::PositiveText ).color() );

        m_labelTask->setText( i18n("Success.") );
        m_labelSubTask->setText( QString() );

        m_progressPercent->setValue(100);
        m_progressSubPercent->setValue(100);
        slotProgress(100);

        KNotification::event("SuccessfullyFinished", i18n("Successfully finished."));
    }
    else {
        m_pixLabel->setThemePixmap( K3b::Theme::PROGRESS_FAIL );

        taskPalette.setColor( QPalette::WindowText,
                              colorScheme.foreground( KColorScheme::NegativeText ).color() );

        if( m_bCanceled ) {
            m_labelTask->setText( i18n("Canceled.") );
        }
        else {
            m_labelTask->setText( i18n("Error.") );
        }
        KNotification::event("FinishedWithError", i18n("Finished with errors"));
    }

    m_labelTask->setPalette( taskPalette );

    m_cancelButton->hide();
    m_showDbgOutButton->show();
    m_closeButton->show();
}


void K3b::JobProgressDialog::slotCanceled()
{
    qDebug();
    m_bCanceled = true;
}


void K3b::JobProgressDialog::setJob( K3b::Job* job )
{
    qDebug();
    m_bCanceled = false;

    // clear everything
    m_cancelButton->show();
    m_showDbgOutButton->hide();
    m_closeButton->hide();
    m_closeButton->setEnabled( true );

    d->viewInfo->clear();
    m_progressPercent->setValue(0);
    m_progressSubPercent->setValue(0);
    m_labelTask->setText("");
    m_labelSubTask->setText("");
    m_labelProcessedSize->setText("");
    m_labelSubProcessedSize->setText("");
    if (k3bappcore)
        m_labelTask->setPalette( k3bappcore->themeManager()->currentTheme()->palette() );
    m_logCache.clear();

    // disconnect from the former job
    if( m_job )
        disconnect( m_job );
    m_job = job;

    if( job ) {
        qDebug() << "connecting";
        connect( job, SIGNAL(infoMessage(QString,int)), this, SLOT(slotInfoMessage(QString,int)) );

        connect( job, SIGNAL(percent(int)), m_progressPercent, SLOT(setValue(int)) );
        connect( job, SIGNAL(percent(int)), this, SLOT(slotProgress(int)) );
        connect( job, SIGNAL(subPercent(int)), m_progressSubPercent, SLOT(setValue(int)) );

        connect( job, SIGNAL(processedSubSize(int,int)), this, SLOT(slotProcessedSubSize(int,int)) );
        connect( job, SIGNAL(processedSize(int,int)), this, SLOT(slotProcessedSize(int,int)) );

        connect( job, SIGNAL(newTask(QString)), this, SLOT(slotNewTask(QString)) );
        connect( job, SIGNAL(newSubTask(QString)), this, SLOT(slotNewSubTask(QString)) );
        connect( job, SIGNAL(started()), this, SLOT(slotStarted()) );
        connect( job, SIGNAL(finished(bool)), this, SLOT(slotFinished(bool)) );
        connect( job, SIGNAL(canceled()), this, SLOT(slotCanceled()) );

        connect( job, SIGNAL(debuggingOutput(QString,QString)),
                 this, SLOT(slotDebuggingOutput(QString,QString)) );

        m_labelJob->setText( m_job->jobDescription() );
        m_labelJobDetails->setText( m_job->jobDetails() );

        setWindowTitle( m_job->jobDescription() );

        if (KConfigGroup(KSharedConfig::openConfig(), "General Options").readEntry("Show progress OSD", false)) {
            KIO::getJobTracker()->registerJob(new KJobBridge(*job));
        }
    }
}


void K3b::JobProgressDialog::reject()
{
    if( m_job && m_job->active() ) {
        if( KMessageBox::questionYesNo( this, i18n("Do you really want to cancel?"), i18n("Cancel Confirmation") ) == KMessageBox::Yes ) {
            if( m_job ) {
                m_job->cancel();
                m_closeButton->setEnabled(true);
            }
        }
    }
}


void K3b::JobProgressDialog::slotNewSubTask(const QString& name)
{
    qDebug() << name;
    m_labelSubTask->setText(name);
    m_labelSubProcessedSize->setText("");
    m_progressSubPercent->setValue(0);
}


void K3b::JobProgressDialog::slotNewTask(const QString& name)
{
    qDebug() << name;
    m_labelTask->setText( name );
}


void K3b::JobProgressDialog::slotStarted()
{
    qDebug();
    d->lastProgress = 0;
    m_lastProgressUpdateTime = 0;
    m_timer.start();
    m_plainCaption = k3bappcore->k3bMainWindow()->windowTitle();

    m_logFile.open();
}


void K3b::JobProgressDialog::slotDebuggingOutput( const QString& type, const QString& output )
{
    m_logCache.addOutput( type, output );
    m_logFile.addOutput( type, output );
}


void K3b::JobProgressDialog::slotShowDebuggingOutput()
{
    K3b::DebuggingOutputDialog debugWidget( this );
    debugWidget.setOutput( m_logCache.toString() );
    debugWidget.exec();
}


void K3b::JobProgressDialog::slotProgress( int percent )
{
    if (percent > d->lastProgress) {
        d->lastProgress = percent;
        m_plainCaption.remove(QRegularExpression("\\(.+?\\) "));
        k3bappcore->k3bMainWindow()->setPlainCaption(QString("(%1%) %2").arg(percent).arg(m_plainCaption));

        setWindowTitle(QString("(%1%) %2").arg(percent).arg(m_job->jobDescription()));
    }

    if (m_timer.isValid()) {
	    qint64 elapsed = m_timer.elapsed();
        m_labelElapsedTime->setText(i18nc("@info %1 is a duration formatted",
            "Elapsed time: %1", QTime::fromMSecsSinceStartOfDay(elapsed).toString("hh:mm:ss")));
        // Update "Remaining time" max. each second (1000 ms)
        if (elapsed - m_lastProgressUpdateTime > 999) {
            m_labelRemainingTime->setText(i18nc("@info %1 is a duration formatted",
                "Remaining: %1", QTime::fromMSecsSinceStartOfDay(
                (d->lastProgress > 0 && d->lastProgress < 100) ? elapsed * (100 - d->lastProgress) / d->lastProgress : 0).toString("hh:mm:ss")));
            m_lastProgressUpdateTime = elapsed;
        }
    }
}


void K3b::JobProgressDialog::keyPressEvent( QKeyEvent* e )
{
    qDebug() << e;

    switch ( e->key() ) {
    case Qt::Key_Enter:
    case Qt::Key_Return:
        // if the process finished this closes the dialog
        if( m_closeButton->isVisible() )
            accept();
        break;

    case Qt::Key_Escape:
        // simulate button clicks
        if( m_job && m_job->active() )
            reject();
        else if( m_closeButton->isVisible() )
            accept();
        break;

    default:
        QDialog::keyPressEvent( e );
        break;
    }
}


QSize K3b::JobProgressDialog::sizeHint() const
{
    QSize s = QDialog::sizeHint();
    if( s.width() < s.height() )
        s.setWidth( s.height() );
    return s;
}


int K3b::JobProgressDialog::startJob( K3b::Job* job )
{
    if( job ) {
        setJob( job );
        new JobInterface( job );
    }
    else if( !m_job ) {
        qCritical() << "(K3b::JobProgressDialog) null job!" << endl;
        return -1;
    }

    QMetaObject::invokeMethod( m_job, "start", Qt::QueuedConnection );
    return exec();
}


K3b::Device::MediaType K3b::JobProgressDialog::waitForMedium( K3b::Device::Device* device,
                                                              Device::MediaStates mediaState,
                                                              Device::MediaTypes mediaType,
                                                              const K3b::Msf& minMediaSize,
                                                              const QString& message )
{
    return K3b::EmptyDiscWaiter::wait( device, mediaState, mediaType, minMediaSize, message, this );
}


bool K3b::JobProgressDialog::questionYesNo( const QString& text,
                                            const QString& caption,
                                            const KGuiItem& buttonYes,
                                            const KGuiItem& buttonNo )
{
    return ( KMessageBox::questionYesNo( this,
                                         text,
                                         caption,
                                         buttonYes,
                                         buttonNo ) == KMessageBox::Yes );
}


void K3b::JobProgressDialog::blockingInformation( const QString& text,
                                                  const QString& caption )
{
    KMessageBox::information( this, text, caption );
}


void K3b::JobProgressDialog::slotThemeChanged()
{
    if (k3bappcore == Q_NULLPTR)
        return;
    if( K3b::Theme* theme = k3bappcore->themeManager()->currentTheme() ) {
        d->progressHeaderFrame->setPalette( theme->palette() );
        d->headerFrame->setPalette( theme->palette() );
    }
}


