/* 
 *
 * $Id$
 * Copyright (C) 1998-2004 Thomas Froescher <tfroescher@k3b.org>
 *           (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef K3BDVDRIPPINGPROCESS_H
#define K3BDVDRIPPINGPROCESS_H

#include <k3bjob.h>
#include <qfile.h>
#include <qvaluelist.h>
#include <kio/job.h>

class KProcess;
class KShellProcess;
class K3bDvdContent;
class K3bDvdCopy;
class K3bDvdAudioGain;
class K3bExternalBin;

 /**
  *@author Thomas Froescher
  */
class K3bDvdRippingProcess : public K3bJob {
    Q_OBJECT
public:
    K3bDvdRippingProcess( QObject *parent );
    ~K3bDvdRippingProcess();
    void setDvdTitle( const QValueList<K3bDvdContent>& titles );
    void setDirectories( const QString& f, const QString& v, const QString& t ){ m_dirname = f; m_dirvob=v; m_dirtmp=t; };
    void setDevice( const QString& f ){ m_device = f; };
    //void setJob( K3bDvdCopy *job );
    void start( );
    void cancel();
    void setRipSize( double );
    void setAngle( const QString& angle ){ m_angle= angle; };
    static float tccatParsedBytes( char *text, int len);
    bool isInitFailed(){ return m_preProcessingFailed; }

signals:
    void rippedBytesPerPercent( unsigned long );

private slots:
    //void slotParseError( KProcess *p, char *text, int len);
    void slotParseOutput( KProcess *p, char *text, int len);
    void slotExited( KProcess* );
    //void slotAudioProcessFinished();
    void slotPreProcessingDvd( KIO::Job *resultJob );
    void slotPreProcessingDvd( );
    //void slotJobDebug( KIO::Job *job );
    void slotIfoCopyFinished( KIO::Job *job );
    void slotPreProcessingFinished( KIO::Job *job );
    void slotIfoRename( KIO::Job *job);

private:
    //    QWidget *m_parent;
    QFile m_outputFile;
    QDataStream *m_stream;
    QString m_dirname;
    QString m_dirvob;
    QString m_dirtmp;
    QString m_device;
    typedef QValueList<K3bDvdContent> DvdTitle;
    DvdTitle::Iterator m_dvd;
    DvdTitle m_titles;
    //K3bDvdCopy *m_ripJob;
    KShellProcess *m_ripProcess;
    QString m_title, m_angle;
    int m_currentRipTitle;
    int m_currentVobIndex;
    int m_maxTitle;
    unsigned int m_percent;
    double m_rippedBytes;
    double m_titleBytes;
    double m_summaryBytes;
    double m_dataRateBytes;
    QString m_ripMode;
    QString m_baseFilename;
    // dvd mount directory
    QString m_mountPoint;
    bool m_processAudio;
    bool m_delAudioProcess;
    bool m_interrupted;
    bool m_dvdOrgFilenameDetected;
    // if dvd is already mounted this will be set to true during preProcessingDVD
    bool m_dvdAlreadyMounted;
    // flag to avoid opening ProcessWindow
    bool m_preProcessingFailed;
    //K3bDvdAudioGain *m_audioProcess;
    //K3bExternalBin *m_tccatBin;
     // flag of already tried to mount dvd as iso9660, if true try to mount as udf and mv aller upper_case names to lower_case names
    bool  m_udfMount;
    QString m_videoCaseSensitive;
    QString m_vobCaseSensitive;
    void checkRippingMode();
    void startRippingProcess();
    void preProcessingDvd();
//    void postProcessingDvd();
//    void postProcessingFinished();
    QString getFilename();
    float getAudioGain();
    void saveConfig();
    // generates 01 form 1, 02 from 2 of the current title to rip
    QString formattedTitleset();
    //
    bool copyIfoFiles(const QString&, const QString&, const QString&);
};

#endif

