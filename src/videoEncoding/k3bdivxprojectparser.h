/* 
 *
 * $Id$
 * Copyright (C) 2003 Thomas Froescher <tfroescher@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

/***************************************************************************
                          k3bdivxprojectparser.h  -  description
                             -------------------
    begin                : Sun Apr 21 2002
    copyright            : (C) 2002 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef K3BDIVXPROJECTPARSER_H
#define K3BDIVXPROJECTPARSER_H

#include <qxml.h>
#include <qstring.h>

class K3bDivxCodecData;
/**
  *@author Sebastian Trueg
  */


class K3bDivXProjectParser : public QXmlDefaultHandler  {
public: 
    K3bDivXProjectParser( K3bDivxCodecData* );
    ~K3bDivXProjectParser();
    bool startDocument();
    bool startElement( const QString&, const QString&, const QString&, const QXmlAttributes& );
    bool endElement( const QString&, const QString&, const QString& );
    bool characters( const QString& content );

private:
    K3bDivxCodecData *m_data;
    int m_level;
    QString m_contentTag;
};

#endif
