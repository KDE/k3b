/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009-2011 Michal Malek <michalm@jabster.pl>
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

#include "k3bapplication.h"
#include "k3bmediaselectiondialog.h"
#include "k3bdevice.h"
#include "k3bdevicemanager.h"
#include "k3bmsf.h"
#include "k3bmediacache.h"
#include "k3baction.h"
#include "k3bthememanager.h"

#include <KColorScheme>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KLocalizedString>
#include <KIO/Global>
#include <KMessageBox>

#include <QDebug>
#include <QEvent>
#include <QLocale>
#include <QRect>
#include <QTimer>
#include <QBrush>
#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QValidator>
#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMenu>
#include <QStyle>
#include <QStyleOptionProgressBar>
#include <QToolButton>
#include <QToolTip>
#include <QWhatsThis>


class K3b::FillStatusDisplayWidget::Private
{
public:
    K3b::Msf cdSize;
    bool showTime;
    K3b::Doc* doc;
};


K3b::FillStatusDisplayWidget::FillStatusDisplayWidget( K3b::Doc* doc, QWidget* parent )
    : QWidget( parent )
{
    d = new Private();
    d->doc = doc;
    setSizePolicy( QSizePolicy( QSizePolicy::Minimum, QSizePolicy::Preferred ) );
}


K3b::FillStatusDisplayWidget::~FillStatusDisplayWidget()
{
    delete d;
}


const K3b::Msf& K3b::FillStatusDisplayWidget::cdSize() const
{
    return d->cdSize;
}


void K3b::FillStatusDisplayWidget::setShowTime( bool b )
{
    d->showTime = b;
    update();
}


void K3b::FillStatusDisplayWidget::setCdSize( const K3b::Msf& size )
{
    d->cdSize = size;
    update();
}


QSize K3b::FillStatusDisplayWidget::sizeHint() const
{
    return minimumSizeHint();
}


QSize K3b::FillStatusDisplayWidget::minimumSizeHint() const
{
    const int margin = style()->pixelMetric( QStyle::PM_ButtonMargin );
    const QFontMetrics fm( font() );
    return QSize( -1, fm.height() + 2 * margin );
}


void K3b::FillStatusDisplayWidget::mousePressEvent( QMouseEvent* e )
{
    if( e->button() == Qt::RightButton )
        emit contextMenu( e->globalPos() );
}


void K3b::FillStatusDisplayWidget::paintEvent( QPaintEvent* )
{
    QPainter p( this );

    const QPalette::ColorGroup colorGroup = isEnabled() ? QPalette::Normal : QPalette::Disabled;

    const Msf docSize = d->doc->length();
    const Msf cdSize = d->cdSize;
    const Msf maxValue = (cdSize > docSize ? cdSize : docSize) + ( 10*60*75 );
    const Msf tolerance = 60*75;

    QBrush fillBrush;
    if( docSize <= cdSize - tolerance ) {
        fillBrush = KColorScheme( colorGroup, KColorScheme::Selection ).background( KColorScheme::PositiveBackground );
    }
    else if( docSize > cdSize + tolerance ) {
        fillBrush = KColorScheme( colorGroup, KColorScheme::Selection ).background( KColorScheme::NegativeBackground );
    }
    else {
        fillBrush = KColorScheme( colorGroup, KColorScheme::Selection ).background( KColorScheme::NeutralBackground );
    }

    QPen normalPen = KColorScheme(colorGroup, KColorScheme::Window).foreground(KColorScheme::NormalText).color();
    if (K3b::Theme* theme = k3bappcore->themeManager()->currentTheme()) {
        normalPen.setColor(theme->foregroundColor());
    }
    const QPen fillPen = KColorScheme( colorGroup, KColorScheme::Selection ).foreground( KColorScheme::NormalText ).color();

    QStyleOptionProgressBar sopb;
    sopb.direction = layoutDirection();
    sopb.fontMetrics = fontMetrics();
    sopb.palette = palette();
    sopb.palette.setBrush( QPalette::Highlight, fillBrush );
    QFont overSizeFont(font());
    int fontPointSize = qMax(8, overSizeFont.pointSize() - 4);
    sopb.rect = rect();
    sopb.rect.setY(sopb.rect.y() + fontPointSize);
    sopb.state = isEnabled() ? QStyle::State_Enabled : QStyle::State_None;
    sopb.minimum = 0;
    sopb.maximum = maxValue.totalFrames();
    sopb.progress = docSize.totalFrames();
    style()->drawControl( QStyle::CE_ProgressBar, &sopb, &p );

    QRect barRect = style()->subElementRect(QStyle::SE_ProgressBarContents, &sopb);
    barRect.setY(rect().y());

    // so split width() in maxValue pieces
    double one = (double)barRect.width() / (double)maxValue.totalFrames();

    QRect crect( barRect );
    crect.setWidth( (int)(one*(double)docSize.totalFrames()) );

    // ====================================================================================
    // Now the colored bar is painted
    // Continue with the texts
    // ====================================================================================

    // first we determine the text to display
    // ====================================================================================
    QString docSizeText;
    if( d->showTime )
        docSizeText = i18n("%1 min", d->doc->length().toString(false));
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

    // calculate the medium size marker
    // ====================================================================================
    int mediumSizeMarkerPos = barRect.left() + (int)(one*cdSize.lba());
    QPoint mediumSizeMarkerFrom( mediumSizeMarkerPos, barRect.bottom() );
    QPoint mediumSizeMarkerTo( mediumSizeMarkerPos, barRect.top() + barRect.height()/2 );
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

    // calculate the over size text
    // ====================================================================================
    overSizeFont.setPointSize(fontPointSize);
    overSizeFont.setBold(false);

    QRect overSizeTextRect( barRect );
    int overSizeTextLength = QFontMetrics(overSizeFont).width(overSizeText);
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

    QRect docTextRect( barRect );
    docTextRect.setLeft( docSizeTextPos );

    // Draw the fill part
    p.setPen( fillPen );
    p.setClipRect( QStyle::visualRect( layoutDirection(), barRect, crect ) );
    p.drawLine( QStyle::visualPos( layoutDirection(), barRect, mediumSizeMarkerFrom ),
                QStyle::visualPos( layoutDirection(), barRect, mediumSizeMarkerTo ) );
    p.drawText( QStyle::visualRect( layoutDirection(), barRect, docTextRect ),
                QStyle::visualAlignment( layoutDirection(), Qt::AlignLeft | Qt::AlignVCenter ), docSizeText );
    p.setFont(overSizeFont);
    p.drawText( QStyle::visualRect( layoutDirection(), barRect, overSizeTextRect ),
                QStyle::visualAlignment( layoutDirection(), Qt::AlignLeft | Qt::AlignVCenter ), overSizeText );

    // Draw the remain part
    p.setPen( normalPen );
    p.setClipRect( QStyle::visualRect( layoutDirection(), barRect,
                                       QRect( crect.right(), barRect.top(), barRect.width()-crect.width(), barRect.height() ) ) );
    p.drawLine( QStyle::visualPos( layoutDirection(), barRect, mediumSizeMarkerFrom ),
                QStyle::visualPos( layoutDirection(), barRect, mediumSizeMarkerTo ) );
    p.setFont( font() );
    p.drawText( QStyle::visualRect( layoutDirection(), barRect, docTextRect ),
                QStyle::visualAlignment( layoutDirection(), Qt::AlignLeft | Qt::AlignVCenter ), docSizeText );
    p.setFont(overSizeFont);
    p.drawText( QStyle::visualRect( layoutDirection(), barRect, overSizeTextRect ),
                QStyle::visualAlignment( layoutDirection(), Qt::AlignLeft | Qt::AlignVCenter ), overSizeText );
    // ====================================================================================
}



// ----------------------------------------------------------------------------------------------------



class K3b::FillStatusDisplay::Private
{
public:
    KActionCollection* actionCollection;
    QAction* actionShowMinutes;
    QAction* actionShowMegs;
    QActionGroup* cdSizeGroup;
    QAction* actionAuto;
    QAction* action74Min;
    QAction* action80Min;
    QAction* action100Min;
    QAction* actionDvd4_7GB;
    QAction* actionDvdDoubleLayer;
    QAction* actionBD25;
    QAction* actionBD50;

    QAction* actionCustomSize;
    QAction* actionDetermineSize;
    QAction* actionSaveUserDefaults;
    QAction* actionLoadUserDefaults;

    QMenu* popup;

    QToolButton* buttonMenu;

    K3b::FillStatusDisplayWidget* displayWidget;

    bool showTime;

    K3b::Doc* doc;

    QTimer updateTimer;

    void setCdSize( const K3b::Msf& size );
};


void K3b::FillStatusDisplay::Private::setCdSize( const K3b::Msf& size )
{
    // Remove check mark from the currently checked action
    if( QAction* checked = cdSizeGroup->checkedAction() ) {
        checked->setChecked( false );
    }

    switch( size.totalFrames() ) {
        case MediaSizeCd74Min:
        case 650*512:
            displayWidget->setCdSize( MediaSizeCd74Min );
            action74Min->setChecked( true );
            break;
        case MediaSizeCd80Min:
        case 700*512:
            displayWidget->setCdSize( MediaSizeCd80Min );
            action80Min->setChecked( true );
            break;
        case MediaSizeCd100Min:
        case 880*512:
            displayWidget->setCdSize( MediaSizeCd100Min );
            action100Min->setChecked( true );
            break;
        case MediaSizeDvd4Gb:
        case 2306867: // rounded 4.4*1024*512
            displayWidget->setCdSize( MediaSizeDvd4Gb );
            actionDvd4_7GB->setChecked( true );
            break;
        case MediaSizeDvd8Gb:
        case 8*1024*512:
            displayWidget->setCdSize( MediaSizeDvd8Gb );
            actionDvdDoubleLayer->setChecked( true );
            break;
        case MediaSizeBluRay25Gb:
        //case 25*1024*512:
            displayWidget->setCdSize( MediaSizeBluRay25Gb );
            actionBD25->setChecked( true );
            break;
        case MediaSizeBluRay50Gb:
        //case 50*1024*512:
            displayWidget->setCdSize( MediaSizeBluRay50Gb );
            actionBD50->setChecked( true );
            break;
        default:
            displayWidget->setCdSize( size );
            break;
    }
}


K3b::FillStatusDisplay::FillStatusDisplay( K3b::Doc* doc, QWidget *parent )
    : QFrame(parent)
{
    d = new Private;
    d->doc = doc;

    d->displayWidget = new K3b::FillStatusDisplayWidget( doc, this );
    d->buttonMenu = new QToolButton( this );
    d->buttonMenu->setIcon( QIcon::fromTheme( "configure" ) );
    d->buttonMenu->setAutoRaise( true );
    d->buttonMenu->setToolTip( i18n( "Set medium size" ) );
    connect( d->buttonMenu, SIGNAL(clicked()), this, SLOT(slotMenuButtonClicked()) );

    QHBoxLayout* layout = new QHBoxLayout( this );
    layout->setSpacing( frameWidth() );
    layout->setContentsMargins( frameWidth(), frameWidth(), frameWidth(), frameWidth() );
    layout->addWidget( d->displayWidget, 1, Qt::AlignVCenter );
    layout->addWidget( d->buttonMenu );

    setupPopupMenu();

    connect( d->doc, SIGNAL(changed()), this, SLOT(slotDocChanged()) );
    connect( &d->updateTimer, SIGNAL(timeout()), this, SLOT(slotUpdateDisplay()) );
    connect( k3bappcore->mediaCache(), SIGNAL(mediumChanged(K3b::Device::Device*)),
             this, SLOT(slotMediumChanged(K3b::Device::Device*)) );

    slotLoadUserDefaults();
}

K3b::FillStatusDisplay::~FillStatusDisplay()
{
    delete d;
}


void K3b::FillStatusDisplay::setupPopupMenu()
{
    d->actionCollection = new KActionCollection( this );

    // we use a nother popup for the dvd sizes
    d->popup = new QMenu( this );

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
    d->actionBD25 = K3b::createToggleAction( this, KIO::convertSizeFromKiB( 25*1024*1024 ), 0, 0, this, SLOT(slotBD25()),
                                             d->actionCollection, "fillstatus_bd_25" );
    d->actionBD50 = K3b::createToggleAction( this, KIO::convertSizeFromKiB( 50*1024*1024 ), 0, 0, this, SLOT(slotBD50()),
                                             d->actionCollection, "fillstatus_bd_50" );

    d->actionCustomSize = K3b::createAction( this, i18n("Custom..."), 0, 0, this, SLOT(slotCustomSize()),
                                             d->actionCollection, "fillstatus_custom_size" );
    d->actionDetermineSize = K3b::createAction( this, i18n("From Medium..."), "media-optical", 0,
                                                this, SLOT(slotDetermineSize()),
                                                d->actionCollection, "fillstatus_size_from_disk" );

    QActionGroup* showSizeInGroup = new QActionGroup( this );
    showSizeInGroup->addAction( d->actionShowMegs );
    showSizeInGroup->addAction( d->actionShowMinutes );

    d->cdSizeGroup = new QActionGroup( this );
    d->cdSizeGroup->addAction( d->actionAuto );
    d->cdSizeGroup->addAction( d->action74Min );
    d->cdSizeGroup->addAction( d->action80Min );
    d->cdSizeGroup->addAction( d->action100Min );
    d->cdSizeGroup->addAction( d->actionDvd4_7GB );
    d->cdSizeGroup->addAction( d->actionDvdDoubleLayer );
    d->cdSizeGroup->addAction( d->actionBD25 );
    d->cdSizeGroup->addAction( d->actionBD50 );

    d->actionLoadUserDefaults = K3b::createAction( this, i18n("User Defaults"), "", 0,
                                                   this, SLOT(slotLoadUserDefaults()),
                                                   d->actionCollection, "load_user_defaults" );
    d->actionSaveUserDefaults = K3b::createAction( this, i18n("Save User Defaults"), "", 0,
                                                   this, SLOT(slotSaveUserDefaults()),
                                                   d->actionCollection, "save_user_defaults" );

    QAction* dvdSizeInfoAction = K3b::createAction( this, i18n("Why 4.4 instead of 4.7?"), "", 0,
                                                    this, SLOT(slotWhy44()),
                                                    d->actionCollection, "why_44_gb" );

    d->popup->addSection( i18n("Show Size In") );
    d->popup->addAction( d->actionShowMinutes );
    d->popup->addAction( d->actionShowMegs );
    d->popup->addSeparator();
    d->popup->addAction( d->actionAuto );
    if ( d->doc->supportedMediaTypes() & K3b::Device::MEDIA_CD_ALL ) {
        d->popup->addSection( i18n("CD Size") );
        d->popup->addAction( d->action74Min );
        d->popup->addAction( d->action80Min );
        d->popup->addAction( d->action100Min );
    }
    if ( d->doc->supportedMediaTypes() & K3b::Device::MEDIA_DVD_ALL ) {
        d->popup->addSection( i18n("DVD Size") );
        d->popup->addAction( dvdSizeInfoAction );
        d->popup->addAction( d->actionDvd4_7GB );
        d->popup->addAction( d->actionDvdDoubleLayer );
    }
    if ( d->doc->supportedMediaTypes() & K3b::Device::MEDIA_BD_ALL ) {
        d->popup->addSection( i18n("Blu-ray Size") );
        d->popup->addAction( d->actionBD25 );
        d->popup->addAction( d->actionBD50 );
    }
    d->popup->addSeparator();
    d->popup->addAction( d->actionCustomSize );
    d->popup->addAction( d->actionDetermineSize );
    d->popup->addSeparator();
    d->popup->addAction( d->actionLoadUserDefaults );
    d->popup->addAction( d->actionSaveUserDefaults );

    connect( d->displayWidget, SIGNAL(contextMenu(QPoint)), this, SLOT(slotPopupMenu(QPoint)) );
}


void K3b::FillStatusDisplay::showSize()
{
    d->actionShowMegs->setChecked( true );

    d->action74Min->setText( i18n("%1 MB",650) );
    d->action80Min->setText( i18n("%1 MB",700) );
    d->action100Min->setText( i18n("%1 MB",880) );

    d->showTime = false;
    d->displayWidget->setShowTime(false);
}

void K3b::FillStatusDisplay::showTime()
{
    d->actionShowMinutes->setChecked( true );

    d->action74Min->setText( i18np("unused", "%1 minutes", 74) );
    d->action80Min->setText( i18np("unused", "%1 minutes", 80) );
    d->action100Min->setText( i18np("unused", "%1 minutes", 100) );

    d->showTime = true;
    d->displayWidget->setShowTime(true);
}


void K3b::FillStatusDisplay::slotAutoSize()
{
    slotMediumChanged( 0 );
}


void K3b::FillStatusDisplay::slot74Minutes()
{
    d->displayWidget->setCdSize( K3b::MediaSizeCd74Min );
}


void K3b::FillStatusDisplay::slot80Minutes()
{
    d->displayWidget->setCdSize( K3b::MediaSizeCd80Min );
}


void K3b::FillStatusDisplay::slot100Minutes()
{
    d->displayWidget->setCdSize( K3b::MediaSizeCd100Min );
}


void K3b::FillStatusDisplay::slotDvd4_7GB()
{
    d->displayWidget->setCdSize( K3b::MediaSizeDvd4Gb );
}


void K3b::FillStatusDisplay::slotDvdDoubleLayer()
{
    d->displayWidget->setCdSize( K3b::MediaSizeDvd8Gb );
}


void K3b::FillStatusDisplay::slotBD25()
{
    d->displayWidget->setCdSize( K3b::MediaSizeBluRay25Gb );
}


void K3b::FillStatusDisplay::slotBD50()
{
    d->displayWidget->setCdSize( K3b::MediaSizeBluRay50Gb );
}


void K3b::FillStatusDisplay::slotWhy44()
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


void K3b::FillStatusDisplay::slotCustomSize()
{
    // allow the units to be translated
    QString gbS = i18n("GB");
    QString mbS = i18n("MB");
    QString minS = i18n("min");

    QLocale const locale = QLocale::system();

    // we certainly do not have BD- or HD-DVD-only projects
    QString defaultCustom;
    if( d->doc->supportedMediaTypes() & K3b::Device::MEDIA_CD_ALL ) {
        defaultCustom = d->showTime ? QString("74") + minS : QString("650") + mbS;
    }
    else {
        defaultCustom = locale.toString(4.4,'g',1) + gbS;
    }

    QRegExp rx( QString("(\\d+\\") + locale.decimalPoint() + "?\\d*)(" + gbS + '|' + mbS + '|' + minS + ")?" );
    QRegExpValidator validator( rx, this );
    bool ok;
    QString size = QInputDialog::getText( this,
                                          i18n("Custom Size"),
                                          i18n("<p>Please specify the size of the medium. Use suffixes <b>GB</b>,<b>MB</b>, "
                                               "and <b>min</b> for <em>gigabytes</em>, <em>megabytes</em>, and <em>minutes</em>"
                                               " respectively."),
                                          QLineEdit::Normal,
                                          defaultCustom,
                                          &ok );

    int validatorPos;
    if( ok && validator.validate( size, validatorPos ) ) {
        // determine size
        if( rx.exactMatch( size ) ) {
            QString valStr = rx.cap(1);
            if( valStr.endsWith( locale.decimalPoint() ) )
                valStr += '0';
            double val = locale.toDouble( valStr, &ok );
            if( ok ) {
                QString s = rx.cap(2);
                if( s == gbS )
                    val *= 1024*512;
                else if( s == mbS || (s.isEmpty() && !d->showTime) )
                    val *= 512;
                else
                    val *= 60*75;
                d->setCdSize( static_cast<int>( val ) );
            }
        }
    }
}


void K3b::FillStatusDisplay::slotMenuButtonClicked()
{
    QRect alignedMenu = style()->alignedRect( layoutDirection(), Qt::AlignRight, d->popup->sizeHint(), d->buttonMenu->frameGeometry() );
    alignedMenu.moveBottom( d->buttonMenu->frameGeometry().top() - 1 );
    slotPopupMenu( mapToGlobal( alignedMenu.topLeft() ) );
}


void K3b::FillStatusDisplay::slotPopupMenu( const QPoint& p )
{
    d->popup->popup(p);
}


void K3b::FillStatusDisplay::slotDetermineSize()
{
    bool canceled = false;
    K3b::Device::Device* dev = K3b::MediaSelectionDialog::selectMedium( d->doc->supportedMediaTypes(),
                                                                        K3b::Device::STATE_EMPTY|K3b::Device::STATE_INCOMPLETE,
                                                                        K3b::Medium::ContentAll,
                                                                        parentWidget(),
                                                                        QString(), QString(), &canceled );

    if( dev ) {
        K3b::Msf size = k3bappcore->mediaCache()->diskInfo( dev ).capacity();
        if( size > 0 )
            d->setCdSize( size );
        else
            KMessageBox::error( parentWidget(), i18n("Medium is not empty.") );
    }
    else if( !canceled )
        KMessageBox::error( parentWidget(), i18n("No usable medium found.") );
}


void K3b::FillStatusDisplay::slotLoadUserDefaults()
{
    // load project specific values
    KConfigGroup c( KSharedConfig::openConfig(), "default " + d->doc->typeString() + " settings" );

    // defaults to megabytes
    d->showTime = c.readEntry( "show minutes", false );
    d->displayWidget->setShowTime(d->showTime);
    d->actionShowMegs->setChecked( !d->showTime );
    d->actionShowMinutes->setChecked( d->showTime );

    long size = c.readEntry( "default media size", 0 );

    // Remove check mark from current checked action
    if( QAction* checked = d->cdSizeGroup->checkedAction() ) {
        checked->setChecked( false );
    }

    switch( size ) {
    case 0:
        // automatic mode
        d->actionAuto->setChecked( true );
        break;
    case MediaSizeCd74Min:
        d->action74Min->setChecked( true );
        break;
    case MediaSizeCd80Min:
        d->action80Min->setChecked( true );
        break;
    case MediaSizeCd100Min:
        d->action100Min->setChecked( true );
        break;
    case MediaSizeDvd4Gb:
        d->actionDvd4_7GB->setChecked( true );
        break;
    case MediaSizeDvd8Gb:
        d->actionDvdDoubleLayer->setChecked( true );
        break;
    case MediaSizeBluRay25Gb:
        d->actionBD25->setChecked( true );
        break;
    case MediaSizeBluRay50Gb:
        d->actionBD50->setChecked( true );
        break;
    default:
        break;
    }

    if( size == 0 ) {
        slotMediumChanged( 0 );
    }
    else {
        d->displayWidget->setCdSize( size );
    }
}


void K3b::FillStatusDisplay::slotMediumChanged( K3b::Device::Device* )
{
    if( d->actionAuto->isChecked() ) {
        //
        // now search for a usable medium
        // if we find exactly one usable or multiple with the same size
        // we use that size
        //
        K3b::Medium autoSelectedMedium;
        QList<K3b::Device::Device*> devs = k3bcore->deviceManager()->burningDevices();

        Q_FOREACH( K3b::Device::Device* dev, devs ) {
            const K3b::Medium medium = k3bappcore->mediaCache()->medium( dev );

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
                            autoSelectedMedium = K3b::Medium();
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
            if( ( d->doc->supportedMediaTypes() & K3b::Device::MEDIA_CD_ALL &&
                  d->doc->length().lba() <= MediaSizeCd80Min ) ||
                !( d->doc->supportedMediaTypes() & ( K3b::Device::MEDIA_DVD_ALL|K3b::Device::MEDIA_BD_ALL ) ) ||
                ( !haveDVD && !haveBD ) ) {
                d->displayWidget->setCdSize( MediaSizeCd80Min );
            }
            else if ( haveDVD && (
                          ( d->doc->supportedMediaTypes() & K3b::Device::MEDIA_DVD_ALL &&
                            d->doc->length().lba() <= MediaSizeDvd8Gb ) ||
                          !( d->doc->supportedMediaTypes() & K3b::Device::MEDIA_BD_ALL ) ||
                          !haveBD ) ) {
                if( d->doc->length().lba() > MediaSizeDvd4Gb )
                    d->displayWidget->setCdSize( MediaSizeDvd8Gb );
                else
                    d->displayWidget->setCdSize( MediaSizeDvd4Gb );
            }
            else if ( d->doc->length().lba() <= MediaSizeBluRay25Gb ) {
                d->displayWidget->setCdSize( MediaSizeBluRay25Gb );
            }
            else {
                d->displayWidget->setCdSize( MediaSizeBluRay50Gb );
            }
        }
    }
}


void K3b::FillStatusDisplay::slotSaveUserDefaults()
{
    // save project specific values
    KConfigGroup c( KSharedConfig::openConfig(), "default " + d->doc->typeString() + " settings" );

    c.writeEntry( "show minutes", d->showTime );
    c.writeEntry( "default media size", d->actionAuto->isChecked() ? 0 : d->displayWidget->cdSize().lba() );
}


void K3b::FillStatusDisplay::slotUpdateDisplay()
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


void K3b::FillStatusDisplay::slotDocChanged()
{
    // cache updates
    if( !d->updateTimer.isActive() ) {
        slotUpdateDisplay();
        d->updateTimer.setSingleShot( false );
        d->updateTimer.start( 500 );
    }
}


bool K3b::FillStatusDisplay::event( QEvent* event )
{
    if ( event->type() == QEvent::ToolTip ) {
        QHelpEvent* he = ( QHelpEvent* )event;

        QToolTip::showText( he->globalPos(),
                            KIO::convertSize( d->doc->size() ) +
                            " (" + QLocale::system().toString( d->doc->size() ) + "), " +
                            i18n("%1 min", d->doc->length().toString(false)) +
                            " (" + i18n("Right click for media sizes") + ')');

        event->accept();

        return true;
    }

    return QFrame::event( event );
}


