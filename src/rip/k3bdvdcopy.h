/* 
 *
 * $Id$
 * Copyright (C) 2003 Thomas Froescher <tfroescher@k3b.org>
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef K3BDVDCOPY_H
#define K3BDVDCOPY_H

#include <k3bjob.h>
#include <qfile.h>
#include <qvaluelist.h>
#include <qstring.h>
#include <qdatetime.h>

class KProcess;
class K3bDvdContent;
class K3bDvdCopy;
class K3bDvdRippingProcess;

/**
  *@author Sebastian Trueg
  */

class K3bDvdCopy : public K3bJob {
    Q_OBJECT
public:
    K3bDvdCopy( K3bJobHandler*, const QString& device, const QString& directory, const QString& vob, const QString& tmp, const QValueList<K3bDvdContent> &titles, QObject *parent );
    ~K3bDvdCopy();

    void setDvdTitle( const QValueList<K3bDvdContent> &titles );
    void setBaseFilename( const QString& f ){ m_filename = f; };
    void setDevice( const QString& f ){ m_device = f; };
    void setSettings( double size, const QString& angle );
    bool isStartFailed(){ return m_preProcessingFailed; };

    QString jobDescription() const;
    QString jobDetails() const;
		
public slots:
    void start();
    void cancel();
    void slotPercent( int );
    void ripFinished( bool );
    void slotDataRate( unsigned long );

signals:
    void estimatedTime( unsigned int );
    void dataRate( float );
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
    QString m_angle;
    //    QWidget *m_parent;
    K3bDvdRippingProcess *m_ripProcess;
    double m_ripSize;
    QTime m_timeEstimated;
    QTime m_timeDataRate;
    bool m_successfulStarted;
    bool m_preProcessingFailed;

};
	

#endif
