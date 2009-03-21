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

#include <k3bdevice.h>
#include <k3bdevicemanager.h>
#include <k3bdeviceglobals.h>
#include <k3bdiskinfo.h>
#include <k3btoc.h>
#include <k3bcdtext.h>

#include <kdebug.h>
#include <klocale.h>

#include <qfont.h>
#include <qmap.h>
#include <qvector.h>
#include <QList>


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
};


K3b::MediaSelectionComboBox::MediaSelectionComboBox( QWidget* parent )
    : KComboBox( false, parent )
{
    d = new Private();

    // set defaults
    d->wantedMediumType = K3b::Device::MEDIA_WRITABLE_CD;
    d->wantedMediumState = K3b::Device::STATE_EMPTY;
    d->wantedMediumContent = K3b::Medium::CONTENT_ALL;

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
}


void K3b::MediaSelectionComboBox::showNoMediumMessage()
{
    // make it italic
    QFont f( font() );
    f.setItalic( true );
    setFont( f );
    addItem( noMediumMessage() );
    setItemData( 0, f, Qt::FontRole );
}


void K3b::MediaSelectionComboBox::updateMedia()
{
    // remember set of devices
    QVector<K3b::Device::Device*> oldDevices = d->devices;

    // remember last selected medium
    K3b::Device::Device* selected = selectedDevice();

    clear();

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

        if( showMedium( medium ) )
            addMedium( *it );
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
                // prefere a newly inserted medium over the previously selected
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
    return( m.diskInfo().mediaType() & d->wantedMediumType &&
            m.content() & d->wantedMediumContent &&
            ( m.diskInfo().diskState() & d->wantedMediumState
              ||
              ( d->wantedMediumState & K3b::Device::STATE_EMPTY &&
                m.diskInfo().rewritable() )
              ||
              ( d->wantedMediumState & K3b::Device::STATE_INCOMPLETE &&
                !m.diskInfo().empty() &&
                m.diskInfo().mediaType() & (K3b::Device::MEDIA_DVD_PLUS_RW|K3b::Device::MEDIA_DVD_RW_OVWR) ) ) &&
            ( d->wantedMediumSize == 0 ||
              d->wantedMediumSize <= m.diskInfo().capacity() )
        );
}


QString K3b::MediaSelectionComboBox::mediumString( const K3b::Medium& medium ) const
{
    return medium.shortString( d->wantedMediumState != K3b::Device::STATE_EMPTY );
}


QString K3b::MediaSelectionComboBox::mediumToolTip( const K3b::Medium& m ) const
{
    return m.longString();
}


QString K3b::MediaSelectionComboBox::noMediumMessage() const
{
    KLocalizedString stateString;
    if( d->wantedMediumContent == K3b::Medium::CONTENT_ALL ) {
        if( d->wantedMediumState == K3b::Device::STATE_EMPTY )
            stateString = ki18n("an empty %1 medium");
        else if( d->wantedMediumState == K3b::Device::STATE_INCOMPLETE )
            stateString = ki18n("an appendable %1 medium");
        else if( d->wantedMediumState == K3b::Device::STATE_COMPLETE )
            stateString = ki18n("a complete %1 medium");
        else if( d->wantedMediumState & (K3b::Device::STATE_EMPTY|K3b::Device::STATE_INCOMPLETE) &&
                 !(d->wantedMediumState & K3b::Device::STATE_COMPLETE) )
            stateString = ki18n("an empty or appendable %1 medium");
        else if( d->wantedMediumState & (K3b::Device::STATE_COMPLETE|K3b::Device::STATE_INCOMPLETE) )
            stateString = ki18n("a complete or appendable %1 medium");
        else
            stateString = ki18n("a %1 medium");
    }
    else {
        //
        // we only handle the combinations that make sense here
        // and if content is requested in does not make sense to
        // also request a specific type of medium (like DVD+RW or DVD-R)
        //
        if( d->wantedMediumContent & K3b::Medium::CONTENT_VIDEO_CD )
            stateString = ki18n("a Video CD medium");
        else if ( d->wantedMediumContent & K3b::Medium::CONTENT_VIDEO_DVD )
            stateString = ki18n("a Video DVD medium");
        else if( d->wantedMediumContent & K3b::Medium::CONTENT_AUDIO &&
                 d->wantedMediumContent & K3b::Medium::CONTENT_DATA )
            stateString = ki18n("a Mixed Mode CD medium");
        else if( d->wantedMediumContent & K3b::Medium::CONTENT_AUDIO )
            stateString = ki18n("an Audio CD medium");
        else if( d->wantedMediumContent & K3b::Medium::CONTENT_DATA ) {
            if ( d->wantedMediumType == K3b::Device::MEDIA_ALL )
                stateString = ki18n("a Data medium");
            else if ( d->wantedMediumType == (K3b::Device::MEDIA_CD_ALL|K3b::Device::MEDIA_DVD_ALL) )
                stateString = ki18n("a Data CD or DVD medium");
            else if ( d->wantedMediumType == K3b::Device::MEDIA_CD_ALL )
                stateString = ki18n("a Data CD medium");
            else if( d->wantedMediumType == K3b::Device::MEDIA_DVD_ALL )
                stateString = ki18n("a Data DVD medium");
            else if ( d->wantedMediumType == K3b::Device::MEDIA_BD_ALL )
                stateString = ki18n("a Data Blu-ray medium");
        }
        else {
            stateString = ki18n("an empty medium");
        }
    }

    // this is basically the same as in K3b::EmptyDiskWaiter
    // FIXME: include things like only rewritable dvd or cd since we will probably need that
    QString mediumString;
    if ( d->wantedMediumType == K3b::Device::MEDIA_WRITABLE )
        mediumString = i18n( "writable" );
    else if( d->wantedMediumType == (K3b::Device::MEDIA_CD_ALL|K3b::Device::MEDIA_DVD_ALL) )
        mediumString = i18n("CD or DVD");
    else if( d->wantedMediumType == K3b::Device::MEDIA_CD_ALL )
        mediumString = i18n("CD");
    else if( d->wantedMediumType == K3b::Device::MEDIA_DVD_ALL )
        mediumString = i18n("DVD");
    else if ( d->wantedMediumType == K3b::Device::MEDIA_BD_ALL )
        mediumString = i18n( "Blu-ray" );
    else if( d->wantedMediumType == ( K3b::Device::MEDIA_WRITABLE_DVD|K3b::Device::MEDIA_WRITABLE_CD) )
        mediumString = i18n("CD-R(W) or DVD%1R(W)",QString("±"));
    else if( d->wantedMediumType == ( K3b::Device::MEDIA_WRITABLE_DVD|K3b::Device::MEDIA_WRITABLE_BD) )
        mediumString = i18n("DVD%1R(W) or BD-R(E)",QString("±"));
    else if( d->wantedMediumType == K3b::Device::MEDIA_WRITABLE_DVD_SL )
        mediumString = i18n("DVD%1R(W)",QString("±"));
    else if( d->wantedMediumType == K3b::Device::MEDIA_WRITABLE_DVD_DL )
        mediumString = i18n("Double Layer DVD%1R",QString("±"));
    else if ( d->wantedMediumType == K3b::Device::MEDIA_WRITABLE_BD )
        mediumString = i18n( "Blu-ray BD-R(E)" );
    else if( d->wantedMediumType == K3b::Device::MEDIA_WRITABLE_CD )
        mediumString = i18n("CD-R(W)");
    else if( d->wantedMediumType == K3b::Device::MEDIA_DVD_ROM )
        mediumString = i18n("DVD-ROM");
    else if( d->wantedMediumType == K3b::Device::MEDIA_CD_ROM )
        mediumString = i18n("CD-ROM");

    if ( stateString.toString().contains( "%1" ) )
        return ki18n("Please insert %1...").subs(stateString.subs( mediumString ).toString() ).toString();
    else
        return ki18n("Please insert %1...").subs(stateString.toString() ).toString();
}


void K3b::MediaSelectionComboBox::slotUpdateToolTip( K3b::Device::Device* dev )
{
    // update the tooltip for the combobox (the tooltip for the dropdown box is created in addMedium)
    setToolTip( dev ? mediumToolTip( k3bappcore->mediaCache()->medium( dev ) ) : QString() );
}

#include "k3bmediaselectioncombobox.moc"
