/*
 *
 * Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
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

#ifndef _K3B_VIDEOCD_RIPPING_OPTIONS_H_
#define _K3B_VIDEOCD_RIPPING_OPTIONS_H_

#include "k3bglobals.h"

#include <KLocalizedString>
#include <QString>

namespace K3b {
class VideoCdRippingOptions
{
    public:
        VideoCdRippingOptions()
            :   m_videocdsize( 0 ),
                m_videocdsource( "/dev/cdrom" ),
                m_videocddestination(K3b::defaultTempPath()),
                m_videocddescription( i18n( "Video CD" ) ),
                m_videocdripfiles( false ),
                m_videocdripsegments( false ),
                m_videocdripsequences( false ),
                m_ignoreExt( false ),
                m_sector2336( false ),
                m_extractXML( false )
        {}

        void setVideoCdSize( unsigned long size ) { m_videocdsize = size;}
        void setVideoCdSource( const QString& source ) { m_videocdsource = source;}
        void setVideoCdDestination( const QString& destination ) { m_videocddestination = destination;}
        void setVideoCdDescription( const QString& description ) { m_videocddescription = description;}
        void setVideoCdRipFiles( bool ripfiles ) { m_videocdripfiles = ripfiles;}
        void setVideoCdRipSegments( bool ripsegments ) { m_videocdripsegments = ripsegments;}
        void setVideoCdRipSequences( bool ripsequences ) { m_videocdripsequences = ripsequences;}
        void setVideoCdIgnoreExt( bool ignoreext ) { m_ignoreExt = ignoreext;}
        void setVideoCdSector2336( bool sector2336 ) { m_sector2336 = sector2336;}
        void setVideoCdExtractXml( bool extractxml ) { m_extractXML = extractxml;}

        unsigned long getVideoCdSize( ) { return m_videocdsize;}
        QString getVideoCdSource( ) { return m_videocdsource;}
        QString getVideoCdDestination( ) { return m_videocddestination;}
        QString getVideoCdDescription( ) { return m_videocddescription;}
        bool getVideoCdRipFiles( ) { return m_videocdripfiles;}
        bool getVideoCdRipSegments( ) { return m_videocdripsegments;}
        bool getVideoCdRipSequences( ) { return m_videocdripsequences;}
        bool getVideoCdIgnoreExt( ) { return m_ignoreExt;}
        bool getVideoCdSector2336( ) { return m_sector2336;}
        bool getVideoCdExtractXml( ) { return m_extractXML;}

    private:
        unsigned long m_videocdsize;

        QString m_videocdsource;
        QString m_videocddestination;
        QString m_videocddescription;
        
        bool m_videocdripfiles;
        bool m_videocdripsegments;
        bool m_videocdripsequences;
        bool m_ignoreExt;
        bool m_sector2336;
        bool m_extractXML;
};
}

#endif
