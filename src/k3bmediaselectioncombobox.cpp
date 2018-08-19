/*
 *
 * Copyright (C) 2005-2009 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bmediaselectioncombobox.h"
#include "k3bapplication.h"
#include "k3bmediacache.h"
#include "k3bmediumdelegate.h"

#include "k3bglobalsettings.h"
#include "k3bdevice.h"
#include "k3bdevicemanager.h"
#include "k3bdeviceglobals.h"
#include "k3bdiskinfo.h"
#include "k3bglobals.h"
#include "k3btoc.h"
#include "k3bcdtext.h"

#include <KLocalizedString>

#include <QDebug>
#include <QMap>
#include <QVector>
#include <QList>
#include <QFont>

Q_DECLARE_METATYPE(K3b::Medium)

class K3b::MediaSelectionComboBox::Private
{
public:
    Private()
        : ignoreDevice( 0 ) {
    }

    QMap<K3b::Device::Device*, int> deviceIndexMap;
    QVector<K3b::Device::Device*> devices;

    K3b::Device::Device* ignoreDevice;

    // medium strings for every entry
    QMap<QString, int> mediaStringMap;

    Device::MediaTypes wantedMediumType;
    Device::MediaStates wantedMediumState;
    Medium::MediumContents wantedMediumContent;
    K3b::Msf wantedMediumSize;

    QList<Medium> excludedMediums;

    QFont origFont;
};


K3b::MediaSelectionComboBox::MediaSelectionComboBox( QWidget* parent )
    : KComboBox( false, parent )
{
    d = new Private();
    d->origFont = font();

    // set defaults
    d->wantedMediumType = K3b::Device::MEDIA_WRITABLE_CD;
    d->wantedMediumState = K3b::Device::STATE_EMPTY;
    d->wantedMediumContent = K3b::Medium::ContentAll;

    setItemDelegate( new MediumDelegate( this ) );

    connect( this, SIGNAL(activated(int)),
             this, SLOT(slotActivated(int)) );
    connect( k3bcore->deviceManager(), SIGNAL(changed(K3b::Device::DeviceManager*)),
             this, SLOT(slotDeviceManagerChanged(K3b::Device::DeviceManager*)) );
    connect( k3bappcore->mediaCache(), SIGNAL(mediumChanged(K3b::Device::Device*)),
             this, SLOT(slotMediumChanged(K3b::Device::Device*)) );
    connect( this, SIGNAL(selectionChanged(K3b::Device::Device*)),
             this, SLOT(slotUpdateToolTip(K3b::Device::Device*)) );

    updateMedia();
}


K3b::MediaSelectionComboBox::~MediaSelectionComboBox()
{
    delete d;
}


void K3b::MediaSelectionComboBox::setIgnoreDevice( K3b::Device::Device* dev )
{
    d->ignoreDevice = dev;
    updateMedia();
}


K3b::Device::Device* K3b::MediaSelectionComboBox::selectedDevice() const
{
    if( d->devices.count() > currentIndex() &&
        currentIndex() >= 0 )
        return d->devices[currentIndex()];
    else
        return 0;
}


QList<K3b::Device::Device*> K3b::MediaSelectionComboBox::allDevices() const
{
    QList<K3b::Device::Device*> l;
    for( int i = 0; i < d->devices.count(); ++i )
        l.append( d->devices[i] );
    return l;
}


void K3b::MediaSelectionComboBox::setSelectedDevice( K3b::Device::Device* dev )
{
    if( dev && d->deviceIndexMap.contains( dev ) ) {
        setCurrentIndex( d->deviceIndexMap[dev] );
        emit selectionChanged( dev );
    }
}


void K3b::MediaSelectionComboBox::setWantedMediumType( K3b::Device::MediaTypes type )
{
    if( type != 0 && type != d->wantedMediumType) {
        d->wantedMediumType = type;
        updateMedia();
    }
}


void K3b::MediaSelectionComboBox::setWantedMediumState( K3b::Device::MediaStates state )
{
    if( state != 0 && state != d->wantedMediumState ) {
        d->wantedMediumState = state;
        updateMedia();
    }
}


void K3b::MediaSelectionComboBox::setWantedMediumContent( K3b::Medium::MediumContents content )
{
    if( content != d->wantedMediumContent ) {
        d->wantedMediumContent = content;
        updateMedia();
    }
}


void K3b::MediaSelectionComboBox::setWantedMediumSize( const K3b::Msf& minSize )
{
    if ( d->wantedMediumSize != minSize ) {
        d->wantedMediumSize = minSize;
        updateMedia();
    }
}


K3b::Device::MediaTypes K3b::MediaSelectionComboBox::wantedMediumType() const
{
    return d->wantedMediumType;
}


K3b::Device::MediaStates K3b::MediaSelectionComboBox::wantedMediumState() const
{
    return d->wantedMediumState;
}


K3b::Medium::MediumContents K3b::MediaSelectionComboBox::wantedMediumContent() const
{
    return d->wantedMediumContent;
}


K3b::Msf K3b::MediaSelectionComboBox::wantedMediumSize() const
{
    return d->wantedMediumSize;
}

void K3b::MediaSelectionComboBox::slotActivated( int i )
{
    if( d->devices.count() > 0 )
        emit selectionChanged( d->devices[i] );
}


void K3b::MediaSelectionComboBox::slotMediumChanged( K3b::Device::Device* dev )
{
    updateMedium( dev );
}


void K3b::MediaSelectionComboBox::clear()
{
    d->deviceIndexMap.clear();
    d->mediaStringMap.clear();
    d->devices.clear();
    KComboBox::clear();
    setFont( d->origFont );
}


void K3b::MediaSelectionComboBox::showNoMediumMessage()
{
    // make it italic
    QFont f( font() );
    f.setItalic( true );
    setFont( f );
    if ( d->excludedMediums.isEmpty() ) {
        addItem( noMediumMessage() );
    }
    else {
        addItems( noMediumMessages() );
    }
    setItemData( 0, f, Qt::FontRole );
}


void K3b::MediaSelectionComboBox::updateMedia()
{
    // remember set of devices
    QVector<K3b::Device::Device*> oldDevices = d->devices;

    // remember last selected medium
    K3b::Device::Device* selected = selectedDevice();

    clear();
    d->excludedMediums.clear();

    //
    // We need to only check a selection of the available devices based on the
    // wanted media type.
    //

    // no ROM media -> we most likely want only CD/DVD writers
    bool rwOnly = !( wantedMediumType() & (K3b::Device::MEDIA_CD_ROM|K3b::Device::MEDIA_DVD_ROM) );
    bool dvdOnly = !( wantedMediumType() & (K3b::Device::MEDIA_CD_ROM|K3b::Device::MEDIA_WRITABLE_CD) );

    QList<K3b::Device::Device *> devices(k3bcore->deviceManager()->allDevices());
    if( dvdOnly ) {
        if( rwOnly )
            devices = k3bcore->deviceManager()->dvdWriter();
        else
            devices = k3bcore->deviceManager()->dvdReader();
    }
    else if( rwOnly )
        devices = k3bcore->deviceManager()->cdWriter();
    else
        devices = k3bcore->deviceManager()->cdReader();

    for( QList<K3b::Device::Device *>::const_iterator it = devices.constBegin();
         it != devices.constEnd(); ++it ) {
        if ( d->ignoreDevice == *it ) {
            continue;
        }

        K3b::Medium medium = k3bappcore->mediaCache()->medium( *it );

        if( showMedium( medium ) ) {
            addMedium( *it );
        }
        else if( !( medium.diskInfo().diskState() & (Device::STATE_NO_MEDIA|Device::STATE_UNKNOWN) ) ) {
            d->excludedMediums.append( medium );
        }
    }

    //
    // Now in case no usable medium was found show the user a little message
    //
    if( d->devices.isEmpty() ) {
        showNoMediumMessage();
        if( selected != 0 ) {
            // inform that we have no medium at all
            emit selectionChanged( 0 );
        }
    }
    else if( selected && d->deviceIndexMap.contains( selected ) ) {
        setCurrentIndex( d->deviceIndexMap[selected] );
    }
    else {
        emit selectionChanged( selectedDevice() );
    }

    // did the selection of devices change
    if( !( d->devices == oldDevices ) ) {
        emit newMedia();
        for( int i = 0; i < d->devices.count(); ++i ) {
            int j = 0;
            for( j = 0; j < oldDevices.count(); ++j ) {
                if( oldDevices[j] == d->devices[i] )
                    break;
            }
            if( j == oldDevices.count() ) {
                // prefer a newly inserted medium over the previously selected
                setSelectedDevice( d->devices[i] );
                emit newMedium( d->devices[i] );
            }
        }
    }
}


void K3b::MediaSelectionComboBox::updateMedium( K3b::Device::Device* )
{
    // for now we simply rebuild the whole list
    updateMedia();
}


void K3b::MediaSelectionComboBox::addMedium( K3b::Device::Device* dev )
{
    //
    // In case we only want an empty medium (this might happen in case the
    // the medium is rewritable) we do not care about the contents but tell
    // the user that the medium is rewritable.
    // Otherwise we show the contents type since this might also be used
    // for source selection.
    //
    QString s = mediumString( k3bappcore->mediaCache()->medium( dev ) );

    //
    // Now let's see if this string is already contained in the list
    // and if so add the device name to both
    //
    if( d->mediaStringMap.contains( s ) ) {
        //
        // insert the modified string
        //
        addItem( s + QString(" (%1 - %2)").arg(dev->vendor()).arg(dev->description()) );
        setItemData( count()-1, QVariant::fromValue(k3bappcore->mediaCache()->medium( dev )), MediumDelegate::MediumRole );

        //
        // change the already existing string if we did not already do so
        // (which happens if more than 2 entries have the same medium string)
        //
        int prevIndex = d->mediaStringMap[s];
        if( prevIndex >= 0 )
            setItemText( prevIndex,itemText(prevIndex) + QString(" (%1 - %2)").arg(d->devices[prevIndex]->vendor()).arg(d->devices[prevIndex]->description() ));

        //
        // mark the string as already changed
        //
        d->mediaStringMap[s] = -1;
    }
    else {
        //
        // insert the plain medium string
        //
        addItem( s );
        setItemData( count()-1, QVariant::fromValue(k3bappcore->mediaCache()->medium( dev )), MediumDelegate::MediumRole );
        d->mediaStringMap[s] = count()-1;
    }

    //
    // update the helper structures
    //
    d->deviceIndexMap[dev] = count()-1;
    d->devices.append( dev );

    setItemData( count()-1, mediumToolTip( k3bappcore->mediaCache()->medium( dev ) ), Qt::ToolTipRole );
}


void K3b::MediaSelectionComboBox::slotDeviceManagerChanged( K3b::Device::DeviceManager* )
{
    updateMedia();
}


bool K3b::MediaSelectionComboBox::showMedium( const K3b::Medium& m ) const
{
    //
    // also use if wantedMediumState empty and medium rewritable
    // because we can always format/erase/overwrite it
    //
    // DVD+RW and DVD-RW restr. ovwr. are never reported as appendable
    //
    return( m.diskInfo().mediaType() & d->wantedMediumType

            &&

            ( m.content() & d->wantedMediumContent )

            &&

            ( m.diskInfo().diskState() & d->wantedMediumState
              ||
              ( d->wantedMediumState & K3b::Device::STATE_EMPTY &&
                m.diskInfo().rewritable() )
              ||
              ( d->wantedMediumState & K3b::Device::STATE_INCOMPLETE &&
                !m.diskInfo().empty() &&
                m.diskInfo().mediaType() & (K3b::Device::MEDIA_DVD_PLUS_RW|K3b::Device::MEDIA_DVD_RW_OVWR) ) )

            &&

            ( d->wantedMediumSize == 0 ||                                        // no specific size requested
              d->wantedMediumSize <= m.diskInfo().capacity() ||                  // size fits
              ( !d->wantedMediumState.testFlag( K3b::Device::STATE_COMPLETE ) && // size does not fit but an empty/appendable medium in overburn mode is requested
                IsOverburnAllowed( d->wantedMediumSize, m.diskInfo().capacity() ) ) )
        );
}


QString K3b::MediaSelectionComboBox::mediumString( const K3b::Medium& medium ) const
{
    return medium.shortString();
}


QString K3b::MediaSelectionComboBox::mediumToolTip( const K3b::Medium& m ) const
{
    return m.longString( Medium::WithContents|Medium::WithDevice );
}


QString K3b::MediaSelectionComboBox::noMediumMessage() const
{
    if( d->wantedMediumContent && d->wantedMediumContent != Medium::ContentAll )
        return Medium::mediaRequestString( d->wantedMediumContent );
    else
        return Medium::mediaRequestString( d->wantedMediumType, d->wantedMediumState, d->wantedMediumSize );
}

QStringList K3b::MediaSelectionComboBox::noMediumMessages() const
{
    if( d->wantedMediumContent && d->wantedMediumContent != Medium::ContentAll )
        return QStringList( Medium::mediaRequestString( d->wantedMediumContent ) );
    else
        return Medium::mediaRequestStrings( d->excludedMediums, d->wantedMediumType, d->wantedMediumState, d->wantedMediumSize );
}


void K3b::MediaSelectionComboBox::slotUpdateToolTip( K3b::Device::Device* dev )
{
    // update the tooltip for the combobox (the tooltip for the dropdown box is created in addMedium)
    setToolTip( dev ? mediumToolTip( k3bappcore->mediaCache()->medium( dev ) ) : QString() );
}


