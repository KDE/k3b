/***************************************************************************
                          k3bdvddoc.h  -  description
                             -------------------
    begin                : Sun Mar 31 2002
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

#ifndef K3BDVDDOC_H
#define K3BDVDDOC_H

#include "../k3bdoc.h"
class QObject;
class K3bView;

/**
  *@author Sebastian Trueg
  */

class K3bDvdDoc : public K3bDoc  {
     Q_OBJECT
public:
    K3bDvdDoc( QObject* );
    ~K3bDvdDoc();
    /** reimplemented from K3bDoc */
    K3bView* newView( QWidget* parent );
    /** reimplemented from K3bDoc */
    void addView(K3bView* view);
    /** reimplemented from K3bDoc */
    bool newDocument();

    /** reimplemented from K3bDoc */
    long unsigned int size() const { return 0; }
    long unsigned int length() const { return 0; }
    K3bBurnJob* newBurnJob() { return 0; }
    void addUrl( const KURL & ) {};
    void addUrls( const KURL::List& ) {};
    bool loadDocumentData( QDomDocument* ) { return false; }
    bool saveDocumentData( QDomDocument* ) { return false; }
    QString documentType() const { return "k3b_dvd_project"; }
};

#endif
