/***************************************************************************
                          k3bdvdcopy.h  -  description
                             -------------------
    begin                : Sun Mar 3 2002
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

#ifndef K3BDVDCOPY_H
#define K3BDVDCOPY_H

#include "../k3bjob.h"
#include <qfile.h>
#include <qvaluelist.h>
#include <qstring.h>

class KProcess;
class K3bDvdContent;
class K3bDvdCopy;
class QWidget;
class K3bDvdRippingProcess;

/**
  *@author Sebastian Trueg
  */

class K3bDvdCopy : public K3bJob {
    Q_OBJECT
public:
    K3bDvdCopy(const QString& device, const QString& directory, const QString& vob, const QString& tmp, const QValueList<K3bDvdContent> &titles, QWidget *parent );
    ~K3bDvdCopy();

    void setDvdTitle( const QValueList<K3bDvdContent> &titles );
    void setBaseFilename( const QString& f ){ m_filename = f; };
    void setDevice( const QString& f ){ m_device = f; };

public slots:
    void start();
    void cancel();
    void slotPercent( int );
    void ripFinished( bool );

//    void slotParseError( KProcess *p, char *text, int len);
//    void slotParseOutput( KProcess *p, char *text, int len);
//    void slotExited( KProcess* );

private:
    QFile m_outputFile;
    QDataStream *m_stream;
    QString m_filename;
    //QString m_ioctlDevice;
    typedef QValueList<K3bDvdContent> DvdTitle;
    DvdTitle m_ripTitles;
    QString m_device;
    QString m_directory;
    QString m_dirvob;
    QString m_dirtmp;
    QWidget *m_parent;
    K3bDvdRippingProcess *m_ripProcess;

};
	

#endif
