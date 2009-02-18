/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bemptydiscwaiter.h"
#include "k3bmediacache.h"
#include "k3bapplication.h"
#include <k3bdevice.h>
#include <k3bdeviceglobals.h>
#include <k3bdevicehandler.h>
#include <k3bglobals.h>
#include <k3bcore.h>
#include <k3biso9660.h>
#include "k3bblankingjob.h"
#include <k3bbusywidget.h>
#include <k3bprogressdialog.h>
#include <k3bdvdformattingjob.h>

#include <qtimer.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qpushbutton.h>
#include <qapplication.h>
#include <qeventloop.h>
#include <qfont.h>
#include <QGridLayout>

#include <klocale.h>
#include <kconfig.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <k3activelabel.h>
#include <KNotification>

class K3bEmptyDiscWaiter::Private
{
public:
    Private()
        : erasingInfoDialog(0) {
        dialogVisible = false;
        inLoop = false;
        mediumChanged = 0;
        blockMediaChange = false;
    }

    K3bDevice::Device* device;

    int wantedMediaType;
    int wantedMediaState;

    QString wantedMediaTypeString;

    int result;
    int dialogVisible;
    bool inLoop;

    bool blockMediaChange;
    int mediumChanged;

    bool forced;
    bool canceled;

    bool waitingDone;

    QLabel* labelRequest;
    QLabel* labelFoundMedia;
    QLabel* pixLabel;

    K3bProgressDialog* erasingInfoDialog;
};



K3bEmptyDiscWaiter::K3bEmptyDiscWaiter( K3bDevice::Device* device, QWidget* parent )
    : KDialog( parent ),
      d( new Private() )
{
    setCaption(i18n("Waiting for Disk"));
    setButtons(KDialog::Cancel|KDialog::User1|KDialog::User2|KDialog::User3);
    setDefaultButton(KDialog::User3);
    setModal(true);
    setButtonText(KDialog::User1,i18n("Force"));
    setButtonText(KDialog::User2,i18n("Eject"));
    setButtonText(KDialog::User3,i18n("Load"));

    d->device = device;

    // setup the gui
    // -----------------------------
    d->labelRequest = new QLabel( mainWidget() );
    d->labelRequest->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );
    d->labelFoundMedia = new QLabel( mainWidget() );
    d->pixLabel = new QLabel( mainWidget() );
    d->pixLabel->setAlignment( Qt::AlignHCenter | Qt::AlignTop );

    QFont f( d->labelFoundMedia->font() );
    f.setBold(true);
    d->labelFoundMedia->setFont( f );

    QGridLayout* grid = new QGridLayout( mainWidget() );
    grid->setMargin( 0 );
    grid->setSpacing( spacingHint() );

    grid->addWidget( d->pixLabel, 0, 0, 3, 1 );
    grid->addItem( new QSpacerItem( 20, 1, QSizePolicy::Fixed, QSizePolicy::Fixed ), 0, 1 );
    grid->addWidget( new QLabel( i18n("Found media:"), mainWidget() ), 0, 2 );
    grid->addWidget( d->labelFoundMedia, 0, 3 );
    grid->addWidget( d->labelRequest, 1, 2, 1, 2 );
    grid->setRowStretch( 2, 1 );
    grid->setColumnStretch( 3, 1 );
    // -----------------------------

    connect( k3bappcore->mediaCache(), SIGNAL(mediumChanged(K3bDevice::Device*)),
             this, SLOT(slotMediumChanged(K3bDevice::Device*)) );

    setButtonToolTip(KDialog::User1,
                     i18n("Force K3b to continue if it seems not to detect your empty CD/DVD.") );
    connect( this, SIGNAL( cancelClicked() ), this, SLOT( slotCancel() ) );
    connect( this, SIGNAL( user1Clicked() ), this, SLOT( slotUser1() ) );
    connect( this, SIGNAL( user2Clicked() ), this, SLOT( slotUser2() ) );
    connect( this, SIGNAL( user3Clicked() ), this, SLOT( slotUser3() ) );
}


K3bEmptyDiscWaiter::~K3bEmptyDiscWaiter()
{
    delete d;
}


int K3bEmptyDiscWaiter::waitForDisc( int mediaState, int mediaType, const QString& message )
{
    if ( d->inLoop ) {
        kError() << "(K3bEmptyDiscWaiter) Recursive call detected." << endl;
        return -1;
    }

    d->wantedMediaState = mediaState;
    d->wantedMediaType = mediaType;
    d->dialogVisible = false;
    d->forced = false;
    d->canceled = false;
    d->waitingDone = false;
    d->blockMediaChange = false;
    d->mediumChanged = 0;

    //
    // We do not cover every case here but just the ones that really make sense
    //
    if( (d->wantedMediaType & K3bDevice::MEDIA_WRITABLE_DVD) &&
        (d->wantedMediaType & K3bDevice::MEDIA_WRITABLE_CD) )
        d->wantedMediaTypeString = i18n("CD-R(W) or DVD%1R(W)",QString("±"));
    else if( d->wantedMediaType & K3bDevice::MEDIA_WRITABLE_DVD_SL )
        d->wantedMediaTypeString = i18n("DVD%1R(W)",QString("±"));
    else if( d->wantedMediaType & K3bDevice::MEDIA_WRITABLE_DVD_DL )
        d->wantedMediaTypeString = i18n("Double Layer DVD%1R",QString("±"));
    else if( d->wantedMediaType & K3bDevice::MEDIA_WRITABLE_BD )
        d->wantedMediaTypeString = i18n("Blu-ray BD-R(E)");
    else
        d->wantedMediaTypeString = i18n("CD-R(W)");

    if( message.isEmpty() ) {
        if( (d->wantedMediaState & K3bDevice::STATE_COMPLETE) && (d->wantedMediaState & K3bDevice::STATE_INCOMPLETE) )
            d->labelRequest->setText( i18n("Please insert a complete or appendable %4 medium "
                                           "into drive<p><b>%1 %2 (%3)</b>."
                                           ,d->device->vendor()
                                           ,d->device->description()
                                           ,d->device->blockDeviceName()
                                           , d->wantedMediaTypeString ) );
        else if( d->wantedMediaState & K3bDevice::STATE_COMPLETE )
            d->labelRequest->setText( i18n("Please insert a complete %4 medium "
                                           "into drive<p><b>%1 %2 (%3)</b>."
                                           ,d->device->vendor()
                                           ,d->device->description()
                                           ,d->device->blockDeviceName()
                                           , d->wantedMediaTypeString ) );
        else if( (d->wantedMediaState & K3bDevice::STATE_INCOMPLETE) && (d->wantedMediaState & K3bDevice::STATE_EMPTY) )
            d->labelRequest->setText( i18n("Please insert an empty or appendable %4 medium "
                                           "into drive<p><b>%1 %2 (%3)</b>."
                                           ,d->device->vendor()
                                           ,d->device->description()
                                           ,d->device->blockDeviceName()
                                           , d->wantedMediaTypeString ) );
        else if( d->wantedMediaState & K3bDevice::STATE_INCOMPLETE )
            d->labelRequest->setText( i18n("Please insert an appendable %4 medium "
                                           "into drive<p><b>%1 %2 (%3)</b>."
                                           ,d->device->vendor()
                                           ,d->device->description()
                                           ,d->device->blockDeviceName()
                                           , d->wantedMediaTypeString ) );
        else if( d->wantedMediaState & K3bDevice::STATE_EMPTY )
            d->labelRequest->setText( i18n("Please insert an empty %4 medium "
                                           "into drive<p><b>%1 %2 (%3)</b>."
                                           ,d->device->vendor()
                                           ,d->device->description()
                                           ,d->device->blockDeviceName()
                                           , d->wantedMediaTypeString ) );
        else // fallback case (this should not happen in K3b)
            d->labelRequest->setText( i18n("Please insert a suitable medium "
                                           "into drive<p><b>%1 %2 (%3)</b>."
                                           ,d->device->vendor()
                                           ,d->device->description()
                                           ,d->device->blockDeviceName()) );

    }
    else
        d->labelRequest->setText( message );

    if( d->wantedMediaType & K3bDevice::MEDIA_WRITABLE_DVD )
        d->pixLabel->setPixmap( KIconLoader::global()->loadIcon( "media-optical-dvd",
                                                                 KIconLoader::NoGroup, KIconLoader::SizeMedium ) );
    else
        d->pixLabel->setPixmap( KIconLoader::global()->loadIcon( "media-optical-recordable",
                                                                 KIconLoader::NoGroup, KIconLoader::SizeMedium ) );

    adjustSize();

    slotMediumChanged( d->device );

    //
    // in case we already found a medium and thus the dialog is not shown entering
    // the loop only causes problems (since there is no dialog yet the user could
    // not have forced or canceled yet
    //
    if( !d->waitingDone ) {
        d->inLoop = true;
        enterLoop();
    }
    return d->result;
}

void K3bEmptyDiscWaiter::enterLoop()
{
    QEventLoop eventLoop;
    connect(this, SIGNAL(leaveModality()),
            &eventLoop, SLOT(quit()));
    eventLoop.exec();
}

int K3bEmptyDiscWaiter::exec()
{
    return waitForDisc();
}


void K3bEmptyDiscWaiter::slotMediumChanged( K3bDevice::Device* dev )
{
    kDebug() << "(K3bEmptyDiscWaiter) slotMediumChanged() ";
    if( d->forced || d->canceled || d->device != dev )
        return;

    //
    // This slot may open dialogs which enter a new event loop and that
    // may result in another call to this slot if a medium changes while
    // a dialog is open
    //
    if( d->blockMediaChange ) {
        d->mediumChanged++;
        return;
    }

    d->blockMediaChange = true;

    bool formatWithoutAsking = KConfigGroup( KGlobal::config(), "General Options" ).readEntry( "auto rewritable erasing", false );

    K3bMedium medium = k3bappcore->mediaCache()->medium( dev );

    d->labelFoundMedia->setText( medium.shortString( false ) );

    if( medium.diskInfo().diskState() == K3bDevice::STATE_NO_MEDIA ) {
        continueWaiting();
        d->blockMediaChange = false;
        return;
    }

//   QString mediaState;
//   if( medium.diskInfo().diskState() == K3bDevice::STATE_COMPLETE )
//     mediaState = i18n("complete");
//   else if( medium.diskInfo().diskState() == K3bDevice::STATE_INCOMPLETE )
//     mediaState = i18n("appendable");
//   else if( medium.diskInfo().diskState() == K3bDevice::STATE_EMPTY )
//     mediaState = i18n("empty");

//   if( !mediaState.isEmpty() )
//     mediaState = " (" + mediaState +")";


    // /////////////////////////////////////////////////////////////
    //
    // BD-RE handling
    //
    // /////////////////////////////////////////////////////////////
    if ( (d->wantedMediaType & K3bDevice::MEDIA_BD_RE) &&
         (medium.diskInfo().mediaType() & K3bDevice::MEDIA_BD_RE) ) {

        kDebug() << "(K3bEmptyDiscWaiter) ------ found BD-RE as wanted.";

        if( d->wantedMediaState == K3bDevice::STATE_EMPTY ) {
            // check if the media contains a filesystem
            K3bIso9660 isoF( d->device );
            bool hasIso = isoF.open();

            if( formatWithoutAsking ||
                !hasIso ||
                KMessageBox::warningContinueCancel( parentWidgetToUse(),
                                                    i18n("Found %1 media in %2 - %3. "
                                                         "Should it be overwritten?"
                                                         ,QString("BD-RE")
                                                         ,d->device->vendor()
                                                         ,d->device->description()),
                                                    i18n("Found %1",QString("BD-RE")),KGuiItem(i18n("Overwrite")) ) == KMessageBox::Continue ) {
                finishWaiting( K3bDevice::MEDIA_BD_RE );
            }
            else {
                kDebug() << "(K3bEmptyDiscWaiter) starting devicehandler: no BD-RE overwrite";
                K3b::unmount( d->device );
                d->device->eject();
                continueWaiting();
            }
        }

        //
        // We want a BD-RE not nessessarily empty. No problem, just use this one. Becasue incomplete and complete
        // are handled the same everywhere (isofs is grown).
        //
        else {
            finishWaiting( K3bDevice::MEDIA_BD_RE );
        }
    }

    // /////////////////////////////////////////////////////////////
    //
    // DVD+RW handling
    //
    // /////////////////////////////////////////////////////////////

    // DVD+RW: if empty we need to preformat. Although growisofs does it before writing doing it here
    //         allows better control and a progress bar. If it's not empty we should check if there is
    //         already a filesystem on the media.
    else if( (d->wantedMediaType & K3bDevice::MEDIA_DVD_PLUS_RW) &&
             (medium.diskInfo().mediaType() & K3bDevice::MEDIA_DVD_PLUS_RW) ) {

        kDebug() << "(K3bEmptyDiscWaiter) ------ found DVD+RW as wanted.";

        if( medium.diskInfo().diskState() == K3bDevice::STATE_EMPTY ) {
            if( d->wantedMediaState & K3bDevice::STATE_EMPTY ) {
                // special case for the formatting job which wants to preformat on it's own!
                if( d->wantedMediaState & K3bDevice::STATE_COMPLETE &&
                    d->wantedMediaState & K3bDevice::STATE_EMPTY ) {
                    kDebug() << "(K3bEmptyDiscWaiter) special case: DVD+RW for the formatting job.";
                    finishWaiting( K3bDevice::MEDIA_DVD_PLUS_RW );
                }
                else {
                    // empty - preformat without asking
                    prepareErasingDialog();

                    K3bDvdFormattingJob job( this );
                    job.setDevice( d->device );
                    job.setQuickFormat( true );
                    job.setForce( false );
                    job.setForceNoEject( true );

                    d->erasingInfoDialog->setText( i18n("Preformatting DVD+RW") );
                    connect( &job, SIGNAL(finished(bool)), this, SLOT(slotErasingFinished(bool)) );
                    connect( &job, SIGNAL(percent(int)), d->erasingInfoDialog, SLOT(setProgress(int)) );
                    connect( d->erasingInfoDialog, SIGNAL(cancelClicked()), &job, SLOT(cancel()) );
                    job.start( medium.diskInfo() );
                    d->erasingInfoDialog->exec( true );
                }
            }
            else {
                kDebug() << "(K3bEmptyDiscWaiter) starting devicehandler: empty DVD+RW where a non-empty was requested.";
                continueWaiting();
            }
        }
        else {
            //
            // We have a DVD+RW medium which is already preformatted
            //
            if( d->wantedMediaState == K3bDevice::STATE_EMPTY ) {
                // check if the media contains a filesystem
                K3bIso9660 isoF( d->device );
                bool hasIso = isoF.open();

                if( formatWithoutAsking ||
                    !hasIso ||
                    KMessageBox::warningContinueCancel( parentWidgetToUse(),
                                                        i18n("Found %1 media in %2 - %3. "
                                                             "Should it be overwritten?",
                                                             QString("DVD+RW"),
                                                             d->device->vendor(),
                                                             d->device->description()),
                                                        i18n("Found %1",QString("DVD+RW")),KGuiItem(i18n("Overwrite")) ) == KMessageBox::Continue ) {
                    finishWaiting( K3bDevice::MEDIA_DVD_PLUS_RW );
                }
                else {
                    kDebug() << "(K3bEmptyDiscWaiter) starting devicehandler: no DVD+RW overwrite";
                    K3b::unmount( d->device );
                    d->device->eject();
                    continueWaiting();
                }
            }

            //
            // We want a DVD+RW not nessessarily empty. No problem, just use this one. Becasue incomplete and complete
            // are handled the same everywhere (isofs is grown).
            //
            else {
                finishWaiting( K3bDevice::MEDIA_DVD_PLUS_RW );
            }
        }
    } // --- DVD+RW --------


    // /////////////////////////////////////////////////////////////
    //
    // DVD-RW handling
    //
    // /////////////////////////////////////////////////////////////

    //
    // DVD-RW in sequential mode can be empty. DVD-RW in restricted overwrite mode is always complete.
    //
    else if( (d->wantedMediaType & (K3bDevice::MEDIA_DVD_RW|
                                    K3bDevice::MEDIA_DVD_RW_SEQ|
                                    K3bDevice::MEDIA_DVD_RW_OVWR) ) &&
             (medium.diskInfo().mediaType() & (K3bDevice::MEDIA_DVD_RW|
                                               K3bDevice::MEDIA_DVD_RW_SEQ|
                                               K3bDevice::MEDIA_DVD_RW_OVWR) ) ) {

        kDebug() << "(K3bEmptyDiscWaiter) ------ found DVD-R(W) as wanted.";

        // we format in the following cases:
        // seq. incr. and not empty and empty requested
        // seq. incr. and restr. overwri. reqested
        // restr. ovw. and seq. incr. requested

        // we have exactly what was requested
        if( (d->wantedMediaType & medium.diskInfo().mediaType()) &&
            (d->wantedMediaState & medium.diskInfo().diskState()) ) {
            finishWaiting( medium.diskInfo().mediaType() );
        }

        // DVD-RW in restr. overwrite may just be overwritten
        else if( (medium.diskInfo().mediaType() & K3bDevice::MEDIA_DVD_RW_OVWR) &&
                 (d->wantedMediaType & K3bDevice::MEDIA_DVD_RW_OVWR) ) {
            if( d->wantedMediaState == K3bDevice::STATE_EMPTY ) {

                kDebug() << "(K3bEmptyDiscWaiter) ------ DVD-RW restricted overwrite.";

                // check if the media contains a filesystem
                K3bIso9660 isoF( d->device );
                bool hasIso = isoF.open();

                if( formatWithoutAsking ||
                    !hasIso ||
                    KMessageBox::warningContinueCancel( parentWidgetToUse(),
                                                        i18n("Found %1 media in %2 - %3. "
                                                             "Should it be overwritten?",
                                                             K3bDevice::mediaTypeString(medium.diskInfo().mediaType()),
                                                             d->device->vendor(),
                                                             d->device->description()),
                                                        i18n("Found %1",QString("DVD-RW")),KGuiItem(i18n("Overwrite")) ) == KMessageBox::Continue ) {
                    finishWaiting( K3bDevice::MEDIA_DVD_RW_OVWR );
                }
                else {
                    kDebug() << "(K3bEmptyDiscWaiter) starting devicehandler: no DVD-RW overwrite.";
                    K3b::unmount( d->device );
                    d->device->eject();
                    continueWaiting();
                }
            }

            else if( !(d->wantedMediaState & K3bDevice::STATE_EMPTY ) ) {
                // check if the media contains a filesystem
                K3bIso9660 isoF( d->device );
                bool hasIso = isoF.open();

                if( hasIso ) {
                    finishWaiting( K3bDevice::MEDIA_DVD_RW_OVWR );
                }
                else {
                    kDebug() << "(K3bEmptyDiscWaiter) starting devicehandler: empty DVD-RW where a non-empty was requested.";
                    continueWaiting();
                }
            }

            //
            // We want a DVD-RW overwrite not nessessarily empty. No problem, just use this one. Becasue incomplete and complete
            // are handled the same everywhere (isofs is grown).
            //
            else {
                finishWaiting( K3bDevice::MEDIA_DVD_RW_OVWR );
            }
        }

        // formatting
        else if( ( (d->wantedMediaType & K3bDevice::MEDIA_DVD_RW_OVWR) &&
                   (medium.diskInfo().mediaType() & K3bDevice::MEDIA_DVD_RW_SEQ) &&
                   !(d->wantedMediaType & K3bDevice::MEDIA_DVD_RW_SEQ) ) ||

                 ( (d->wantedMediaType & K3bDevice::MEDIA_DVD_RW_SEQ) &&
                   (medium.diskInfo().mediaType() & K3bDevice::MEDIA_DVD_RW_OVWR) &&
                   !(d->wantedMediaType & K3bDevice::MEDIA_DVD_RW_OVWR) ) ||

                 ( (d->wantedMediaType & K3bDevice::MEDIA_DVD_RW_SEQ) &&
                   (medium.diskInfo().mediaType() & K3bDevice::MEDIA_DVD_RW_SEQ) &&
                   (d->wantedMediaState & K3bDevice::STATE_EMPTY) &&
                   (medium.diskInfo().diskState() != K3bDevice::STATE_EMPTY) ) ) {

            kDebug() << "(K3bEmptyDiscWaiter) ------ DVD-RW needs to be formated.";

            if( formatWithoutAsking ||
                KMessageBox::warningContinueCancel( parentWidgetToUse(),
                                                    i18n("Found %1 media in %2 - %3. "
                                                         "Should it be formatted?",
                                                         K3bDevice::mediaTypeString(medium.diskInfo().mediaType()),
                                                         d->device->vendor(),
                                                         d->device->description()),
                                                    i18n("Found %1",QString("DVD-RW")), KGuiItem(i18n("Format")) ) == KMessageBox::Continue ) {

                kDebug() << "(K3bEmptyDiscWaiter) ------ formatting DVD-RW.";

                prepareErasingDialog();

                K3bDvdFormattingJob job( this );
                job.setDevice( d->device );
                // we prefere the current mode of the media if no special mode has been requested
                job.setMode( ( (d->wantedMediaType & K3bDevice::MEDIA_DVD_RW_SEQ) &&
                               (d->wantedMediaType & K3bDevice::MEDIA_DVD_RW_OVWR) )
                             ? ( medium.diskInfo().mediaType() == K3bDevice::MEDIA_DVD_RW_OVWR
                                 ? K3b::WRITING_MODE_RES_OVWR
                                 : K3b::WRITING_MODE_INCR_SEQ )
                             : ( (d->wantedMediaType & K3bDevice::MEDIA_DVD_RW_SEQ)
                                 ? K3b::WRITING_MODE_INCR_SEQ
                                 : K3b::WRITING_MODE_RES_OVWR ) );
                job.setQuickFormat( true );
                job.setForce( false );
                job.setForceNoEject(true);

                d->erasingInfoDialog->setText( i18n("Formatting DVD-RW") );
                connect( &job, SIGNAL(finished(bool)), this, SLOT(slotErasingFinished(bool)) );
                connect( &job, SIGNAL(percent(int)), d->erasingInfoDialog, SLOT(setProgress(int)) );
                connect( d->erasingInfoDialog, SIGNAL(cancelClicked()), &job, SLOT(cancel()) );
                job.start( medium.diskInfo() );
                d->erasingInfoDialog->exec( true );
            }
            else {
                kDebug() << "(K3bEmptyDiscWaiter) starting devicehandler: no DVD-RW formatting.";
                K3b::unmount( d->device );
                d->device->eject();
                continueWaiting();
            }
        }
        else {
            kDebug() << "(K3bEmptyDiscWaiter) ------ nothing useful found.";
            continueWaiting();
        }
    } // --- DVD-RW ------


    // /////////////////////////////////////////////////////////////
    //
    // CD handling (and DVD-R and DVD+R)
    //
    // /////////////////////////////////////////////////////////////

    // we have exactly what was requested
    else if( (d->wantedMediaType & medium.diskInfo().mediaType()) &&
             (d->wantedMediaState & medium.diskInfo().diskState()) )
        finishWaiting( medium.diskInfo().mediaType() );

    else if( (medium.diskInfo().mediaType() != K3bDevice::MEDIA_UNKNOWN) &&
             (d->wantedMediaType & medium.diskInfo().mediaType()) &&
             (d->wantedMediaState & medium.diskInfo().diskState()) )
        finishWaiting( medium.diskInfo().mediaType() );

    // this is for CD drives that are not able to determine the state of a disk
    else if( medium.diskInfo().diskState() == K3bDevice::STATE_UNKNOWN &&
             medium.diskInfo().mediaType() == K3bDevice::MEDIA_CD_ROM &&
             d->wantedMediaType & K3bDevice::MEDIA_CD_ROM )
        finishWaiting( medium.diskInfo().mediaType() );

    // format CD-RW
    else if( (d->wantedMediaType & medium.diskInfo().mediaType()) &&
             (d->wantedMediaState & K3bDevice::STATE_EMPTY) &&
             medium.diskInfo().rewritable() ) {

        if( formatWithoutAsking ||
            KMessageBox::questionYesNo( parentWidgetToUse(),
                                        i18n("Found rewritable media in %1 - %2. "
                                             "Should it be erased?",d->device->vendor(),d->device->description()),
                                        i18n("Found Rewritable Disk"),
                                        KGuiItem(i18n("&Erase"), "cdrwblank"),
                                        KGuiItem(i18n("E&ject")) ) == KMessageBox::Yes ) {


            prepareErasingDialog();

            // start a k3bblankingjob
            d->erasingInfoDialog->setText( i18n("Erasing CD-RW") );

            // the user may be using cdrdao for erasing as cdrecord does not work
            int erasingApp = K3b::WRITING_APP_DEFAULT;
            if( KGlobal::config()->group( "General Options" ).readEntry( "Manual writing app selection", false ) ) {
                erasingApp = K3b::writingAppFromString( KGlobal::config()->group( "CDRW Erasing" ).readEntry( "writing_app" ) );
            }

            K3bBlankingJob job( this );
            job.setDevice( d->device );
            job.setMode( K3bBlankingJob::Fast );
            job.setForce(true);
            job.setForceNoEject(true);
            job.setSpeed( 0 ); // Auto
            job.setWritingApp( erasingApp );
            connect( &job, SIGNAL(finished(bool)), this, SLOT(slotErasingFinished(bool)) );
            connect( d->erasingInfoDialog, SIGNAL(cancelClicked()), &job, SLOT(cancel()) );
            job.start();
            d->erasingInfoDialog->exec( false );
        }
        else {
            kDebug() << "(K3bEmptyDiscWaiter) starting devicehandler: no CD-RW overwrite.";
            K3b::unmount( d->device );
            d->device->eject();
            continueWaiting();
        }
    }
    else {
        kDebug() << "(K3bEmptyDiscWaiter) ------ nothing useful found.";
        continueWaiting();
    }

    // handle queued medium changes
    d->blockMediaChange = false;
    if( d->mediumChanged > 0 ) {
        d->mediumChanged--;
        slotMediumChanged( dev );
    }
}


void K3bEmptyDiscWaiter::showDialog()
{
    // we need to show the dialog if not done already
    if( !d->dialogVisible ) {

        KNotification::event( "WaitingForMedium", i18n("Waiting for Medium"),QPixmap(),0 );

        d->dialogVisible = true;
        //clear it.
        setAttribute(Qt::WA_DeleteOnClose,false);
        setWindowFlags(windowFlags () ^ Qt::WShowModal);
        setResult( 0 );
        show();
    }
}


void K3bEmptyDiscWaiter::continueWaiting()
{
    showDialog();
}


void K3bEmptyDiscWaiter::slotCancel()
{
    kDebug() << "(K3bEmptyDiscWaiter) slotCancel() ";
    d->canceled = true;
    finishWaiting( CANCELED );
}


void K3bEmptyDiscWaiter::slotUser1()
{
    d->forced = true;
    finishWaiting( DISK_READY );
}


void K3bEmptyDiscWaiter::slotUser2()
{
    K3b::unmount( d->device );
    K3bDevice::eject( d->device );
}


void K3bEmptyDiscWaiter::slotUser3()
{
    K3bDevice::load( d->device );
}


void K3bEmptyDiscWaiter::finishWaiting( int code )
{
    kDebug() << "(K3bEmptyDiscWaiter) finishWaiting() ";

    d->waitingDone = true;
    d->result = code;

    if( d->dialogVisible )
        hide();

    if( d->inLoop ) {
        d->inLoop = false;
        kDebug() << "(K3bEmptyDiscWaiter) exitLoop ";
        emit leaveModality();
    }
}


void K3bEmptyDiscWaiter::slotErasingFinished( bool success )
{
    if( success ) {
        // close the dialog thus ending it's event loop -> back to slotMediumChanged
        d->erasingInfoDialog->hide();
    }
    else {
        K3bDevice::eject( d->device );
        KMessageBox::error( d->erasingInfoDialog, i18n("Erasing failed.") );
        d->erasingInfoDialog->hide(); // close the dialog thus ending it's event loop -> back to slotMediumChanged
    }
}


int K3bEmptyDiscWaiter::wait( K3bDevice::Device* device, bool appendable, int mediaType, QWidget* parent )
{
    K3bEmptyDiscWaiter d( device, parent ? parent : qApp->activeWindow() );
    int mediaState = K3bDevice::STATE_EMPTY;
    if( appendable ) mediaState |= K3bDevice::STATE_INCOMPLETE;
    return d.waitForDisc( mediaState, mediaType );
}


int K3bEmptyDiscWaiter::wait( K3bDevice::Device* device,
                              int mediaState,
                              int mediaType,
                              const QString& message,
                              QWidget* parent )
{
    K3bEmptyDiscWaiter d( device, parent ? parent : qApp->activeWindow() );
    return d.waitForDisc( mediaState, mediaType, message );
}


void K3bEmptyDiscWaiter::prepareErasingDialog()
{
    // we hide the emptydiskwaiter so the info dialog needs to have the same parent
    if( !d->erasingInfoDialog )
        d->erasingInfoDialog = new K3bProgressDialog( QString(), parentWidget() );

    //
    // hide the dialog
    //
    if( d->dialogVisible ) {
        hide();
        d->dialogVisible = false;
    }
}


QWidget* K3bEmptyDiscWaiter::parentWidgetToUse()
{
    // we might also show dialogs if the discwaiter widget is not visible yet
    if( d->dialogVisible )
        return this;
    else
        return parentWidget();
}


int K3bEmptyDiscWaiter::waitForMedia( K3bDevice::Device* device,
                                      int mediaState,
                                      int mediaType,
                                      const QString& message )
{
    // this is only needed for the formatting
    return wait( device, mediaState, mediaType, message, d->erasingInfoDialog );
}


bool K3bEmptyDiscWaiter::questionYesNo( const QString& text,
                                        const QString& caption,
                                        const QString& yesText,
                                        const QString& noText )
{
    return ( KMessageBox::questionYesNo( parentWidgetToUse(),
                                         text,
                                         caption,
                                         yesText.isEmpty() ? KStandardGuiItem::yes() : KGuiItem(yesText),
                                         noText.isEmpty() ? KStandardGuiItem::no() : KGuiItem(noText) ) == KMessageBox::Yes );
}


void K3bEmptyDiscWaiter::blockingInformation( const QString& text,
                                              const QString& caption )
{
    KMessageBox::information( this, text, caption );
}

#include "k3bemptydiscwaiter.moc"
