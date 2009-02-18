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


#include "k3bfillstatusdisplay.h"
#include "k3bdoc.h"

#include <k3bapplication.h>
#include <k3bmediaselectiondialog.h>
#include <k3bdevice.h>
#include <k3bdevicemanager.h>
#include <k3bmsf.h>
#include <k3bradioaction.h>
#include <k3bmediacache.h>
#include <k3baction.h>

#include <qevent.h>
#include <qpainter.h>
#include <qcolor.h>
#include <qrect.h>
#include <qfont.h>
#include <qfontmetrics.h>
#include <qvalidator.h>
#include <qtoolbutton.h>
#include <qtooltip.h>
#include <qlayout.h>

#include <qtimer.h>
#include <QGridLayout>
#include <QPixmap>
#include <QFrame>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QLinearGradient>
#include <QBrush>
#include <QWhatsThis>

#include <kaction.h>
#include <kmenu.h>
#include <klocale.h>
#include <kinputdialog.h>
#include <kconfiggroup.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kio/global.h>
#include <kmessagebox.h>
#include <kglobal.h>


static const int DEFAULT_CD_SIZE_74 = 74*60*75;
static const int DEFAULT_CD_SIZE_80 = 80*60*75;
static const int DEFAULT_CD_SIZE_100 = 100*60*75;
static const int DEFAULT_DVD_SIZE_4_4 = 2295104;
static const int DEFAULT_DVD_SIZE_8_0 = 4173824;

// FIXME: get the proper BD sizes
static const int DEFAULT_BD_SIZE_25 = 13107200;
static const int DEFAULT_BD_SIZE_50 = 26214400;


class K3bFillStatusDisplayWidget::Private
{
public:
    K3b::Msf cdSize;
    bool showTime;
    K3bDoc* doc;
};


K3bFillStatusDisplayWidget::K3bFillStatusDisplayWidget( K3bDoc* doc, QWidget* parent )
    : QWidget( parent )
{
    d = new Private();
    d->doc = doc;
    setSizePolicy( QSizePolicy( QSizePolicy::Minimum, QSizePolicy::Preferred ) );
}


K3bFillStatusDisplayWidget::~K3bFillStatusDisplayWidget()
{
    delete d;
}


const K3b::Msf& K3bFillStatusDisplayWidget::cdSize() const
{
    return d->cdSize;
}


void K3bFillStatusDisplayWidget::setShowTime( bool b )
{
    d->showTime = b;
    update();
}


void K3bFillStatusDisplayWidget::setCdSize( const K3b::Msf& size )
{
    d->cdSize = size;
    update();
}


QSize K3bFillStatusDisplayWidget::sizeHint() const
{
    return minimumSizeHint();
}


QSize K3bFillStatusDisplayWidget::minimumSizeHint() const
{
    int margin = 2;
    QFontMetrics fm( font() );
    return QSize( -1, fm.height() + 2 * margin );
}


void K3bFillStatusDisplayWidget::mousePressEvent( QMouseEvent* e )
{
    if( e->button() == Qt::RightButton )
        emit contextMenu( e->globalPos() );
}


void K3bFillStatusDisplayWidget::paintEvent( QPaintEvent* )
{
    QPainter p( this );
    p.setPen( Qt::black ); // we use a fixed bar color (which is not very nice btw, so we also fix the text color)

    K3b::Msf docSize;
    K3b::Msf cdSize;
    K3b::Msf maxValue;
    K3b::Msf tolerance;

    docSize = d->doc->length();
    cdSize = d->cdSize;
    maxValue = (cdSize > docSize ? cdSize : docSize) + ( 10*60*75 );
    tolerance = 60*75;

    // so split width() in maxValue pieces
    double one = (double)rect().width() / (double)maxValue.totalFrames();
    QRect crect( rect() );
    crect.setWidth( (int)(one*(double)docSize.totalFrames()) );

    p.setClipping(true);
    p.setClipRect(crect);

    p.fillRect( crect, Qt::green );

    QRect oversizeRect(crect);

    // draw red if docSize > cdSize + tolerance
    if( docSize > cdSize + tolerance ) {
        oversizeRect.setLeft( oversizeRect.left() + (int)(one * (cdSize - tolerance).totalFrames()) );
        p.fillRect( oversizeRect, Qt::red );
        QLinearGradient gradient( QPoint( oversizeRect.left() - rect().height(), 0 ),
                                  QPoint( oversizeRect.left() + rect().height(), 0 ) );
        gradient.setColorAt( 0.1, Qt::green );
        gradient.setColorAt( 0.5, Qt::yellow );
        gradient.setColorAt( 0.9, Qt::red );
        p.fillRect( oversizeRect.left() - rect().height(), 0, rect().height()*2, rect().height(), gradient );
    }

    // draw yellow if cdSize - tolerance < docSize
    else if( docSize > cdSize - tolerance ) {
        oversizeRect.setLeft( oversizeRect.left() + (int)(one * (cdSize - tolerance).lba()) );
        p.fillRect( oversizeRect, Qt::yellow );
        QLinearGradient gradient( QPoint( oversizeRect.left() - rect().height(), 0 ),
                                  QPoint( oversizeRect.left() + rect().height(), 0 ) );
        gradient.setColorAt( 0.1, Qt::green );
        gradient.setColorAt( 0.9, Qt::yellow );
        p.fillRect( oversizeRect.left() - rect().height(), 0, rect().height()*2, rect().height(), gradient );
    }

    p.setClipping(false);

    // ====================================================================================
    // Now the colored bar is painted
    // Continue with the texts
    // ====================================================================================

    // first we determine the text to display
    // ====================================================================================
    QString docSizeText;
    if( d->showTime )
        docSizeText = d->doc->length().toString(false) + " " + i18n("min");
    else
        docSizeText = KIO::convertSize( d->doc->size() );

    QString overSizeText;
    if( d->cdSize.mode1Bytes() >= d->doc->size() )
        overSizeText = i18n("Available: %1 of %2",
                            d->showTime
                            ? i18n("%1 min", (cdSize - d->doc->length()).toString(false) )
                            : KIO::convertSize( (cdSize - d->doc->length()).mode1Bytes() ),
                            d->showTime
                            ? i18n("%1 min", cdSize.toString(false))
                            : KIO::convertSize( cdSize.mode1Bytes() ) );
    else
        overSizeText = i18n("Capacity exceeded by %1",
                            d->showTime
                            ? i18n("%1 min", (d->doc->length() - cdSize ).toString(false))
                            : KIO::convertSize( (long long)d->doc->size() - cdSize.mode1Bytes() ) );
    // ====================================================================================

    // draw the medium size marker
    // ====================================================================================
    int mediumSizeMarkerPos = rect().left() + (int)(one*cdSize.lba());
    p.drawLine( mediumSizeMarkerPos, rect().bottom(),
                mediumSizeMarkerPos, rect().top() + ((rect().bottom()-rect().top())/2) );
    // ====================================================================================



    // we want to draw the docSizeText centered in the filled area
    // if there is not enough space we just align it left
    // ====================================================================================
    int docSizeTextPos = 0;
    int docSizeTextLength = fontMetrics().width(docSizeText);
    if( docSizeTextLength + 5 > crect.width() ) {
        docSizeTextPos = crect.left() + 5; // a little margin
    }
    else {
        docSizeTextPos = ( crect.width() - docSizeTextLength ) / 2;

        // make sure the text does not cross the medium size marker
        if( docSizeTextPos <= mediumSizeMarkerPos && mediumSizeMarkerPos <= docSizeTextPos + docSizeTextLength )
            docSizeTextPos = qMax( crect.left() + 5, mediumSizeMarkerPos - docSizeTextLength - 5 );
    }
    // ====================================================================================

    // draw the over size text
    // ====================================================================================
    QFont fnt(font());
    fnt.setPointSize( qMax( 8, fnt.pointSize()-4 ) );
    fnt.setBold(false);

    QRect overSizeTextRect( rect() );
    int overSizeTextLength = QFontMetrics(fnt).width(overSizeText);
    if( overSizeTextLength + 5 > overSizeTextRect.width() - (int)(one*cdSize.totalFrames()) ) {
        // we don't have enough space on the right, so we paint to the left of the line
        overSizeTextRect.setLeft( (int)(one*cdSize.totalFrames()) - overSizeTextLength - 5 );
    }
    else {
        overSizeTextRect.setLeft( mediumSizeMarkerPos + 5 );
    }

    // make sure the two text do not overlap (this does not cover all cases though)
    if( overSizeTextRect.left() < docSizeTextPos + docSizeTextLength )
        docSizeTextPos = qMax( crect.left() + 5, qMin( overSizeTextRect.left() - docSizeTextLength - 5, mediumSizeMarkerPos - docSizeTextLength - 5 ) );

    QRect docTextRect( rect() );
    docTextRect.setLeft( docSizeTextPos );
    p.drawText( docTextRect, Qt::AlignLeft | Qt::AlignVCenter, docSizeText );

    p.setFont(fnt);
    p.drawText( overSizeTextRect, Qt::AlignLeft | Qt::AlignVCenter, overSizeText );
    // ====================================================================================
}



// ----------------------------------------------------------------------------------------------------



class K3bFillStatusDisplay::Private
{
public:
    KActionCollection* actionCollection;
    KAction* actionShowMinutes;
    KAction* actionShowMegs;
    KAction* actionAuto;
    KAction* action74Min;
    KAction* action80Min;
    KAction* action100Min;
    KAction* actionDvd4_7GB;
    KAction* actionDvdDoubleLayer;
    KAction* actionBD25;
    KAction* actionBD50;

    KAction* actionCustomSize;
    KAction* actionDetermineSize;
    KAction* actionSaveUserDefaults;
    KAction* actionLoadUserDefaults;

    KMenu* popup;

    QToolButton* buttonMenu;

    K3bFillStatusDisplayWidget* displayWidget;

    bool showTime;

    K3bDoc* doc;

    QTimer updateTimer;
};


K3bFillStatusDisplay::K3bFillStatusDisplay( K3bDoc* doc, QWidget *parent )
    : QFrame(parent)
{
    d = new Private;
    d->doc = doc;

    setFrameStyle( Panel | Sunken );

    d->displayWidget = new K3bFillStatusDisplayWidget( doc, this );
//   d->buttonMenu = new QToolButton( this );
//   d->buttonMenu->setIconSet( SmallIconSet("media-optical") );
//   d->buttonMenu->setAutoRaise(true);
//   d->buttonMenu->setToolTip( i18n("Fill display properties") );
//   connect( d->buttonMenu, SIGNAL(clicked()), this, SLOT(slotMenuButtonClicked()) );

    QGridLayout* layout = new QGridLayout( this );
    layout->setSpacing(5);
    layout->setMargin(frameWidth());
    layout->addWidget( d->displayWidget, 0, 0 );
    //  layout->addWidget( d->buttonMenu, 0, 1 );
    layout->setColumnStretch( 0, 1 );

    setupPopupMenu();

    connect( d->doc, SIGNAL(changed()), this, SLOT(slotDocChanged()) );
    connect( &d->updateTimer, SIGNAL(timeout()), this, SLOT(slotUpdateDisplay()) );
    connect( k3bappcore->mediaCache(), SIGNAL(mediumChanged(K3bDevice::Device*)),
             this, SLOT(slotMediumChanged(K3bDevice::Device*)) );

    slotLoadUserDefaults();
}

K3bFillStatusDisplay::~K3bFillStatusDisplay()
{
    delete d;
}


void K3bFillStatusDisplay::setupPopupMenu()
{
    d->actionCollection = new KActionCollection( this );

    // we use a nother popup for the dvd sizes
    d->popup = new KMenu( this );

    d->actionShowMinutes = K3b::createToggleAction( this, i18n("Minutes"), 0, 0, this, SLOT(showTime()),
                                                    d->actionCollection, "fillstatus_show_minutes" );
    d->actionShowMegs = K3b::createToggleAction( this, i18n("Megabytes"), 0, 0, this, SLOT(showSize()),
                                                 d->actionCollection, "fillstatus_show_megabytes" );

    d->actionAuto = K3b::createToggleAction( this, i18n("Automatic Size"), 0, 0, this, SLOT(slotAutoSize()),
                                             d->actionCollection, "fillstatus_auto" );
    d->action74Min = K3b::createToggleAction( this, i18n("%1 MB",650), 0, 0, this, SLOT(slot74Minutes()),
                                              d->actionCollection, "fillstatus_74minutes" );
    d->action80Min = K3b::createToggleAction( this, i18n("%1 MB",700), 0, 0, this, SLOT(slot80Minutes()),
                                              d->actionCollection, "fillstatus_80minutes" );
    d->action100Min = K3b::createToggleAction( this, i18n("%1 MB",880), 0, 0, this, SLOT(slot100Minutes()),
                                               d->actionCollection, "fillstatus_100minutes" );
    d->actionDvd4_7GB = K3b::createToggleAction( this, KIO::convertSizeFromKiB((int)(4.4*1024.0*1024.0)), 0, 0, this, SLOT(slotDvd4_7GB()),
                                                 d->actionCollection, "fillstatus_dvd_4_7gb" );
    d->actionDvdDoubleLayer = K3b::createToggleAction( this, KIO::convertSizeFromKiB((int)(8.0*1024.0*1024.0)),
                                                       0, 0, this, SLOT(slotDvdDoubleLayer()),
                                                       d->actionCollection, "fillstatus_dvd_double_layer" );
    d->actionBD25 = K3b::createToggleAction( this, KIO::convertSizeFromKiB( 25*1024*1024 ), 0, 0, this, SLOT( slotBD25() ),
                                             d->actionCollection, "fillstatus_bd_25" );
    d->actionBD50 = K3b::createToggleAction( this, KIO::convertSizeFromKiB( 50*1024*1024 ), 0, 0, this, SLOT( slotBD50() ),
                                             d->actionCollection, "fillstatus_bd_50" );

    d->actionCustomSize = K3b::createToggleAction( this, i18n("Custom..."), 0, 0, this, SLOT(slotCustomSize()),
                                                   d->actionCollection, "fillstatus_custom_size" );
#ifdef __GNUC__
#warning setAlwaysEmitActivated
#endif
    //    d->actionCustomSize->setAlwaysEmitActivated(true);
    d->actionDetermineSize = K3b::createToggleAction( this, i18n("From Medium..."), "media-optical", 0,
                                                      this, SLOT(slotDetermineSize()),
                                                 d->actionCollection, "fillstatus_size_from_disk" );
//    d->actionDetermineSize->setAlwaysEmitActivated(true);

    QActionGroup* showSizeInGroup = new QActionGroup( this );
    showSizeInGroup->addAction( d->actionShowMegs );
    showSizeInGroup->addAction( d->actionShowMinutes );

    QActionGroup* cdSizeGroup = new QActionGroup( this );
    cdSizeGroup->addAction( d->actionAuto );
    cdSizeGroup->addAction( d->action74Min );
    cdSizeGroup->addAction( d->action80Min );
    cdSizeGroup->addAction( d->action100Min );
    cdSizeGroup->addAction( d->actionDvd4_7GB );
    cdSizeGroup->addAction( d->actionDvdDoubleLayer );
    cdSizeGroup->addAction( d->actionBD25 );
    cdSizeGroup->addAction( d->actionBD50 );
    cdSizeGroup->addAction( d->actionCustomSize );
    cdSizeGroup->addAction( d->actionDetermineSize );

    d->actionLoadUserDefaults = K3b::createAction( this, i18n("User Defaults"), "", 0,
                                                   this, SLOT(slotLoadUserDefaults()),
                                                   d->actionCollection, "load_user_defaults" );
    d->actionSaveUserDefaults = K3b::createAction( this, i18n("Save User Defaults"), "", 0,
                                                   this, SLOT(slotSaveUserDefaults()),
                                                   d->actionCollection, "save_user_defaults" );

    KAction* dvdSizeInfoAction = K3b::createAction( this, i18n("Why 4.4 instead of 4.7?"), "", 0,
                                                    this, SLOT(slotWhy44()),
                                                    d->actionCollection, "why_44_gb" );

    d->popup->addTitle( i18n("Show Size In") );
    d->popup->addAction( d->actionShowMinutes );
    d->popup->addAction( d->actionShowMegs );
    d->popup->addSeparator();
    d->popup->addAction( d->actionAuto );
    if ( d->doc->supportedMediaTypes() & K3bDevice::MEDIA_CD_ALL ) {
        d->popup->addTitle( i18n("CD Size") );
        d->popup->addAction( d->action74Min );
        d->popup->addAction( d->action80Min );
        d->popup->addAction( d->action100Min );
    }
    if ( d->doc->supportedMediaTypes() & K3bDevice::MEDIA_DVD_ALL ) {
        d->popup->addTitle( i18n("DVD Size") );
        d->popup->addAction( dvdSizeInfoAction );
        d->popup->addAction( d->actionDvd4_7GB );
        d->popup->addAction( d->actionDvdDoubleLayer );
    }
    if ( d->doc->supportedMediaTypes() & K3bDevice::MEDIA_BD_ALL ) {
        d->popup->addTitle( i18n("Blu-Ray Size") );
        d->popup->addAction( d->actionBD25 );
        d->popup->addAction( d->actionBD50 );
    }
    d->popup->addSeparator();
    d->popup->addAction( d->actionCustomSize );
    d->popup->addAction( d->actionDetermineSize );
    d->popup->addSeparator();
    d->popup->addAction( d->actionLoadUserDefaults );
    d->popup->addAction( d->actionSaveUserDefaults );

    connect( d->displayWidget, SIGNAL(contextMenu(const QPoint&)), this, SLOT(slotPopupMenu(const QPoint&)) );
}


void K3bFillStatusDisplay::showSize()
{
    d->actionShowMegs->setChecked( true );

    d->action74Min->setText( i18n("%1 MB",650) );
    d->action80Min->setText( i18n("%1 MB",700) );
    d->action100Min->setText( i18n("%1 MB",880) );

    d->showTime = false;
    d->displayWidget->setShowTime(false);
}

void K3bFillStatusDisplay::showTime()
{
    d->actionShowMinutes->setChecked( true );

    d->action74Min->setText( i18np("unused", "%1 minutes", 74) );
    d->action80Min->setText( i18np("unused", "%1 minutes", 80) );
    d->action100Min->setText( i18np("unused", "%1 minutes", 100) );

    d->showTime = true;
    d->displayWidget->setShowTime(true);
}


void K3bFillStatusDisplay::slotAutoSize()
{
    slotMediumChanged( 0 );
}


void K3bFillStatusDisplay::slot74Minutes()
{
    d->displayWidget->setCdSize( DEFAULT_CD_SIZE_74 );
}


void K3bFillStatusDisplay::slot80Minutes()
{
    d->displayWidget->setCdSize( DEFAULT_CD_SIZE_80 );
}


void K3bFillStatusDisplay::slot100Minutes()
{
    d->displayWidget->setCdSize( DEFAULT_CD_SIZE_100 );
}


void K3bFillStatusDisplay::slotDvd4_7GB()
{
    d->displayWidget->setCdSize( DEFAULT_DVD_SIZE_4_4 );
}


void K3bFillStatusDisplay::slotDvdDoubleLayer()
{
    d->displayWidget->setCdSize( DEFAULT_DVD_SIZE_8_0 );
}


void K3bFillStatusDisplay::slotBD25()
{
    d->displayWidget->setCdSize( DEFAULT_BD_SIZE_25 );
}


void K3bFillStatusDisplay::slotBD50()
{
    d->displayWidget->setCdSize( DEFAULT_BD_SIZE_50 );
}


void K3bFillStatusDisplay::slotWhy44()
{
    QWhatsThis::showText( QCursor::pos(),
                          i18n("<p><b>Why does K3b offer 4.4 GB and 8.0 GB instead of 4.7 and 8.5 like "
                               "it says on the media?</b>"
                               "<p>A single layer DVD media has a capacity of approximately "
                               "4.4 GB which equals 4.4*1024<sup>3</sup> bytes. Media producers just "
                               "calculate with 1000 instead of 1024 for advertising reasons.<br>"
                               "This results in 4.4*1024<sup>3</sup>/1000<sup>3</sup> = 4.7 GB."),
                          this );
}


void K3bFillStatusDisplay::slotCustomSize()
{
    // allow the units to be translated
    QString gbS = i18n("gb");
    QString mbS = i18n("mb");
    QString minS = i18n("min");

    // we certainly do not have BD- or HD-DVD-only projects
    QString defaultCustom;
    if( d->doc->supportedMediaTypes() & K3bDevice::MEDIA_CD_ALL ) {
        defaultCustom = d->showTime ? QString("74") + minS : QString("650") + mbS;
    }
    else {
        defaultCustom = QString("4%14%2").arg( KGlobal::locale()->decimalSymbol() ).arg( gbS );
    }

    QRegExp rx( "(\\d+\\" + KGlobal::locale()->decimalSymbol() + "?\\d*)(" + gbS + "|" + mbS + "|" + minS + ")?" );
    bool ok;
    QString size = KInputDialog::getText( i18n("Custom Size"),
                                          i18n("<p>Please specify the size of the media. Use suffixes <b>gb</b>,<b>mb</b>, "
                                               "and <b>min</b> for <em>gigabytes</em>, <em>megabytes</em>, and <em>minutes</em>"
                                               " respectively."),
                                          defaultCustom,
                                          &ok,
                                          this,
                                          new QRegExpValidator( rx, this ) );
    if( ok ) {
        // determine size
        if( rx.exactMatch( size ) ) {
            QString valStr = rx.cap(1);
            if( valStr.endsWith( KGlobal::locale()->decimalSymbol() ) )
                valStr += "0";
            double val = KGlobal::locale()->readNumber( valStr, &ok );
            if( ok ) {
                QString s = rx.cap(2);
                if( s == gbS )
                    val *= 1024*512;
                else if( s == mbS || (s.isEmpty() && !d->showTime) )
                    val *= 512;
                else
                    val *= 60*75;
                d->displayWidget->setCdSize( (int)val );
                update();
            }
        }
    }
}


void K3bFillStatusDisplay::slotMenuButtonClicked()
{
    QSize size = d->popup->sizeHint();
    slotPopupMenu( d->buttonMenu->mapToGlobal(QPoint(d->buttonMenu->width(), 0)) +
                   QPoint(-1*size.width(), -1*size.height()) );
}


void K3bFillStatusDisplay::slotPopupMenu( const QPoint& p )
{
    d->popup->popup(p);
}


void K3bFillStatusDisplay::slotDetermineSize()
{
    bool canceled = false;
    K3bDevice::Device* dev = K3bMediaSelectionDialog::selectMedium( d->doc->supportedMediaTypes(),
                                                                    K3bDevice::STATE_EMPTY|K3bDevice::STATE_INCOMPLETE,
                                                                    parentWidget(),
                                                                    QString(), QString(), &canceled );

    if( dev ) {
        K3b::Msf size = k3bappcore->mediaCache()->diskInfo( dev ).capacity();
        if( size > 0 ) {
            d->displayWidget->setCdSize( size );
            d->actionCustomSize->setChecked(true);
            update();
        }
        else
            KMessageBox::error( parentWidget(), i18n("Medium is not empty.") );
    }
    else if( !canceled )
        KMessageBox::error( parentWidget(), i18n("No usable medium found.") );
}


void K3bFillStatusDisplay::slotLoadUserDefaults()
{
    // load project specific values
    KConfigGroup c( KGlobal::config(), "default " + d->doc->typeString() + " settings" );

    // defaults to megabytes
    d->showTime = c.readEntry( "show minutes", false );
    d->displayWidget->setShowTime(d->showTime);
    d->actionShowMegs->setChecked( !d->showTime );
    d->actionShowMinutes->setChecked( d->showTime );

    long size = c.readEntry( "default media size", 0 );

    switch( size ) {
    case 0:
        // automatic mode
        d->actionAuto->setChecked( true );
        break;
    case DEFAULT_CD_SIZE_74:
        d->action74Min->setChecked( true );
        break;
    case DEFAULT_CD_SIZE_80:
        d->action80Min->setChecked( true );
        break;
    case DEFAULT_CD_SIZE_100:
        d->action100Min->setChecked( true );
        break;
    case DEFAULT_DVD_SIZE_4_4:
        d->actionDvd4_7GB->setChecked( true );
        break;
    case DEFAULT_DVD_SIZE_8_0:
        d->actionDvdDoubleLayer->setChecked( true );
        break;
    case DEFAULT_BD_SIZE_25:
        d->actionBD25->setChecked( true );
        break;
    case DEFAULT_BD_SIZE_50:
        d->actionBD50->setChecked( true );
        break;
    default:
        d->actionCustomSize->setChecked( true );
        break;
    }

    if( size == 0 ) {
        slotMediumChanged( 0 );
    }
    else {
        d->displayWidget->setCdSize( size*60*75 );
    }
}


void K3bFillStatusDisplay::slotMediumChanged( K3bDevice::Device* )
{
    if( d->actionAuto->isChecked() ) {
        //
        // now search for a usable medium
        // if we find exactly one usable or multiple with the same size
        // we use that size
        //
        K3bMedium autoSelectedMedium;
        QList<K3bDevice::Device*> devs = k3bcore->deviceManager()->burningDevices();

        Q_FOREACH( K3bDevice::Device* dev, devs ) {
            const K3bMedium medium = k3bappcore->mediaCache()->medium( dev );

            if( ( medium.diskInfo().empty() ||
                  medium.diskInfo().appendable() ||
                  medium.diskInfo().rewritable() ) &&
                ( medium.diskInfo().mediaType() & d->doc->supportedMediaTypes() ) ) {

                // We use a 10% margin to allow the user to fine-tune project sizes
                // However, if we have a bigger medium we always use that
                if ( ( double )d->doc->size() <= ( double )( medium.diskInfo().capacity().mode1Bytes() ) * 1.1 ) {

                    // first usable medium
                    if( !autoSelectedMedium.isValid() ) {
                        autoSelectedMedium = medium;
                    }

                    else {
                        // prefer the medium which can fit the whole doc
                        if ( d->doc->length() <= medium.diskInfo().capacity() &&
                             d->doc->length() > autoSelectedMedium.diskInfo().capacity() ) {
                            autoSelectedMedium = medium;
                        }

                        // roughly compare the sizes of the two usable media. If they match, carry on.
                        else if( medium.diskInfo().capacity().lba()/75/60
                                 != autoSelectedMedium.diskInfo().capacity().lba()/75/60 ) {
                            // different usable media -> fallback
                            autoSelectedMedium = K3bMedium();
                            break;
                        }
                    }
                }
            }
        }

        if( autoSelectedMedium.isValid() ) {
            d->displayWidget->setCdSize( autoSelectedMedium.diskInfo().capacity().lba() );
        }
        else {
            bool haveDVD = !k3bcore->deviceManager()->dvdWriter().isEmpty();
            bool haveBD = !k3bcore->deviceManager()->blueRayWriters().isEmpty();


            // default fallback
            // we do not have BD- or HD-DVD only projects
            if( ( d->doc->supportedMediaTypes() & K3bDevice::MEDIA_CD_ALL &&
                  d->doc->length().lba() <= DEFAULT_CD_SIZE_80 ) ||
                !( d->doc->supportedMediaTypes() & ( K3bDevice::MEDIA_DVD_ALL|K3bDevice::MEDIA_BD_ALL ) ) ||
                ( !haveDVD && !haveBD ) ) {
                d->displayWidget->setCdSize( DEFAULT_CD_SIZE_80 );
            }
            else if ( haveDVD && (
                          ( d->doc->supportedMediaTypes() & K3bDevice::MEDIA_DVD_ALL &&
                            d->doc->length().lba() <= DEFAULT_DVD_SIZE_8_0 ) ||
                          !( d->doc->supportedMediaTypes() & K3bDevice::MEDIA_BD_ALL ) ||
                          !haveBD ) ) {
                if( d->doc->length().lba() > DEFAULT_DVD_SIZE_4_4 )
                    d->displayWidget->setCdSize( DEFAULT_DVD_SIZE_8_0 );
                else
                    d->displayWidget->setCdSize( DEFAULT_DVD_SIZE_4_4 );
            }
            else if ( d->doc->length().lba() <= DEFAULT_BD_SIZE_25 ) {
                d->displayWidget->setCdSize( DEFAULT_BD_SIZE_25 );
            }
            else {
                d->displayWidget->setCdSize( DEFAULT_BD_SIZE_50 );
            }
        }
    }
}


void K3bFillStatusDisplay::slotSaveUserDefaults()
{
    // save project specific values
    KConfigGroup c( KGlobal::config(), "default " + d->doc->typeString() + " settings" );

    c.writeEntry( "show minutes", d->showTime );
    c.writeEntry( "default media size", d->actionAuto->isChecked() ? 0 : d->displayWidget->cdSize().lba() );

    c.sync();
}


void K3bFillStatusDisplay::slotUpdateDisplay()
{
    if( d->actionAuto->isChecked() ) {
        //
        // also update the medium list in case the docs size exceeds the capacity
        //
        slotMediumChanged( 0 );
    }
    else {
        d->displayWidget->update();
    }
}


void K3bFillStatusDisplay::slotDocChanged()
{
    // cache updates
    if( !d->updateTimer.isActive() ) {
        slotUpdateDisplay();
        d->updateTimer.setSingleShot( false );
        d->updateTimer.start( 500 );
    }
}


bool K3bFillStatusDisplay::event( QEvent* event )
{
    if ( event->type() == QEvent::ToolTip ) {
        QHelpEvent* he = ( QHelpEvent* )event;
        QPoint pos = he->pos();

        QToolTip::showText( he->globalPos(),
                            KIO::convertSize( d->doc->size() ) +
                            " (" + KGlobal::locale()->formatNumber( d->doc->size(), 0 ) + "), " +
                            d->doc->length().toString(false) + " " + i18n("min") +
                            " (" + i18n("Right click for media sizes") + ")");

        event->accept();

        return true;
    }

    return QFrame::event( event );
}

#include "k3bfillstatusdisplay.moc"
