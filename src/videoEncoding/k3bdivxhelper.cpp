/***************************************************************************
                          k3bdivxhelper.cpp  -  description
                             -------------------
    begin                : Sun Jan 5 2003
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

#include "k3bdivxcodecdata.h"

#include "k3bdivxhelper.h"

#include <qdir.h>
#include <qstringlist.h>

#include <kdebug.h>
#include <kurl.h>
#include <kio/job.h>

K3bDivxHelper::K3bDivxHelper() : QObject() {
}

K3bDivxHelper::~K3bDivxHelper(){
}

void K3bDivxHelper::deleteIfos( K3bDivxCodecData *data){
     // delete ifos
    QDir vobs( data->getProjectDir() + "/vob");
    if( vobs.exists() ){
        QStringList ifos = vobs.entryList("*.ifo");
        for ( QStringList::Iterator it = ifos.begin(); it != ifos.end(); ++it ) {
            (*it) = data->getProjectDir() + "/vob/" +(*it);
        }
        KURL::List ifoList( ifos );
        connect( KIO::del( ifoList, false, false ), SIGNAL( result( KIO::Job *) ), this, SLOT( slotDeleteFinished( ) ) );
        kdDebug() << "(K3bDivxHelper) Delete IFO files in " << vobs.path() << endl;
    }
}

void K3bDivxHelper::slotDeleteFinished(){
    emit finished( true );
}

#include "k3bdivxhelper.moc"
