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


#include "k3bemptydiscwaiter.h"
#include "k3bmediacache.h"
#include "k3bapplication.h"
#include "k3bdevice.h"
#include "k3bdeviceglobals.h"
#include "k3bdevicehandler.h"
#include "k3bglobals.h"
#include "k3bcore.h"
#include "k3biso9660.h"
#include "k3bblankingjob.h"
#include "k3bbusywidget.h"
#include "k3bdvdformattingjob.h"

#include <KConfig>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KIconLoader>
#include <KLocalizedString>
#include <KNotification>
#include <KGuiItem>
#include <KMessageBox>

#include <QEventLoop>
#include <QTimer>
#include <QFont>
#include <QApplication>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QLayout>
#include <QProgressDialog>
#include <QPushButton>
#include <QToolTip>


class K3b::EmptyDiscWaiter::Private
{
public:
    Private()
        : erasingInfoDialog(0) {
        dialogVisible = false;
        inLoop = false;
        mediumChanged = 0;
        blockMediaChange = false;
    }

    K3b::Device::Device* device;

    Device::MediaTypes wantedMediaType;
    Device::MediaStates wantedMediaState;
    Msf wantedMinMediaSize;

    Device::MediaType result;
    int dialogVisible;
    bool inLoop;

    bool blockMediaChange;
    int mediumChanged;

    bool canceled;

    bool waitingDone;

    QLabel* labelRequest;
    QLabel* labelFoundMedia;
    QLabel* pixLabel;

    QProgressDialog* erasingInfoDialog;
};



K3b::EmptyDiscWaiter::EmptyDiscWaiter( K3b::Device::Device* device, QWidget* parent )
    : QDialog( parent ),
      d( new Private() )
{
    setWindowTitle(i18n("Waiting for Disk"));
    setModal(true);

    d->device = device;

    // setup the gui
    // -----------------------------
    d->labelRequest = new QLabel( this );
    d->labelRequest->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );
    d->labelFoundMedia = new QLabel( this );
    d->pixLabel = new QLabel( this );
    d->pixLabel->setAlignment( Qt::AlignHCenter | Qt::AlignTop );

    QFont f( d->labelFoundMedia->font() );
    f.setBold(true);
    d->labelFoundMedia->setFont( f );

    QDialogButtonBox* buttonBox = new QDialogButtonBox( this );

    QPushButton* cancelButton = buttonBox->addButton( QDialogButtonBox::Cancel );
    connect( cancelButton, SIGNAL(clicked()), this, SLOT(slotCancel()) );

    QPushButton* ejectButton = new QPushButton( this );
    KGuiItem::assign( ejectButton, KGuiItem( i18n("Eject"), QString::fromLatin1( "media-eject" ) ) );
    buttonBox->addButton( ejectButton, QDialogButtonBox::NoRole );
    connect( ejectButton, SIGNAL(clicked()), this, SLOT(slotEject()) );

    QPushButton* loadButton = buttonBox->addButton( i18n("Load"), QDialogButtonBox::NoRole );
    connect( loadButton, SIGNAL(clicked()), this, SLOT(slotLoad()) );

    QVBoxLayout* box = new QVBoxLayout( this );
    QHBoxLayout* hbox = new QHBoxLayout( this );

    hbox->addWidget(d->pixLabel);
    QVBoxLayout* vbox = new QVBoxLayout( this );
    vbox->addWidget(new QLabel(i18n("Found medium:"), this));
    vbox->addWidget(d->labelFoundMedia);
    vbox->addWidget(d->labelRequest);
    hbox->addLayout( vbox );
    box->addLayout( hbox );
    box->addWidget(buttonBox);
    // -----------------------------

    connect( k3bappcore->mediaCache(), SIGNAL(mediumChanged(K3b::Device::Device*)),
             this, SLOT(slotMediumChanged(K3b::Device::Device*)) );
}


K3b::EmptyDiscWaiter::~EmptyDiscWaiter()
{
    delete d;
}


K3b::Device::MediaType K3b::EmptyDiscWaiter::waitForDisc( Device::MediaStates mediaState,
                                                          Device::MediaTypes mediaType,
                                                          const K3b::Msf& minMediaSize,
                                                          const QString& message )
{
    if ( d->inLoop ) {
        qCritical() << "Recursive call detected." << endl;
        return Device::MEDIA_UNKNOWN;
    }

    qDebug() << "Waiting for medium" << mediaState << mediaType << message;

    d->wantedMediaState = mediaState;
    d->wantedMediaType = mediaType;
    d->wantedMinMediaSize = minMediaSize;
    d->dialogVisible = false;
    d->canceled = false;
    d->waitingDone = false;
    d->blockMediaChange = false;
    d->mediumChanged = 0;

    // FIXME: reproducablitity race?
    if (message.isEmpty())
        d->labelRequest->setText( Medium::mediaRequestString( d->wantedMediaType, d->wantedMediaState, minMediaSize, d->device ) );
    else
        d->labelRequest->setText( message );

    if( d->wantedMediaType & K3b::Device::MEDIA_WRITABLE_DVD )
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
    // not have canceled yet
    //
    if( !d->waitingDone ) {
        d->inLoop = true;
        enterLoop();
    }
    return d->result;
}

void K3b::EmptyDiscWaiter::enterLoop()
{
    QEventLoop eventLoop;
    connect(this, SIGNAL(leaveModality()),
            &eventLoop, SLOT(quit()));
    eventLoop.exec();
}

int K3b::EmptyDiscWaiter::exec()
{
    return waitForDisc();
}


void K3b::EmptyDiscWaiter::slotMediumChanged( K3b::Device::Device* dev )
{
    qDebug() << dev->blockDeviceName();
    if( d->canceled || d->device != dev )
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

    bool formatWithoutAsking = KConfigGroup( KSharedConfig::openConfig(), "General Options" ).readEntry( "auto rewritable erasing", false );

    K3b::Medium medium = k3bappcore->mediaCache()->medium( dev );

    d->labelFoundMedia->setText( medium.shortString( Medium::NoStringFlags ) );

    if( medium.diskInfo().diskState() == K3b::Device::STATE_NO_MEDIA ) {
        continueWaiting();
        d->blockMediaChange = false;
        return;
    }

//   QString mediaState;
//   if( medium.diskInfo().diskState() == K3b::Device::STATE_COMPLETE )
//     mediaState = i18n("complete");
//   else if( medium.diskInfo().diskState() == K3b::Device::STATE_INCOMPLETE )
//     mediaState = i18n("appendable");
//   else if( medium.diskInfo().diskState() == K3b::Device::STATE_EMPTY )
//     mediaState = i18n("empty");

//   if( !mediaState.isEmpty() )
//     mediaState = " (" + mediaState +")";

    // /////////////////////////////////////////////////////////////
    //
    // BD-RE handling
    //
    // /////////////////////////////////////////////////////////////
    if ( (d->wantedMediaType & K3b::Device::MEDIA_BD_RE) &&
         (medium.diskInfo().mediaType() & K3b::Device::MEDIA_BD_RE) ) {

        qDebug() << "------ found BD-RE as wanted.";

#ifdef _MSC_VER
#pragma message ("WARNING: FIXME: We need to preformat empty BD-RE just like we do with empty DVD+RW")
#else
#warning FIXME: We need to preformat empty BD-RE just like we do with empty DVD+RW
#endif

        if( d->wantedMediaState == K3b::Device::STATE_EMPTY &&
            ( d->wantedMinMediaSize <= medium.diskInfo().capacity() ||
              IsOverburnAllowed( d->wantedMinMediaSize, medium.diskInfo().capacity() ) ) ) {
            // check if the media contains a filesystem
            K3b::Iso9660 isoF( d->device );
            bool hasIso = isoF.open();

            if( formatWithoutAsking ||
                !hasIso ||
                KMessageBox::warningContinueCancel( parentWidgetToUse(),
                                                    i18n("Found %1 medium in %2 - %3. Should it be overwritten?",
                                                         QLatin1String("BD-RE"),
                                                         d->device->vendor(),
                                                         d->device->description()),
                                                    i18n("Found %1", QLatin1String("BD-RE")),
                                                    KStandardGuiItem::overwrite(),
                                                    KGuiItem(i18n("&Eject"), "media-eject") ) == KMessageBox::Continue ) {
                finishWaiting( K3b::Device::MEDIA_BD_RE );
            }
            else {
                qDebug() << "starting devicehandler: no BD-RE overwrite";
                K3b::eject( d->device );
                continueWaiting();
            }
        }

        //
        // We want a BD-RE not nessessarily empty. No problem, just use this one. Because incomplete and complete
        // are handled the same everywhere (isofs is grown).
        //
        else if ( d->wantedMediaState != Device::STATE_EMPTY &&
                  ( d->wantedMinMediaSize <= medium.actuallyRemainingSize() ||
                    IsOverburnAllowed( d->wantedMinMediaSize, medium.diskInfo().capacity(), medium.actuallyUsedCapacity() ) ) ) {
            finishWaiting( K3b::Device::MEDIA_BD_RE );
        }
        else {
            qDebug() << "BD-RE medium too small";
            continueWaiting();
        }
    }

    // /////////////////////////////////////////////////////////////
    //
    // DVD+RW handling
    //
    // /////////////////////////////////////////////////////////////

    // DVD+RW: if empty we need to preformat. Although growisofs does it before writing doing it here
    //         allows better control and a progress bar. If it's not empty we should check if there is
    //         already a filesystem on the medium.
    else if( (d->wantedMediaType & K3b::Device::MEDIA_DVD_PLUS_RW) &&
             (medium.diskInfo().mediaType() & K3b::Device::MEDIA_DVD_PLUS_RW) ) {

        qDebug() << "------ found DVD+RW as wanted.";

        if( medium.diskInfo().diskState() == K3b::Device::STATE_EMPTY ) {
            if( d->wantedMediaState & K3b::Device::STATE_EMPTY &&
                ( d->wantedMinMediaSize <= medium.diskInfo().capacity() ||
                  IsOverburnAllowed( d->wantedMinMediaSize, medium.diskInfo().capacity() ) ) ) {
                // special case for the formatting job which wants to preformat on it's own!
                if( d->wantedMediaState & K3b::Device::STATE_COMPLETE &&
                    d->wantedMediaState & K3b::Device::STATE_EMPTY ) {
                    qDebug() << "special case: DVD+RW for the formatting job.";
                    finishWaiting( K3b::Device::MEDIA_DVD_PLUS_RW );
                }
                else {
                    // empty - preformat without asking
                    prepareErasingDialog();

                    K3b::DvdFormattingJob job( this );
                    job.setDevice( d->device );
                    job.setFormattingMode( FormattingQuick );
                    job.setForce( false );
                    job.setForceNoEject( true );

                    d->erasingInfoDialog->reset();
                    d->erasingInfoDialog->setLabelText( i18n("Preformatting DVD+RW") );
                    d->erasingInfoDialog->setRange( 0, 100 );
                    connect( &job, SIGNAL(finished(bool)), this, SLOT(slotErasingFinished(bool)) );
                    connect( &job, SIGNAL(percent(int)), d->erasingInfoDialog, SLOT(setValue(int)) );
                    connect( d->erasingInfoDialog, SIGNAL(cancelClicked()), &job, SLOT(cancel()) );
                    job.start( medium.diskInfo() );
                    d->erasingInfoDialog->exec();
                }
            }
            else {
                qDebug() << "starting devicehandler: empty DVD+RW where a non-empty was requested.";
                continueWaiting();
            }
        }
        else {
            //
            // We have a DVD+RW medium which is already preformatted
            //
            if( d->wantedMediaState == K3b::Device::STATE_EMPTY ) {
                if ( d->wantedMinMediaSize <= medium.diskInfo().capacity() ||
                     IsOverburnAllowed( d->wantedMinMediaSize, medium.diskInfo().capacity() ) ) {
                    // check if the media contains a filesystem
                    K3b::Iso9660 isoF( d->device );
                    bool hasIso = isoF.open();

                    if( formatWithoutAsking ||
                        !hasIso ||
                        KMessageBox::warningContinueCancel( parentWidgetToUse(),
                                                            i18n("Found %1 medium in %2 - %3. "
                                                                 "Should it be overwritten?",
                                                                 QString("DVD+RW"),
                                                                 d->device->vendor(),
                                                                 d->device->description()),
                                                            i18n("Found %1",QString("DVD+RW")),
                                                            KStandardGuiItem::overwrite(),
                                                            KGuiItem(i18n("&Eject"), "media-eject") ) == KMessageBox::Continue ) {
                        finishWaiting( K3b::Device::MEDIA_DVD_PLUS_RW );
                    }
                    else {
                        qDebug() << "starting devicehandler: no DVD+RW overwrite";
                        K3b::eject( d->device );
                        continueWaiting();
                    }
                }
                else {
                    qDebug() << "starting devicehandler: DVD+RW too small";
                    continueWaiting();
                }
            }

            //
            // We want a DVD+RW not nessessarily empty. No problem, just use this one. Becasue incomplete and complete
            // are handled the same everywhere (isofs is grown).
            //
            else if ( d->wantedMinMediaSize <= medium.actuallyRemainingSize() ||
                      IsOverburnAllowed( d->wantedMinMediaSize, medium.diskInfo().capacity(), medium.actuallyUsedCapacity() ) ) {
                finishWaiting( K3b::Device::MEDIA_DVD_PLUS_RW );
            }
            else {
                qDebug() << "starting devicehandler: DVD+RW too small";
                continueWaiting();
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
    else if( (d->wantedMediaType & (K3b::Device::MEDIA_DVD_RW|
                                    K3b::Device::MEDIA_DVD_RW_SEQ|
                                    K3b::Device::MEDIA_DVD_RW_OVWR) ) &&
             (medium.diskInfo().mediaType() & (K3b::Device::MEDIA_DVD_RW|
                                               K3b::Device::MEDIA_DVD_RW_SEQ|
                                               K3b::Device::MEDIA_DVD_RW_OVWR) ) ) {

        qDebug() << "------ found DVD-R(W) as wanted.";

        // we format in the following cases:
        // seq. incr. and not empty and empty requested
        // seq. incr. and restr. overwri. reqested
        // restr. ovw. and seq. incr. requested

        // we have exactly what was requested (K3b never requests a specific
        // size for read-only cases, thus using remainingSize() is perfectly fine)
        if( (d->wantedMediaType & medium.diskInfo().mediaType()) &&
            (d->wantedMediaState & medium.diskInfo().diskState()) &&
            ( d->wantedMinMediaSize <= medium.actuallyRemainingSize() ||
              IsOverburnAllowed( d->wantedMinMediaSize, medium.diskInfo().capacity(), medium.actuallyUsedCapacity() ) ) ) {
            finishWaiting( medium.diskInfo().mediaType() );
        }

        // DVD-RW in restr. overwrite may just be overwritten
        else if( (medium.diskInfo().mediaType() & K3b::Device::MEDIA_DVD_RW_OVWR) &&
                 (d->wantedMediaType & K3b::Device::MEDIA_DVD_RW_OVWR) ) {
            if( d->wantedMediaState == K3b::Device::STATE_EMPTY &&
                ( d->wantedMinMediaSize <= medium.diskInfo().capacity() ||
                  IsOverburnAllowed( d->wantedMinMediaSize, medium.diskInfo().capacity() ) ) ) {

                qDebug() << "------ DVD-RW restricted overwrite.";

                // check if the media contains a filesystem
                K3b::Iso9660 isoF( d->device );
                bool hasIso = isoF.open();

                if( formatWithoutAsking ||
                    !hasIso ||
                    KMessageBox::warningContinueCancel( parentWidgetToUse(),
                                                        i18n("Found %1 medium in %2 - %3. "
                                                             "Should it be overwritten?",
                                                             K3b::Device::mediaTypeString(medium.diskInfo().mediaType()),
                                                             d->device->vendor(),
                                                             d->device->description()),
                                                        i18n("Found %1",QString("DVD-RW")),
                                                        KStandardGuiItem::overwrite(),
                                                        KGuiItem(i18n("&Eject"), "media-eject")) == KMessageBox::Continue ) {
                    finishWaiting( K3b::Device::MEDIA_DVD_RW_OVWR );
                }
                else {
                    qDebug() << "starting devicehandler: no DVD-RW overwrite.";
                    K3b::eject( d->device );
                    continueWaiting();
                }
            }

            // no need to check the size here. K3b never asks for non-empty media by size
            else if( !(d->wantedMediaState & K3b::Device::STATE_EMPTY ) ) {
                // check if the media contains a filesystem
                K3b::Iso9660 isoF( d->device );
                bool hasIso = isoF.open();

                if( hasIso ) {
                    finishWaiting( K3b::Device::MEDIA_DVD_RW_OVWR );
                }
                else {
                    qDebug() << "starting devicehandler: empty DVD-RW where a non-empty was requested.";
                    continueWaiting();
                }
            }

            //
            // We want a DVD-RW overwrite not nessessarily empty. No problem, just use this one. Becasue incomplete and complete
            // are handled the same everywhere (isofs is grown).
            //
            else if ( d->wantedMinMediaSize <= medium.actuallyRemainingSize() ||
                      IsOverburnAllowed( d->wantedMinMediaSize, medium.diskInfo().capacity(), medium.actuallyUsedCapacity() ) ) {
                finishWaiting( K3b::Device::MEDIA_DVD_RW_OVWR );
            }
            else {
                qDebug() << "starting devicehandler: DVD-RW too small";
                continueWaiting();
            }
        }

        // formatting
        else if( ( (d->wantedMediaType & K3b::Device::MEDIA_DVD_RW_OVWR) &&
                   (medium.diskInfo().mediaType() & K3b::Device::MEDIA_DVD_RW_SEQ) &&
                   !(d->wantedMediaType & K3b::Device::MEDIA_DVD_RW_SEQ) ) ||

                 ( (d->wantedMediaType & K3b::Device::MEDIA_DVD_RW_SEQ) &&
                   (medium.diskInfo().mediaType() & K3b::Device::MEDIA_DVD_RW_OVWR) &&
                   !(d->wantedMediaType & K3b::Device::MEDIA_DVD_RW_OVWR) ) ||

                 ( (d->wantedMediaType & K3b::Device::MEDIA_DVD_RW_SEQ) &&
                   (medium.diskInfo().mediaType() & K3b::Device::MEDIA_DVD_RW_SEQ) &&
                   (d->wantedMediaState & K3b::Device::STATE_EMPTY) &&
                   (medium.diskInfo().diskState() != K3b::Device::STATE_EMPTY) ) ) {

            qDebug() << "------ DVD-RW needs to be formatted.";

            if( formatWithoutAsking ||
                KMessageBox::warningContinueCancel( parentWidgetToUse(),
                                                    i18n("Found %1 medium in %2 - %3. "
                                                         "Should it be formatted?",
                                                         K3b::Device::mediaTypeString(medium.diskInfo().mediaType()),
                                                         d->device->vendor(),
                                                         d->device->description()),
                                                    i18n("Found %1",QString("DVD-RW")),
                                                    KGuiItem(i18n("&Format"), "tools-media-optical-format"),
                                                    KGuiItem(i18n("&Eject"), "media-eject")) == KMessageBox::Continue ) {

                qDebug() << "------ formatting DVD-RW.";

                prepareErasingDialog();

                K3b::DvdFormattingJob job( this );
                job.setDevice( d->device );
                // we prefer the current mode of the media if no special mode has been requested
                job.setMode( ( (d->wantedMediaType & K3b::Device::MEDIA_DVD_RW_SEQ) &&
                               (d->wantedMediaType & K3b::Device::MEDIA_DVD_RW_OVWR) )
                             ? ( medium.diskInfo().mediaType() == K3b::Device::MEDIA_DVD_RW_OVWR
                                 ? K3b::WritingModeRestrictedOverwrite
                                 : K3b::WritingModeIncrementalSequential )
                             : ( (d->wantedMediaType & K3b::Device::MEDIA_DVD_RW_SEQ)
                                 ? K3b::WritingModeIncrementalSequential
                                 : K3b::WritingModeRestrictedOverwrite ) );
                job.setFormattingMode( FormattingQuick );
                job.setForce( false );
                job.setForceNoEject(true);

                d->erasingInfoDialog->reset();
                d->erasingInfoDialog->setLabelText( i18n("Formatting DVD-RW") );
                d->erasingInfoDialog->setRange( 0, 100 );
                connect( &job, SIGNAL(finished(bool)), this, SLOT(slotErasingFinished(bool)) );
                connect( &job, SIGNAL(percent(int)), d->erasingInfoDialog, SLOT(setValue(int)) );
                connect( d->erasingInfoDialog, SIGNAL(cancelClicked()), &job, SLOT(cancel()) );
                job.start( medium.diskInfo() );
                d->erasingInfoDialog->exec();
            }
            else {
                qDebug() << "starting devicehandler: no DVD-RW formatting.";
                K3b::eject( d->device );
                continueWaiting();
            }
        }
        else {
            qDebug() << "------ nothing useful found.";
            continueWaiting();
        }
    } // --- DVD-RW ------


    // /////////////////////////////////////////////////////////////
    //
    // CD-RW handling
    //
    // /////////////////////////////////////////////////////////////

    // format CD-RW
    else if( medium.diskInfo().mediaType() == Device::MEDIA_CD_RW &&
             (d->wantedMediaType & medium.diskInfo().mediaType()) &&
             (d->wantedMediaState & K3b::Device::STATE_EMPTY) &&
             !medium.diskInfo().empty() &&
             medium.diskInfo().rewritable() ) {

        if( formatWithoutAsking ||
            KMessageBox::questionYesNo( parentWidgetToUse(),
                                        i18n("Found rewritable medium in %1 - %2. "
                                             "Should it be erased?",d->device->vendor(),d->device->description()),
                                        i18n("Found Rewritable Disk"),
                                        KGuiItem(i18n("E&rase"), "tools-media-optical-erase"),
                                        KGuiItem(i18n("&Eject"), "media-eject") ) == KMessageBox::Yes ) {


            prepareErasingDialog();

            // start a k3bblankingjob
            d->erasingInfoDialog->reset();
            d->erasingInfoDialog->setLabelText( i18n("Erasing CD-RW") );
            d->erasingInfoDialog->setRange( 0, 0 );

            // the user may be using cdrdao for erasing as cdrecord does not work
            WritingApp erasingApp = K3b::WritingAppAuto;
            if( KSharedConfig::openConfig()->group( "General Options" ).readEntry( "Show advanced GUI", false ) ) {
                erasingApp = K3b::writingAppFromString( KSharedConfig::openConfig()->group( "CDRW Erasing" ).readEntry( "writing_app" ) );
            }

            K3b::BlankingJob job( this );
            job.setDevice( d->device );
            job.setFormattingMode( FormattingQuick );
            job.setForce(true);
            job.setForceNoEject(true);
            job.setSpeed( 0 ); // Auto
            job.setWritingApp( erasingApp );
            connect( &job, SIGNAL(finished(bool)), this, SLOT(slotErasingFinished(bool)) );
            connect( d->erasingInfoDialog, SIGNAL(cancelClicked()), &job, SLOT(cancel()) );
            job.start();
            d->erasingInfoDialog->exec();
        }
        else {
            qDebug() << "starting devicehandler: no CD-RW overwrite.";
            K3b::eject( d->device );
            continueWaiting();
        }
    }

    // /////////////////////////////////////////////////////////////
    //
    // All the non-rewritable media types are handled here
    //
    // /////////////////////////////////////////////////////////////

    // we have exactly what was requested (K3b never requests a specific
    // size for read-only cases, thus using remainingSize() is perfectly fine)
    else if( (d->wantedMediaType & medium.diskInfo().mediaType()) &&
             (d->wantedMediaState & medium.diskInfo().diskState()) &&
             (d->wantedMinMediaSize <= medium.actuallyRemainingSize() ||
              IsOverburnAllowed( d->wantedMinMediaSize, medium.diskInfo().capacity(), medium.actuallyUsedCapacity() )) ) {
        finishWaiting( medium.diskInfo().mediaType() );
    }

    // this is for CD drives that are not able to determine the state of a disk
    else if( medium.diskInfo().diskState() == K3b::Device::STATE_UNKNOWN &&
             medium.diskInfo().mediaType() == K3b::Device::MEDIA_CD_ROM &&
             d->wantedMediaType & K3b::Device::MEDIA_CD_ROM ) {
        finishWaiting( medium.diskInfo().mediaType() );
    }

    else {
        qDebug() << "------ nothing useful found.";
        continueWaiting();
    }

    // handle queued medium changes
    d->blockMediaChange = false;
    if( d->mediumChanged > 0 ) {
        d->mediumChanged--;
        slotMediumChanged( dev );
    }
}


void K3b::EmptyDiscWaiter::showDialog()
{
    // we need to show the dialog if not done already
    if( !d->dialogVisible ) {

        KNotification::event( "WaitingForMedium", i18n("Waiting for Medium") );

        d->dialogVisible = true;
        //clear it.
        setAttribute(Qt::WA_DeleteOnClose,false);
        setWindowModality( Qt::NonModal );
        setResult( 0 );
        show();
    }
}


void K3b::EmptyDiscWaiter::continueWaiting()
{
    showDialog();
}


void K3b::EmptyDiscWaiter::slotCancel()
{
    qDebug() << "slotCancel() ";
    d->canceled = true;
    finishWaiting( Device::MEDIA_UNKNOWN );
}


void K3b::EmptyDiscWaiter::slotEject()
{
    K3b::unmount( d->device );
    K3b::Device::eject( d->device );
}


void K3b::EmptyDiscWaiter::slotLoad()
{
    K3b::Device::load( d->device );
}


void K3b::EmptyDiscWaiter::finishWaiting( Device::MediaType type )
{
    qDebug() << "finishWaiting() ";

    d->waitingDone = true;
    d->result = type;

    if( d->dialogVisible )
        hide();

    if( d->inLoop ) {
        d->inLoop = false;
        qDebug() << "exitLoop ";
        emit leaveModality();
    }
}


void K3b::EmptyDiscWaiter::slotErasingFinished( bool success )
{
    if( success ) {
        // close the dialog thus ending it's event loop -> back to slotMediumChanged
        d->erasingInfoDialog->hide();
    }
    else {
        K3b::Device::eject( d->device );
        KMessageBox::error( d->erasingInfoDialog, i18n("Erasing failed.") );
        d->erasingInfoDialog->hide(); // close the dialog thus ending it's event loop -> back to slotMediumChanged
    }
}


K3b::Device::MediaType K3b::EmptyDiscWaiter::wait( K3b::Device::Device* device,
                                                   Device::MediaStates mediaState,
                                                   Device::MediaTypes mediaType,
                                                   const K3b::Msf& minMediaSize,
                                                   const QString& message,
                                                   QWidget* parent )
{
    if( device != 0 ) {
        K3b::EmptyDiscWaiter d( device, parent ? parent : qApp->activeWindow() );
        return d.waitForDisc( mediaState, mediaType, minMediaSize, message );
    }
    else {
        return Device::MEDIA_UNKNOWN;
    }
}


void K3b::EmptyDiscWaiter::prepareErasingDialog()
{
    // we hide the emptydiskwaiter so the info dialog needs to have the same parent
    if( !d->erasingInfoDialog )
        d->erasingInfoDialog = new QProgressDialog( parentWidget() );

    //
    // hide the dialog
    //
    if( d->dialogVisible ) {
        hide();
        d->dialogVisible = false;
    }
}


QWidget* K3b::EmptyDiscWaiter::parentWidgetToUse()
{
    // we might also show dialogs if the discwaiter widget is not visible yet
    if( d->dialogVisible )
        return this;
    else
        return parentWidget();
}


K3b::Device::MediaType K3b::EmptyDiscWaiter::waitForMedium( K3b::Device::Device* device,
                                                            Device::MediaStates mediaState,
                                                            Device::MediaTypes mediaType,
                                                            const K3b::Msf& minMediaSize,
                                                            const QString& message )
{
    // this is only needed for the formatting
    return wait( device, mediaState, mediaType, minMediaSize, message, d->erasingInfoDialog );
}


bool K3b::EmptyDiscWaiter::questionYesNo( const QString& text,
                                          const QString& caption,
                                          const KGuiItem& buttonYes,
                                          const KGuiItem& buttonNo )
{
    return ( KMessageBox::questionYesNo( parentWidgetToUse(),
                                         text,
                                         caption,
                                         buttonYes,
                                         buttonNo ) == KMessageBox::Yes );
}


void K3b::EmptyDiscWaiter::blockingInformation( const QString& text,
                                                const QString& caption )
{
    KMessageBox::information( this, text, caption );
}
