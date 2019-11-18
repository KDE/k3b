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

#include "k3bdiskinfo.h"
#include "k3btoc.h"
#include "k3bcdtext.h"
#include "k3bdevice.h"
#include "k3biso9660.h"

#include <QSharedDataPointer>
#include <QList>
#include <QIcon>

namespace KCDDB{
    class CDInfo;
}


namespace K3b {
    class MediumPrivate;

    /**
     * Medium represents a medium in K3b.
     *
     * It is implicitly shared, thus copying is very fast.
     */
    class LIBK3B_EXPORT Medium
    {
    public:
        Medium();
        Medium( const Medium& );
        explicit Medium( Device::Device* dev );
        ~Medium();

        /**
         * Copy operator
         */
        Medium& operator=( const Medium& );

        bool isValid() const;

        void setDevice( Device::Device* dev );

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

        Device::Device* device() const;
        Device::DiskInfo diskInfo() const;
        Device::Toc toc() const;
        Device::CdText cdText() const;

        KCDDB::CDInfo cddbInfo() const;

        /**
         * The writing speeds the device supports with the inserted medium.
         * With older devices this list might even be empty for writable
         * media. In that case refer to Device::Device::maxWriteSpeed
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
        QIcon icon() const;

        /**
         * Content type. May be combined by a binary OR.
         */
        enum MediumContent {
            ContentUnknown = 0x0,
            ContentNone = 0x1,
            ContentAudio = 0x2,
            ContentData = 0x4,
            ContentVideoCD = 0x8,
            ContentVideoDVD = 0x10,
            ContentAll = ContentNone|ContentAudio|ContentData|ContentVideoCD|ContentVideoDVD
        };
        Q_DECLARE_FLAGS( MediumContents, MediumContent )

        /**
         * \return a bitwise combination of MediumContent.
         * A VideoCD for example may have the following content:
         * ContentAudio|ContentData|ContentVideoCD
         */
        MediumContents content() const;

        /**
         * \return The volume descriptor from the ISO9660 filesystem.
         */
        const Iso9660SimplePrimaryDescriptor& iso9660Descriptor() const;

        /**
         * The used capacity size on the medium. This only differs from DiskInfo::size()
         * in that it handles rewritable media properly. It uses the size of the filesystem
         * for overwrite media.
         */
        K3b::Msf actuallyUsedCapacity() const;

        /**
         * The remaining size on the medium. This only differs from DiskInfo::remainingSize()
         * in that it handles rewritable media properly. It uses the size of the filesystem
         * for overwrite media.
         */
        K3b::Msf actuallyRemainingSize() const;

        /**
         * Format strings for methods shortString and longString
         */
        enum MediumStringFlag {
            NoStringFlags = 0x0, /**< no flags */
            WithContents = 0x1,  /**< Include the contents, i.e. the volume id or cd text/cddb values. */
            WithDevice = 0x2     /**< Include the device vendor, name, and system name in the long string. */
        };
        Q_DECLARE_FLAGS( MediumStringFlags, MediumStringFlag )

        QString contentTypeString() const;

        /**
         * \return A short one-liner string representing the medium.
         *         This string may be used for labels or selection boxes.
         * \param flags Can be WithContents in which case the content of the CD/DVD will be used, otherwise
         *              the string will simply be something like "empty DVD-R medium".
         *
         * \sa longString, MediumStringFlag
         */
        QString shortString( MediumStringFlags flags = WithContents ) const;

        /**
         * \return A HTML formatted string describing this medium. This includes the device, the
         *         medium type, the contents type, and some detail information like the number of
         *         tracks.
         *         This string may be used for tooltips or short descriptions.
         *
         * \sa shortString, MediumStringFlag
         */
        QString longString( MediumStringFlags flags = WithContents ) const;

        /**
         * Compares the plain medium ignoring the cddb information which can differ
         * based on the cddb settings.
         */
        bool sameMedium( const Medium& other ) const;

        bool operator==( const Medium& other ) const;
        bool operator!=( const Medium& other ) const;

        /**
         * Constructs a user readable string which can be used to request certain media.
         */
        static QString mediaRequestString( Device::MediaTypes requestedMediaTypes,
                                           Device::MediaStates requestedMediaStates,
                                           const K3b::Msf& requestedSize = Msf(),
                                           Device::Device* dev = 0 );

        static QStringList mediaRequestStrings( QList<K3b::Medium> unsuitableMediums,
                                                Device::MediaTypes requestedMediaTypes,
                                                Device::MediaStates requestedMediaStates,
                                                const K3b::Msf& requestedSize = Msf(),
                                                Device::Device* dev = 0 );

        static QString mediaRequestString( MediumContents content, Device::Device* dev = 0 );

    private:
        void analyseContent();

        QSharedDataPointer<MediumPrivate> d;

        friend class MediaCache;
    };
}

Q_DECLARE_OPERATORS_FOR_FLAGS( K3b::Medium::MediumContents )
Q_DECLARE_OPERATORS_FOR_FLAGS( K3b::Medium::MediumStringFlags )

#endif
