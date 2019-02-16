/*
 *
 * Copyright (C) 2003-2004 Christian Kvasny <chris@k3b.org>
 * Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef K3B_VCD_OPTIONS_H
#define K3B_VCD_OPTIONS_H

#include "k3b_export.h"
#include <KConfigGroup>
#include <QString>

namespace K3b {
    class LIBK3B_EXPORT VcdOptions
    {
    public:
        VcdOptions();

        enum MPEGVersion {
            VCD11 = 0,
            VCD20 = 1,
            SVCD10 = 2,
            HQVCD10 = 3
        };

        QString volumeId() const
        {
            return m_volumeID;
        }
        QString albumId() const
        {
            return m_albumID;
        }
        QString volumeSetId() const
        {
            return m_volumeSetId;
        }
        QString preparer() const
        {
            return m_preparer;
        }
        QString publisher() const
        {
            return m_publisher;
        }

        QString applicationId() const
        {
            return m_applicationId;
        }
        QString systemId() const
        {
            return m_systemId;
        }

        QString vcdClass() const
        {
            return m_vcdclass;
        }
        QString vcdVersion() const
        {
            return m_vcdversion;
        }

        int PreGapLeadout()
        {
            return m_pregapleadout;
        }
        int PreGapTrack()
        {
            return m_pregaptrack;
        }
        int FrontMarginTrack()
        {
            return m_frontmargintrack;
        }
        int RearMarginTrack()
        {
            return m_rearmargintrack;
        }
        int FrontMarginTrackSVCD()
        {
            return m_frontmargintrackSVCD;
        }
        int RearMarginTrackSVCD()
        {
            return m_rearmargintrackSVCD;
        }

        MPEGVersion mpegVersion() const
        {
            return m_mpegversion;
        }

        int volumeCount() const
        {
            return m_volumeCount;
        }
        int volumeNumber() const
        {
            return m_volumeNumber;
        }

        bool AutoDetect() const
        {
            return m_autodetect;
        }
        bool CdiSupport() const
        {
            return m_cdisupport;
        }
        bool NonCompliantMode() const
        {
            return m_brokensvcdmode;
        }
        bool VCD30interpretation() const
        {
            return m_VCD30interpretation;
        }
        bool Sector2336() const
        {
            return m_sector2336;
        }
        bool UpdateScanOffsets() const
        {
            return m_updatescanoffsets;
        }
        bool RelaxedAps() const
        {
            return m_relaxedaps;
        }
        bool UseGaps() const
        {
            return m_usegaps;
        }
        unsigned long long CDIsize() const
        {
            return m_cdisize;
        }

        void setAlbumId( const QString& s )
        {
            m_albumID = s;
        }
        void setVolumeId( const QString& s )
        {
            m_volumeID = s;
        }
        void setVolumeSetId( const QString& s )
        {
            m_volumeSetId = s;
        }
        void setPreparer( const QString& s )
        {
            m_preparer = s;
        }
        void setPublisher( const QString& s )
        {
            m_publisher = s;
        }

        void setVcdClass( const QString& s )
        {
            m_vcdclass = s;
        }
        void setVcdVersion( const QString& s )
        {
            m_vcdversion = s;
        }

        void setPreGapLeadout( int i )
        {
            m_pregapleadout = i;
        }
        void setPreGapTrack( int i )
        {
            m_pregaptrack = i;
        }
        void setFrontMarginTrack( int i )
        {
            m_frontmargintrack = i;
        }
        void setRearMarginTrack( int i )
        {
            m_rearmargintrack = i;
        }
        void setFrontMarginTrackSVCD( int i )
        {
            m_frontmargintrackSVCD = i;
        }
        void setRearMarginTrackSVCD( int i )
        {
            m_rearmargintrackSVCD = i;
        }

        void setMpegVersion( MPEGVersion v )
        {
            m_mpegversion = v;
        }
        void setVolumeCount( int c )
        {
            m_volumeCount = c;
        }
        void setVolumeNumber( int n )
        {
            m_volumeNumber = n;
        }

        void setAutoDetect( bool b )
        {
            m_autodetect = b;
        }
        void setCdiSupport( bool b )
        {
            m_cdisupport = b;
        }
        void setNonCompliantMode( bool b )
        {
            m_brokensvcdmode = b;
        }
        void setVCD30interpretation( bool b )
        {
            m_VCD30interpretation = b;
        }
        void setSector2336( bool b )
        {
            m_sector2336 = b;
        }
        void setUpdateScanOffsets( bool b )
        {
            m_updatescanoffsets = b;
        }
        void setRelaxedAps( bool b )
        {
            m_relaxedaps = b;
        }
        void setUseGaps( bool b )
        {
            m_usegaps = b;
        }

        bool checkCdiFiles();
        void save( KConfigGroup c );

        static VcdOptions load( const KConfigGroup& c );
        static VcdOptions defaults();

        void setPbcEnabled( bool b )
        {
            m_pbcenabled = b;
        }
        bool PbcEnabled() const
        {
            return m_pbcenabled;
        };
        void setPbcNumkeysEnabled( bool b )
        {
            m_pbcnumkeysenabled = b;
        }
        bool PbcNumkeysEnabled() const
        {
            return m_pbcnumkeysenabled;
        };

        void setPbcPlayTime( int i )
        {
            m_def_pbcplaytime = i;
        }
        int PbcPlayTime( )
        {
            return m_def_pbcplaytime;
        }

        void setPbcWaitTime( int i )
        {
            m_def_pbcwaittime = i;
        }
        int PbcWaitTime( )
        {
            return m_def_pbcwaittime;
        }

        void setSegmentFolder( bool b )
        {
            m_segmentfolder = b;
        }
        bool SegmentFolder() const
        {
            return m_segmentfolder;
        };

        void setRestriction( int i )
        {
            m_restriction = i;
        }
        int Restriction() const
        {
            return m_restriction;
        };
        void increaseSegments( )
        {
            m_segment += 1;
        }
        void decreaseSegments( )
        {
            m_segment -= 1;
        }
        bool haveSegments() const
        {
            return m_segment > 0;
        };
        void increaseSequence( )
        {
            m_sequence += 1;
        }
        void decreaseSequence( )
        {
            m_sequence -= 1;
        }

        bool haveSequence() const
        {
            return m_sequence > 0;
        }

    private:
        int m_restriction;
        int m_segment;
        int m_sequence;

        // pbc
        bool m_pbcenabled;
        bool m_pbcnumkeysenabled;

        // volume descriptor
        QString m_volumeID;
        QString m_albumID;
        QString m_volumeSetId;

        QString m_preparer;
        QString m_publisher;

        QString m_applicationId;
        QString m_systemId;

        QString m_vcdclass;
        QString m_vcdversion;

        int m_pregapleadout;
        int m_pregaptrack;
        int m_frontmargintrack;
        int m_rearmargintrack;
        int m_frontmargintrackSVCD;
        int m_rearmargintrackSVCD;

        MPEGVersion m_mpegversion;
        int m_volumeCount;
        int m_volumeNumber;

        bool m_autodetect;
        bool m_cdisupport;
        bool m_brokensvcdmode;
        bool m_VCD30interpretation;
        bool m_sector2336;
        bool m_updatescanoffsets;
        bool m_relaxedaps;
        bool m_segmentfolder;
        bool m_usegaps;

        int m_def_pbcplaytime;
        int m_def_pbcwaittime;
        unsigned long long m_cdisize;
    };
}

#endif
