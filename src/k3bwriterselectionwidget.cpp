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


#include "k3bwriterselectionwidget.h"
#include "k3bapplication.h"
#include "k3bmediacache.h"

#include <k3bmediaselectioncombobox.h>
#include <k3bdevice.h>
#include <k3bdevicemanager.h>
#include <k3bglobals.h>
#include <k3bcore.h>
#include <k3bintmapcombobox.h>

#include <klocale.h>
#include <kdialog.h>
#include <kconfig.h>
#include <kcombobox.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <kinputdialog.h>

#include <qlabel.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qtooltip.h>
#include <qtoolbutton.h>

#include <qcursor.h>
#include <qapplication.h>


#include <stdlib.h>


namespace {
    int s_autoSpeedValue = 0;
    int s_ignoreSpeedValue = -1;
    int s_moreSpeedValue = -2;
}


class K3bWriterSelectionWidget::MediaSelectionComboBox : public K3bMediaSelectionComboBox
{
public:
    MediaSelectionComboBox( QWidget* parent )
        : K3bMediaSelectionComboBox( parent ),
          m_overrideDevice( 0 ) {
    }

    void setOverrideDevice( K3bDevice::Device* dev, const QString& s, const QString& t ) {
        m_overrideDevice = dev;
        m_overrideString = s;
        m_overrideToolTip = t;
        updateMedia();
    }

    K3bDevice::Device* overrideDevice() const {
        return m_overrideDevice;
    }

protected:
    bool showMedium( const K3bMedium& m ) const {
        return ( m.device() == m_overrideDevice ||
                 K3bMediaSelectionComboBox::showMedium( m ) );
    }

    QString mediumString( const K3bMedium& m ) const {
        if( m.device() == m_overrideDevice )
            return m_overrideString;
        else
            return K3bMediaSelectionComboBox::mediumString( m );
    }

    QString mediumToolTip( const K3bMedium& m ) const {
        if( m.device() == m_overrideDevice )
            return m_overrideToolTip;
        else {
            QString s = K3bMediaSelectionComboBox::mediumToolTip( m );
            if( !m.diskInfo().empty() && !(wantedMediumState() & m.diskInfo().diskState()) )
                s.append( "<p><i>" + i18n("Medium will be overwritten.") + "</i>" );
            return s;
        }
    }

private:
    K3bDevice::Device* m_overrideDevice;
    QString m_overrideString;
    QString m_overrideToolTip;
};


class K3bWriterSelectionWidget::Private
{
public:
    bool forceAutoSpeed;
    bool haveIgnoreSpeed;
    bool haveManualSpeed;

    K3b::WritingApps supportedWritingApps;

    int lastSetSpeed;
};


K3bWriterSelectionWidget::K3bWriterSelectionWidget( QWidget *parent )
    : QWidget( parent )
{
    d = new Private;
    d->forceAutoSpeed = false;
    d->supportedWritingApps = K3b::WRITING_APP_CDRECORD|K3b::WRITING_APP_CDRDAO|K3b::WRITING_APP_GROWISOFS;
    d->lastSetSpeed = -1;

    QGroupBox* groupWriter = new QGroupBox( this );
    groupWriter->setTitle( i18n( "Burn Medium" ) );

    QGridLayout* groupWriterLayout = new QGridLayout( groupWriter );
    groupWriterLayout->setAlignment( Qt::AlignTop );
    groupWriterLayout->setSpacing( KDialog::spacingHint() );
    groupWriterLayout->setMargin( KDialog::marginHint() );

    QLabel* labelSpeed = new QLabel( groupWriter );
    labelSpeed->setText( i18n( "Speed:" ) );

    m_comboSpeed = new K3bIntMapComboBox( groupWriter );

    m_comboMedium = new MediaSelectionComboBox( groupWriter );

    m_writingAppLabel = new QLabel( i18n("Writing app:"), groupWriter );
    m_comboWritingApp = new K3bIntMapComboBox( groupWriter );

    groupWriterLayout->addWidget( m_comboMedium, 0, 0 );
    groupWriterLayout->addWidget( labelSpeed, 0, 1 );
    groupWriterLayout->addWidget( m_comboSpeed, 0, 2 );
    groupWriterLayout->addWidget( m_writingAppLabel, 0, 3 );
    groupWriterLayout->addWidget( m_comboWritingApp, 0, 4 );
    groupWriterLayout->setColumnStretch( 0, 1 );


    QGridLayout* mainLayout = new QGridLayout( this );
    mainLayout->setAlignment( Qt::AlignTop );
    mainLayout->setSpacing( KDialog::spacingHint() );
    mainLayout->setMargin( 0 );

    mainLayout->addWidget( groupWriter, 0, 0 );

    // tab order
    setTabOrder( m_comboMedium, m_comboSpeed );
    setTabOrder( m_comboSpeed, m_comboWritingApp );

    connect( m_comboMedium, SIGNAL(selectionChanged(K3bDevice::Device*)), this, SIGNAL(writerChanged()) );
    connect( m_comboMedium, SIGNAL(selectionChanged(K3bDevice::Device*)),
             this, SIGNAL(writerChanged(K3bDevice::Device*)) );
    connect( m_comboMedium, SIGNAL(newMedia()), this, SIGNAL(newMedia()) );
    connect( m_comboMedium, SIGNAL(newMedium(K3bDevice::Device*)), this, SIGNAL(newMedium(K3bDevice::Device*)) );
    connect( m_comboMedium, SIGNAL(newMedium(K3bDevice::Device*)), this, SLOT(slotNewBurnMedium(K3bDevice::Device*)) );
    connect( m_comboWritingApp, SIGNAL(valueChanged(int)), this, SLOT(slotWritingAppSelected(int)) );
    connect( this, SIGNAL(writerChanged()), SLOT(slotWriterChanged()) );
    connect( m_comboSpeed, SIGNAL(valueChanged(int)), this, SLOT(slotSpeedChanged(int)) );


    m_comboMedium->setToolTip( i18n("The medium that will be used for burning") );
    m_comboSpeed->setToolTip( i18n("The speed at which to burn the medium") );
    m_comboWritingApp->setToolTip( i18n("The external application to actually burn the medium") );

    m_comboMedium->setWhatsThis( i18n("<p>Select the medium that you want to use for burning."
                                      "<p>In most cases there will only be one medium available which "
                                      "does not leave much choice.") );
    m_comboSpeed->setWhatsThis( i18n("<p>Select the speed with which you want to burn."
                                     "<p><b>Auto</b><br>"
                                     "This will choose the maximum writing speed possible with the used "
                                     "medium. "
                                     "This is the recommended selection for most media.</p>"
                                     "<p><b>Ignore</b> (DVD only)<br>"
                                     "This will leave the speed selection to the writer device. "
                                     "Use this if K3b is unable to set the writing speed."
                                     "<p>1x refers to 175 KB/s for CD, 1385 KB/s for DVD, and 4496 KB/s for Blu-ray.</p>"
                                     "<p><b>Caution:</b> Make sure your system is able to send the data "
                                     "fast enough to prevent buffer underruns.") );
    m_comboWritingApp->setWhatsThis( i18n("<p>K3b uses the command line tools cdrecord, growisofs, and cdrdao "
                                          "to actually write a CD or DVD."
                                          "<p>Normally K3b chooses the best "
                                          "suited application for every task automatically but in some cases it "
                                          "may be possible that one of the applications does not work as intended "
                                          "with a certain writer. In this case one may select the "
                                          "application manually.") );

    clearSpeedCombo();

    slotConfigChanged( KGlobal::config() );
    slotWriterChanged();
}


K3bWriterSelectionWidget::~K3bWriterSelectionWidget()
{
    delete d;
}


void K3bWriterSelectionWidget::setWantedMediumType( int type )
{
    m_comboMedium->setWantedMediumType( type );
}


void K3bWriterSelectionWidget::setWantedMediumState( int state )
{
    m_comboMedium->setWantedMediumState( state );
}


void K3bWriterSelectionWidget::setWantedMediumSize( const K3b::Msf& minSize )
{
#warning The wanted medium size may not be enough if we need to handle multisession!
    m_comboMedium->setWantedMediumSize( minSize );
}


int K3bWriterSelectionWidget::wantedMediumType() const
{
    return m_comboMedium->wantedMediumType();
}


int K3bWriterSelectionWidget::wantedMediumState() const
{
    return m_comboMedium->wantedMediumState();
}


K3b::Msf K3bWriterSelectionWidget::wantedMediumSize() const
{
    return m_comboMedium->wantedMediumSize();
}


void K3bWriterSelectionWidget::slotConfigChanged( KSharedConfig::Ptr c )
{
    KConfigGroup g( c, "General Options" );
    if( g.readEntry( "Manual writing app selection", false ) ) {
        m_comboWritingApp->show();
        m_writingAppLabel->show();
    }
    else {
        m_comboWritingApp->hide();
        m_writingAppLabel->hide();
    }
}


void K3bWriterSelectionWidget::slotRefreshWriterSpeeds()
{
    if( writerDevice() ) {
        QList<int> speeds = k3bappcore->mediaCache()->writingSpeeds( writerDevice() );

        int lastSpeed = writerSpeed();

        clearSpeedCombo();

        m_comboSpeed->insertItem( s_autoSpeedValue, i18n("Auto") );
        if( k3bappcore->mediaCache()->diskInfo( writerDevice() ).isDvdMedia() ) {
            m_comboSpeed->insertItem( s_ignoreSpeedValue, i18n("Ignore") );
            d->haveIgnoreSpeed = true;
        }
        else
            d->haveIgnoreSpeed = false;

        if( !d->forceAutoSpeed ) {
            if( speeds.isEmpty() || writerDevice() == m_comboMedium->overrideDevice() ) {
                //
                // In case of the override device we do not know which medium will actually be used
                // So this is the only case in which we need to use the device's max writing speed
                //
                // But we need to know if it will be a CD or DVD medium. Since the override device
                // is only used for CD/DVD copy anyway we simply reply on the inserted medium's type.
                //
                int i = 1;
                int x1Speed = K3bDevice::SPEED_FACTOR_CD;
                if ( k3bappcore->mediaCache()->diskInfo( writerDevice() ).isDvdMedia() ) {
                    x1Speed = K3bDevice::SPEED_FACTOR_DVD;
                }
                else if ( K3bDevice::isBdMedia( k3bappcore->mediaCache()->diskInfo( writerDevice() ).mediaType() ) ) {
                    x1Speed = K3bDevice::SPEED_FACTOR_BD;
                }
                int max = writerDevice()->maxWriteSpeed();
                while( i*x1Speed <= max ) {
                    insertSpeedItem( i*x1Speed );
                    // a little hack to handle the stupid 2.4x DVD speed
                    if( i == 2 && x1Speed == K3bDevice::SPEED_FACTOR_DVD )
                        insertSpeedItem( (int)(2.4*( double )K3bDevice::SPEED_FACTOR_DVD) );
                    i = ( i == 1 ? 2 : i+2 );
                }
            }
            else {
                for( QList<int>::iterator it = speeds.begin(); it != speeds.end(); ++it )
                    insertSpeedItem( *it );
            }
        }


        //
        // Although most devices return all speeds properly there are still some dumb ones around
        // that don't. Users of those will need the possibility to set the speed manually even if
        // a medium is inserted.
        //
        if ( !d->forceAutoSpeed ) {
            m_comboSpeed->insertItem( s_moreSpeedValue, i18n("More...") );
            d->haveManualSpeed = true;
        }
        else {
            d->haveManualSpeed = false;
        }


        // try to reload last set speed
        if( d->lastSetSpeed == -1 )
            setSpeed( lastSpeed );
        else
            setSpeed( d->lastSetSpeed );
    }

    m_comboSpeed->setEnabled( writerDevice() != 0 );
}


void K3bWriterSelectionWidget::clearSpeedCombo()
{
    m_comboSpeed->clear();
    d->haveManualSpeed = false;
    d->haveIgnoreSpeed = false;
}


void K3bWriterSelectionWidget::insertSpeedItem( int speed )
{
    int mediaType = k3bappcore->mediaCache()->diskInfo( writerDevice() ).mediaType();

    int insertIndex = -1;
    if ( m_comboSpeed->hasValue( s_moreSpeedValue ) ) {
        insertIndex = m_comboSpeed->count()-1;
    }

    //
    // polish the speed
    //
    if( K3bDevice::isDvdMedia( mediaType ) ) {
        //
        // AFAIK there is only one strange DVD burning speed like 2.4
        //
        int xs = ( int )( 1385.0*2.4 );
        if ( abs( speed - xs ) < 1385/2 )
            speed = xs;
        else
            speed = ( ( speed+692 )/1385 )*1385;
    }
    else if ( K3bDevice::isBdMedia( mediaType ) ) {
        speed = ( ( speed+2250 )/4496 )*4496;
    }
    else {
        speed = ( ( speed + ( K3bDevice::SPEED_FACTOR_CD/2 ) )/K3bDevice::SPEED_FACTOR_CD )*K3bDevice::SPEED_FACTOR_CD;
    }

    if( !m_comboSpeed->hasValue( speed ) ) {
        if( K3bDevice::isDvdMedia( mediaType ) ) {
            m_comboSpeed->insertItem( speed,
                                      ( speed%1385 > 0
                                        ? QString::number( (float)speed/1385.0, 'f', 1 )  // example: DVD+R(W): 2.4x
                                        : QString::number( speed/K3bDevice::SPEED_FACTOR_DVD ) )
                                      + "x",
                                      QString(),
                                      insertIndex );
        }
        else if ( K3bDevice::isBdMedia( mediaType ) ) {
            m_comboSpeed->insertItem( speed, QString("%1x").arg(speed/K3bDevice::SPEED_FACTOR_BD), QString(), insertIndex );
        }
        else {
            m_comboSpeed->insertItem( speed, QString("%1x").arg(speed/K3bDevice::SPEED_FACTOR_CD), QString(), insertIndex );
        }
    }
}


void K3bWriterSelectionWidget::slotWritingAppSelected( int app )
{
    emit writingAppChanged( K3b::WritingApp( app ) );
}


K3bDevice::Device* K3bWriterSelectionWidget::writerDevice() const
{
    return m_comboMedium->selectedDevice();
}


QList<K3bDevice::Device*> K3bWriterSelectionWidget::allDevices() const
{
    return m_comboMedium->allDevices();
}


void K3bWriterSelectionWidget::setWriterDevice( K3bDevice::Device* dev )
{
    m_comboMedium->setSelectedDevice( dev );
}


void K3bWriterSelectionWidget::setSpeed( int s )
{
    d->lastSetSpeed = -1;

    if( d->haveIgnoreSpeed && s < 0 )
        m_comboSpeed->setSelectedValue( s_ignoreSpeedValue ); // Ignore
    else if( m_comboSpeed->hasValue( s ) )
        m_comboSpeed->setSelectedValue( s );
    else {
        m_comboSpeed->setSelectedValue( s_autoSpeedValue ); // Auto
        d->lastSetSpeed = s; // remember last set speed
    }
}


void K3bWriterSelectionWidget::setWritingApp( K3b::WritingApp app )
{
    m_comboWritingApp->setSelectedValue( ( int )app );
}


int K3bWriterSelectionWidget::writerSpeed() const
{
    if( m_comboSpeed->selectedValue() == s_autoSpeedValue )
        return 0; // Auto
    else if( d->haveIgnoreSpeed && m_comboSpeed->selectedValue() == s_ignoreSpeedValue )
        return -1; // Ignore
    else
        return m_comboSpeed->selectedValue();
}


K3b::WritingApp K3bWriterSelectionWidget::writingApp() const
{
    KConfigGroup g( KGlobal::config(), "General Options" );
    if( g.readEntry( "Manual writing app selection", false ) ) {
        return selectedWritingApp();
    }
    else
        return K3b::WRITING_APP_DEFAULT;
}


K3b::WritingApp K3bWriterSelectionWidget::selectedWritingApp() const
{
    return K3b::WritingApp( m_comboWritingApp->selectedValue() );
}


void K3bWriterSelectionWidget::slotSpeedChanged( int s )
{
    // the last item is the manual speed selection item
    if( d->haveManualSpeed && s == s_moreSpeedValue ) {
        slotManualSpeed();
    }
    else {
        d->lastSetSpeed = s;

        if( K3bDevice::Device* dev = writerDevice() )
            dev->setCurrentWriteSpeed( writerSpeed() );
    }
}


void K3bWriterSelectionWidget::slotWriterChanged()
{
    slotRefreshWriterSpeeds();
    slotRefreshWritingApps();

    // save last selected writer
    if( K3bDevice::Device* dev = writerDevice() ) {
        KConfigGroup g( KGlobal::config(), "General Options" );
        g.writeEntry( "current_writer", dev->blockDeviceName() );
    }
}


void K3bWriterSelectionWidget::setSupportedWritingApps( K3b::WritingApps i )
{
    K3b::WritingApp oldApp = writingApp();

    d->supportedWritingApps = i;

    slotRefreshWritingApps();

    setWritingApp( oldApp );
}


void K3bWriterSelectionWidget::slotRefreshWritingApps()
{
    K3b::WritingApps i = 0;

    // select the ones that make sense
    if( k3bappcore->mediaCache()->diskInfo( writerDevice() ).isDvdMedia() )
        i = K3b::WRITING_APP_GROWISOFS|K3b::WRITING_APP_DVD_RW_FORMAT|K3b::WRITING_APP_CDRECORD;
    else if ( K3bDevice::isBdMedia( k3bappcore->mediaCache()->diskInfo( writerDevice() ).mediaType() ) )
        i = K3b::WRITING_APP_GROWISOFS|K3b::WRITING_APP_CDRECORD;
    else
        i = K3b::WRITING_APP_CDRDAO|K3b::WRITING_APP_CDRECORD;

    // now strip it down to the ones we support
    i &= d->supportedWritingApps;

    m_comboWritingApp->clear();
    m_comboWritingApp->insertItem( K3b::WRITING_APP_DEFAULT, i18n("Auto") );

    if( i & K3b::WRITING_APP_CDRDAO )
        m_comboWritingApp->insertItem( K3b::WRITING_APP_CDRDAO, "cdrdao" );
    if( i & K3b::WRITING_APP_CDRECORD )
        m_comboWritingApp->insertItem( K3b::WRITING_APP_CDRECORD, "cdrecord" );
    if( i & K3b::WRITING_APP_GROWISOFS )
        m_comboWritingApp->insertItem( K3b::WRITING_APP_GROWISOFS, "growisofs" );
    if( i & K3b::WRITING_APP_DVD_RW_FORMAT )
        m_comboWritingApp->insertItem( K3b::WRITING_APP_DVD_RW_FORMAT, "dvd+rw-format" );

    m_comboWritingApp->setEnabled( writerDevice() != 0 );
}


void K3bWriterSelectionWidget::loadConfig( const KConfigGroup& c )
{
    setWriterDevice( k3bcore->deviceManager()->findDevice( c.readEntry( "writer_device" ) ) );
    setSpeed( c.readEntry( "writing_speed",  0 ) );
    setWritingApp( K3b::writingAppFromString( c.readEntry( "writing_app" ) ) );
}


void K3bWriterSelectionWidget::saveConfig( KConfigGroup c )
{
    c.writeEntry( "writing_speed", writerSpeed() );
    c.writeEntry( "writer_device", writerDevice() ? writerDevice()->blockDeviceName() : QString() );
    c.writeEntry( "writing_app", m_comboWritingApp->currentText() );
}

void K3bWriterSelectionWidget::loadDefaults()
{
    // ignore the writer
    m_comboSpeed->setSelectedValue( s_autoSpeedValue ); // Auto
    setWritingApp( K3b::WRITING_APP_DEFAULT );
}


void K3bWriterSelectionWidget::setForceAutoSpeed( bool b )
{
    d->forceAutoSpeed = b;
    slotRefreshWriterSpeeds();
}


void K3bWriterSelectionWidget::setOverrideDevice( K3bDevice::Device* dev, const QString& overrideString, const QString& tooltip )
{
    m_comboMedium->setOverrideDevice( dev, overrideString, tooltip );
}


void K3bWriterSelectionWidget::slotNewBurnMedium( K3bDevice::Device* dev )
{
    //
    // Try to select a medium that is better suited than the current one
    //
    if( dev && dev != writerDevice() ) {
        K3bMedium medium = k3bappcore->mediaCache()->medium( dev );

        //
        // Always prefer newly inserted media over the override device
        //
        if( writerDevice() == m_comboMedium->overrideDevice() ) {
            setWriterDevice( dev );
        }

        //
        // Prefer an empty medium over one that has to be erased
        //
        else if( wantedMediumState() & K3bDevice::STATE_EMPTY &&
                 !k3bappcore->mediaCache()->diskInfo( writerDevice() ).empty() &&
                 medium.diskInfo().empty() ) {
            setWriterDevice( dev );
        }
    }
}


void K3bWriterSelectionWidget::slotManualSpeed()
{
    //
    // In case we think we have all the available speeds (i.e. if the device reported a non-empty list)
    // we just treat it as a manual selection. Otherwise we admit that we cannot do better
    //
    bool haveSpeeds = ( writerDevice() && !k3bappcore->mediaCache()->writingSpeeds( writerDevice() ).isEmpty() );
    QString s;
    if ( haveSpeeds ) {
        s = i18n( "Please enter the speed that K3b should use for burning (Example: 16x)." );
    }
    else {
        s = i18n("<p>K3b is not able to perfectly determine the maximum "
                 "writing speed of an optical writer. Writing speed is always "
                 "reported subject to the inserted medium."
                 "<p>Please enter the writing speed here and K3b will remember it "
                 "for future sessions (Example: 16x).");
    }

    //
    // We need to know the type of medium. Since the override device
    // is only used for copy anyway we simply reply on the inserted medium's type.
    //
    int speedFactor = K3bDevice::SPEED_FACTOR_CD;
    if( k3bappcore->mediaCache()->diskInfo( writerDevice() ).isDvdMedia() ) {
        speedFactor = K3bDevice::SPEED_FACTOR_DVD;
    }
    else if ( k3bappcore->mediaCache()->diskInfo( writerDevice() ).mediaType() & K3bDevice::MEDIA_BD_ALL ) {
        speedFactor = K3bDevice::SPEED_FACTOR_BD;
    }

    bool ok = true;
    int newSpeed = KInputDialog::getInteger( i18n("Set writing speed manually"),
                                             s,
                                             writerDevice()->maxWriteSpeed()/speedFactor,
                                             1,
                                             10000,
                                             1,
                                             10,
                                             &ok,
                                             this ) * speedFactor;
    if( ok ) {
        writerDevice()->setMaxWriteSpeed( qMax( newSpeed, writerDevice()->maxWriteSpeed() ) );
        if ( haveSpeeds ) {
            insertSpeedItem( newSpeed );
        }
        else {
            slotRefreshWriterSpeeds();
        }
        setSpeed( newSpeed );
    }
    else {
        if( d->lastSetSpeed == -1 )
            m_comboSpeed->setSelectedValue( s_autoSpeedValue ); // Auto
        else
            setSpeed( d->lastSetSpeed );
    }
}


void K3bWriterSelectionWidget::setIgnoreDevice( K3bDevice::Device* dev )
{
    m_comboMedium->setIgnoreDevice( dev );
}

#include "k3bwriterselectionwidget.moc"
