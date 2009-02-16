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

#ifndef _K3B_MEDIUM_H_
#define _K3B_MEDIUM_H_

#include "k3b_export.h"

#include <k3bdiskinfo.h>
#include <k3btoc.h>
#include <k3bcdtext.h>
#include <k3bdevice.h>
#include <k3biso9660.h>

#include <KIcon>

#include <QtCore/QSharedDataPointer>
#include <QList>

namespace KCDDB{
    class CDInfo;
}


class K3bMediumPrivate;

/**
 * K3bMedium represents a medium in K3b.
 *
 * It is implicetely shared, thus copying is very fast.
 */
class LIBK3B_EXPORT K3bMedium
{
public:
    K3bMedium();
    K3bMedium( const K3bMedium& );
    explicit K3bMedium( K3bDevice::Device* dev );
    ~K3bMedium();

    /**
     * Copy operator
     */
    K3bMedium& operator=( const K3bMedium& );

    bool isValid() const;

    void setDevice( K3bDevice::Device* dev );

    /**
     * Resets everything to default values except the device.
     * This means empty toc, cd text, no writing speeds, and a diskinfo
     * with state UNKNOWN.
     */
    void reset();

    /**
     * Updates the medium information if the device is not null.
     * Do not use this in the GUI thread since it uses blocking
     * K3bdevice methods.
     */
    void update();

    K3bDevice::Device* device() const;
    K3bDevice::DiskInfo diskInfo() const;
    K3bDevice::Toc toc() const;
    K3bDevice::CdText cdText() const;

    KCDDB::CDInfo cddbInfo() const;

    /**
     * The writing speeds the device supports with the inserted medium.
     * With older devices this list might even be empty for writable
     * media. In that case refer to K3bDevice::Device::maxWriteSpeed
     * combined with a manual speed selection.
     */
    QList<int> writingSpeeds() const;
    QString volumeId() const;

    /**
     * This method tries to make a volume identificator witch uses a reduced character set
     * look more beautiful by, for example, replacing '_' with a space or replacing all upper
     * case words.
     *
     * Volume ids already containing spaces or lower case characters are left unchanged.
     */
    QString beautifiedVolumeId() const;

    /**
     * An icon representing the contents of the medium.
     */
    KIcon icon() const;

    /**
     * Content type. May be combined by a binary OR.
     */
    enum MediumContent {
        CONTENT_NONE = 0x1,
        CONTENT_AUDIO = 0x2,
        CONTENT_DATA = 0x4,
        CONTENT_VIDEO_CD = 0x8,
        CONTENT_VIDEO_DVD = 0x10,
        CONTENT_ALL = 0xFF
    };

    /**
     * \return a bitwise combination of MediumContent.
     * A VideoCD for example may have the following content: 
     * CONTENT_AUDIO|CONTENT_DATA|CONTENT_VIDEO_CD
     */
    int content() const;

    /**
     * \return The volume descriptor from the ISO9660 filesystem.
     */
    const K3bIso9660SimplePrimaryDescriptor& iso9660Descriptor() const;

    /**
     * \return A short one-liner string representing the medium.
     *         This string may be used for labels or selection boxes.
     * \param useContent if true the content of the CD/DVD will be used, otherwise
     *                   the string will simply be something like "empty DVD-R medium".
     */
    QString shortString( bool useContent = true ) const;

    /**
     * \return A HTML formatted string decribing this medium. This includes the device, the
     *         medium type, the contents type, and some detail information like the number of
     *         tracks.
     *         This string may be used for tooltips or short descriptions.
     */
    QString longString() const;

    /**
     * Compares the plain medium ignoring the cddb information which can differ
     * based on the cddb settings.
     */
    bool sameMedium( const K3bMedium& other ) const;

    bool operator==( const K3bMedium& other ) const;
    bool operator!=( const K3bMedium& other ) const;

private:
    void analyseContent();

    QSharedDataPointer<K3bMediumPrivate> d;

    friend class K3bMediaCache;
};

#endif
