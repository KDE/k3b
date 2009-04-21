/*
*
* Copyright (C) 2003-2004 Christian Kvasny <chris@k3b.org>
*             THX to Manfred Odenstein <odix@chello.at>
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

#ifndef K3B_VCD_XMLVIEW_H
#define K3B_VCD_XMLVIEW_H

#include <qstring.h>
#include <qdom.h>
#include <ktemporaryfile.h>

#include "k3bvcddoc.h"

namespace K3b {
    class VcdTrack;

    class VcdXmlView
    {

    public:
        VcdXmlView( VcdDoc* );
        ~VcdXmlView();

        bool write( const QString& );
        QString xmlString()
        {
            return m_xmlstring;
        }

    private:
        QString m_xmlstring;

        void addComment( QDomDocument& doc, QDomElement& parent, const QString& text );
        QDomElement addSubElement( QDomDocument&, QDomElement&, const QString& name, const QString& value = QString() );
        QDomElement addSubElement( QDomDocument&, QDomElement&, const QString& name, const int& value );

        QDomElement addFolderElement( QDomDocument&, QDomElement&, const QString& name );
        void addFileElement( QDomDocument&, QDomElement&, const QString& src, const QString& name, bool mixed = false );
        void doPbc( QDomDocument&, QDomElement&, VcdTrack* );
        void setNumkeyBSN( QDomDocument& , QDomElement&, VcdTrack* );
        void setNumkeySEL( QDomDocument& , QDomElement&, VcdTrack* );
        VcdDoc* m_doc;
        int m_startkey;
    };
}

#endif
