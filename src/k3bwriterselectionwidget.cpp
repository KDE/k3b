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


#include "k3bwriterselectionwidget.h"
#include "k3bapplication.h"
#include "k3bmediacache.h"

#include "k3bmediaselectioncombobox.h"
#include "k3bdevice.h"
#include "k3bdevicemanager.h"
#include "k3bglobals.h"
#include "k3bcore.h"
#include "k3bintmapcombobox.h"

#include <KConfigCore/KConfig>
#include <KConfigCore/KConfigGroup>
#include <KConfigCore/KSharedConfig>
#include <KLocalizedString>
#include <KWidgetsAddons/KMessageBox>

#include <QCursor>
#include <QApplication>
#include <QGridLayout>
#include <QGroupBox>
#include <QInputDialog>
#include <QLabel>
#include <QLayout>
#include <QToolButton>
#include <QToolTip>

#include <cstdlib>


namespace {
    int s_autoSpeedValue = 0;
    int s_ignoreSpeedValue = -1;
    int s_moreSpeedValue = -2;
}


class K3b::WriterSelectionWidget::MediaSelectionComboBox : public K3b::MediaSelectionComboBox
{
public:
    MediaSelectionComboBox( QWidget* parent )
        : K3b::MediaSelectionComboBox( parent ),
          m_overrideDevice( 0 ) {
    }

    void setOverrideDevice( K3b::Device::Device* dev, const QString& s, const QString& t ) {
        m_overrideDevice = dev;
        m_overrideString = s;
        m_overrideToolTip = t;
        updateMedia();
    }

    K3b::Device::Device* overrideDevice() const {
        return m_overrideDevice;
    }

protected:
    bool showMedium( const K3b::Medium& m ) const {
        return ( m.device() == m_overrideDevice ||
                 K3b::MediaSelectionComboBox::showMedium( m ) );
    }

    QString mediumString( const K3b::Medium& m ) const {
        if( m.device() == m_overrideDevice )
            return m_overrideString;
        else
            return K3b::MediaSelectionComboBox::mediumString( m );
    }

    QString mediumToolTip( const K3b::Medium& m ) const {
        if( m.device() == m_overrideDevice )
            return m_overrideToolTip;
        else {
            QString s = K3b::MediaSelectionComboBox::mediumToolTip( m );
            if( !m.diskInfo().empty() && !(wantedMediumState() & m.diskInfo().diskState()) )
                s.append( "<p><i>" + i18n("Medium will be overwritten.") + "</i>" );
            return s;
        }
    }

private:
    K3b::Device::Device* m_overrideDevice;
    QString m_overrideString;
    QString m_overrideToolTip;
};


class K3b::WriterSelectionWidget::Private
{
public:
    bool forceAutoSpeed;
    bool haveIgnoreSpeed;
    bool haveManualSpeed;

    K3b::WritingApps supportedWritingApps;

    int lastSetSpeed;
};


K3b::WriterSelectionWidget::WriterSelectionWidget( QWidget *parent )
    : QWidget( parent )
{
    d = new Private;
    d->forceAutoSpeed = false;
    d->supportedWritingApps = K3b::WritingAppCdrecord|K3b::WritingAppCdrdao|K3b::WritingAppGrowisofs;
    d->lastSetSpeed = -1;

    QGroupBox* groupWriter = new QGroupBox( this );
    groupWriter->setTitle( i18n( "Burn Medium" ) );

    QGridLayout* groupWriterLayout = new QGridLayout( groupWriter );
    groupWriterLayout->setAlignment( Qt::AlignTop );

    QLabel* labelSpeed = new QLabel( groupWriter );
    labelSpeed->setText( i18n( "Speed:" ) );

    m_comboSpeed = new K3b::IntMapComboBox( groupWriter );

    m_comboMedium = new MediaSelectionComboBox( groupWriter );

    m_writingAppLabel = new QLabel( i18n("Writing app:"), groupWriter );
    m_comboWritingApp = new K3b::IntMapComboBox( groupWriter );

    groupWriterLayout->addWidget( m_comboMedium, 0, 0 );
    groupWriterLayout->addWidget( labelSpeed, 0, 1 );
    groupWriterLayout->addWidget( m_comboSpeed, 0, 2 );
    groupWriterLayout->addWidget( m_writingAppLabel, 0, 3 );
    groupWriterLayout->addWidget( m_comboWritingApp, 0, 4 );
    groupWriterLayout->setColumnStretch( 0, 1 );


    QGridLayout* mainLayout = new QGridLayout( this );
    mainLayout->setAlignment( Qt::AlignTop );
    mainLayout->setContentsMargins( 0, 0, 0, 0 );

    mainLayout->addWidget( groupWriter, 0, 0 );

    // tab order
    setTabOrder( m_comboMedium, m_comboSpeed );
    setTabOrder( m_comboSpeed, m_comboWritingApp );

    connect( m_comboMedium, SIGNAL(selectionChanged(K3b::Device::Device*)), this, SIGNAL(writerChanged()) );
    connect( m_comboMedium, SIGNAL(selectionChanged(K3b::Device::Device*)),
             this, SIGNAL(writerChanged(K3b::Device::Device*)) );
    connect( m_comboMedium, SIGNAL(newMedia()), this, SIGNAL(newMedia()) );
    connect( m_comboMedium, SIGNAL(newMedium(K3b::Device::Device*)), this, SIGNAL(newMedium(K3b::Device::Device*)) );
    connect( m_comboMedium, SIGNAL(newMedium(K3b::Device::Device*)), this, SLOT(slotNewBurnMedium(K3b::Device::Device*)) );
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

    slotConfigChanged( KSharedConfig::openConfig() );
    slotWriterChanged();
}


K3b::WriterSelectionWidget::~WriterSelectionWidget()
{
    delete d;
}


void K3b::WriterSelectionWidget::setWantedMediumType( Device::MediaTypes type )
{
    m_comboMedium->setWantedMediumType( type );
}


void K3b::WriterSelectionWidget::setWantedMediumState( Device::MediaStates state )
{
    m_comboMedium->setWantedMediumState( state );
}


void K3b::WriterSelectionWidget::setWantedMediumSize( const K3b::Msf& minSize )
{
#ifdef __GNUC__
#warning The wanted medium size may not be enough if we need to handle multisession!
#endif
    m_comboMedium->setWantedMediumSize( minSize );
}


K3b::Device::MediaTypes K3b::WriterSelectionWidget::wantedMediumType() const
{
    return m_comboMedium->wantedMediumType();
}


K3b::Device::MediaStates K3b::WriterSelectionWidget::wantedMediumState() const
{
    return m_comboMedium->wantedMediumState();
}


K3b::Msf K3b::WriterSelectionWidget::wantedMediumSize() const
{
    return m_comboMedium->wantedMediumSize();
}


void K3b::WriterSelectionWidget::slotConfigChanged( KSharedConfig::Ptr c )
{
    KConfigGroup g( c, "General Options" );
    if( g.readEntry( "Show advanced GUI", false ) ) {
        m_comboWritingApp->show();
        m_writingAppLabel->show();
    }
    else {
        m_comboWritingApp->hide();
        m_writingAppLabel->hide();
    }
}


void K3b::WriterSelectionWidget::slotRefreshWriterSpeeds()
{
    if( writerDevice() ) {
        QList<int> speeds = k3bappcore->mediaCache()->writingSpeeds( writerDevice() );

        int lastSpeed = writerSpeed();

        clearSpeedCombo();

        m_comboSpeed->insertItem( s_autoSpeedValue, i18n("Auto") );
        if( Device::isDvdMedia( k3bappcore->mediaCache()->diskInfo( writerDevice() ).mediaType() ) ) {
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
                int x1Speed = K3b::Device::SPEED_FACTOR_CD;
                if( Device::isDvdMedia( k3bappcore->mediaCache()->diskInfo( writerDevice() ).mediaType() ) ) {
                    x1Speed = K3b::Device::SPEED_FACTOR_DVD;
                }
                else if( Device::isBdMedia( k3bappcore->mediaCache()->diskInfo( writerDevice() ).mediaType() ) ) {
                    x1Speed = K3b::Device::SPEED_FACTOR_BD;
                }

                const int max = writerDevice()->maxWriteSpeed();
                for( int i = 1; i*x1Speed <= max; i = ( i == 1 ? 2 : i+2 ) ) {
                    insertSpeedItem( i*x1Speed );
                    // a little hack to handle the stupid 2.4x DVD speed
                    if( i == 2 && x1Speed == K3b::Device::SPEED_FACTOR_DVD )
                        insertSpeedItem( (int)(2.4*( double )K3b::Device::SPEED_FACTOR_DVD) );
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


void K3b::WriterSelectionWidget::clearSpeedCombo()
{
    m_comboSpeed->clear();
    d->haveManualSpeed = false;
    d->haveIgnoreSpeed = false;
}


void K3b::WriterSelectionWidget::insertSpeedItem( int speed )
{
    Device::MediaType mediaType = k3bappcore->mediaCache()->diskInfo( writerDevice() ).mediaType();

    int insertIndex = -1;
    if ( m_comboSpeed->hasValue( s_moreSpeedValue ) ) {
        insertIndex = m_comboSpeed->count()-1;
    }

    //
    // polish the speed
    //
    if( K3b::Device::isDvdMedia( mediaType ) ) {
        //
        // AFAIK there is only one strange DVD burning speed like 2.4
        //
        int xs = int( double( Device::SPEED_FACTOR_DVD ) * 2.4 );
        if ( abs( speed - xs ) < Device::SPEED_FACTOR_DVD/2 )
            speed = xs;
        else
            speed = ( ( speed+692 )/Device::SPEED_FACTOR_DVD )*Device::SPEED_FACTOR_DVD;
    }
    else if ( K3b::Device::isBdMedia( mediaType ) ) {
        speed = ( ( speed+2250 )/Device::SPEED_FACTOR_BD )*Device::SPEED_FACTOR_BD;
    }
    else {
        speed = ( ( speed + ( K3b::Device::SPEED_FACTOR_CD/2 ) )/K3b::Device::SPEED_FACTOR_CD )*K3b::Device::SPEED_FACTOR_CD;
    }

    if( !m_comboSpeed->hasValue( speed ) ) {
        if( K3b::Device::isDvdMedia( mediaType ) ) {
            m_comboSpeed->insertItem( speed,
                                      ( speed%Device::SPEED_FACTOR_DVD > 0
                                        ? QString::number( float(speed)/float(Device::SPEED_FACTOR_DVD), 'f', 1 )  // example: DVD+R(W): 2.4x
                                        : QString::number( speed/K3b::Device::SPEED_FACTOR_DVD ) )
                                      + 'x',
                                      QString(),
                                      insertIndex );
        }
        else if ( K3b::Device::isBdMedia( mediaType ) ) {
            m_comboSpeed->insertItem( speed, QString("%1x").arg(speed/K3b::Device::SPEED_FACTOR_BD), QString(), insertIndex );
        }
        else {
            m_comboSpeed->insertItem( speed, QString("%1x").arg(speed/K3b::Device::SPEED_FACTOR_CD), QString(), insertIndex );
        }
    }
}


void K3b::WriterSelectionWidget::slotWritingAppSelected( int app )
{
    emit writingAppChanged( K3b::WritingApp( app ) );
}


K3b::Device::Device* K3b::WriterSelectionWidget::writerDevice() const
{
    return m_comboMedium->selectedDevice();
}


QList<K3b::Device::Device*> K3b::WriterSelectionWidget::allDevices() const
{
    return m_comboMedium->allDevices();
}


void K3b::WriterSelectionWidget::setWriterDevice( K3b::Device::Device* dev )
{
    m_comboMedium->setSelectedDevice( dev );
}


void K3b::WriterSelectionWidget::setSpeed( int s )
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


void K3b::WriterSelectionWidget::setWritingApp( K3b::WritingApp app )
{
    m_comboWritingApp->setSelectedValue( ( int )app );
}


int K3b::WriterSelectionWidget::writerSpeed() const
{
    if( m_comboSpeed->selectedValue() == s_autoSpeedValue )
        return 0; // Auto
    else if( d->haveIgnoreSpeed && m_comboSpeed->selectedValue() == s_ignoreSpeedValue )
        return -1; // Ignore
    else
        return m_comboSpeed->selectedValue();
}


K3b::WritingApp K3b::WriterSelectionWidget::writingApp() const
{
    KConfigGroup g( KSharedConfig::openConfig(), "General Options" );
    if( g.readEntry( "Show advanced GUI", false ) ) {
        return selectedWritingApp();
    }
    else
        return K3b::WritingAppAuto;
}


K3b::WritingApp K3b::WriterSelectionWidget::selectedWritingApp() const
{
    return K3b::WritingApp( m_comboWritingApp->selectedValue() );
}


void K3b::WriterSelectionWidget::slotSpeedChanged( int s )
{
    // the last item is the manual speed selection item
    if( d->haveManualSpeed && s == s_moreSpeedValue ) {
        slotManualSpeed();
    }
    else {
        d->lastSetSpeed = s;

        if( K3b::Device::Device* dev = writerDevice() )
            dev->setCurrentWriteSpeed( writerSpeed() );
    }
}


void K3b::WriterSelectionWidget::slotWriterChanged()
{
    slotRefreshWriterSpeeds();
    slotRefreshWritingApps();

    // save last selected writer
    if( K3b::Device::Device* dev = writerDevice() ) {
        KConfigGroup g( KSharedConfig::openConfig(), "General Options" );
        g.writeEntry( "current_writer", dev->blockDeviceName() );
    }
}


void K3b::WriterSelectionWidget::setSupportedWritingApps( K3b::WritingApps i )
{
    K3b::WritingApp oldApp = writingApp();

    d->supportedWritingApps = i;

    slotRefreshWritingApps();

    setWritingApp( oldApp );
}


void K3b::WriterSelectionWidget::slotRefreshWritingApps()
{
    K3b::WritingApps i = 0;

    int lastSelected = m_comboWritingApp->selectedValue();

    // select the ones that make sense
    if( Device::isDvdMedia( k3bappcore->mediaCache()->diskInfo( writerDevice() ).mediaType() ) )
        i = K3b::WritingAppGrowisofs|K3b::WritingAppDvdRwFormat|K3b::WritingAppCdrecord;
    else if ( K3b::Device::isBdMedia( k3bappcore->mediaCache()->diskInfo( writerDevice() ).mediaType() ) )
        i = K3b::WritingAppGrowisofs|K3b::WritingAppCdrecord;
    else
        i = K3b::WritingAppCdrdao|K3b::WritingAppCdrecord;

    // now strip it down to the ones we support
    i &= d->supportedWritingApps;

    m_comboWritingApp->clear();
    m_comboWritingApp->insertItem( K3b::WritingAppAuto, i18n("Auto") );

    if( i & K3b::WritingAppCdrdao )
        m_comboWritingApp->insertItem( K3b::WritingAppCdrdao, "cdrdao" );
    if( i & K3b::WritingAppCdrecord )
        m_comboWritingApp->insertItem( K3b::WritingAppCdrecord, "cdrecord" );
    if( i & K3b::WritingAppGrowisofs )
        m_comboWritingApp->insertItem( K3b::WritingAppGrowisofs, "growisofs" );
    if( i & K3b::WritingAppDvdRwFormat )
        m_comboWritingApp->insertItem( K3b::WritingAppDvdRwFormat, "dvd+rw-format" );
    if (i & K3b::WritingAppCdrskin)
        m_comboWritingApp->insertItem(K3b::WritingAppCdrskin, "cdrskin");

    m_comboWritingApp->setSelectedValue( lastSelected );

    m_comboWritingApp->setEnabled( writerDevice() != 0 );
}


void K3b::WriterSelectionWidget::loadConfig( const KConfigGroup& c )
{
    setWriterDevice( k3bcore->deviceManager()->findDevice( c.readEntry( "writer_device" ) ) );
    setSpeed( c.readEntry( "writing_speed",  s_autoSpeedValue ) );
    setWritingApp( K3b::writingAppFromString( c.readEntry( "writing_app" ) ) );
}


void K3b::WriterSelectionWidget::saveConfig( KConfigGroup c )
{
    c.writeEntry( "writing_speed", writerSpeed() );
    c.writeEntry( "writer_device", writerDevice() ? writerDevice()->blockDeviceName() : QString() );
    c.writeEntry( "writing_app", m_comboWritingApp->currentText() );
}

void K3b::WriterSelectionWidget::setForceAutoSpeed( bool b )
{
    d->forceAutoSpeed = b;
    slotRefreshWriterSpeeds();
}


void K3b::WriterSelectionWidget::setOverrideDevice( K3b::Device::Device* dev, const QString& overrideString, const QString& tooltip )
{
    m_comboMedium->setOverrideDevice( dev, overrideString, tooltip );
}


void K3b::WriterSelectionWidget::slotNewBurnMedium( K3b::Device::Device* dev )
{
    //
    // Try to select a medium that is better suited than the current one
    //
    if( dev && dev != writerDevice() ) {
        K3b::Medium medium = k3bappcore->mediaCache()->medium( dev );

        //
        // Always prefer newly inserted media over the override device
        //
        if( writerDevice() == m_comboMedium->overrideDevice() ) {
            setWriterDevice( dev );
        }

        //
        // Prefer an empty medium over one that has to be erased
        //
        else if( wantedMediumState() & K3b::Device::STATE_EMPTY &&
                 !k3bappcore->mediaCache()->diskInfo( writerDevice() ).empty() &&
                 medium.diskInfo().empty() ) {
            setWriterDevice( dev );
        }
    }
}


void K3b::WriterSelectionWidget::slotManualSpeed()
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
    int speedFactor = K3b::Device::SPEED_FACTOR_CD;
    if( Device::isDvdMedia( k3bappcore->mediaCache()->diskInfo( writerDevice() ).mediaType() ) ) {
        speedFactor = K3b::Device::SPEED_FACTOR_DVD;
    }
    else if( Device::isBdMedia( k3bappcore->mediaCache()->diskInfo( writerDevice() ).mediaType() ) ) {
        speedFactor = K3b::Device::SPEED_FACTOR_BD;
    }

    bool ok = true;
    int newSpeed = QInputDialog::getInt( this,
                                         i18n("Set writing speed manually"),
                                         s,
                                         writerDevice()->maxWriteSpeed()/speedFactor,
                                         1,
                                         10000,
                                         1,
                                         &ok ) * speedFactor;
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


void K3b::WriterSelectionWidget::setIgnoreDevice( K3b::Device::Device* dev )
{
    m_comboMedium->setIgnoreDevice( dev );
}


