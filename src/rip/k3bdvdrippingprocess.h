/***************************************************************************
                          k3bdvdrippingprocess.h  -  description
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

#ifndef K3BDVDRIPPINGPROCESS_H
#define K3BDVDRIPPINGPROCESS_H

#include <qthread.h>
#include <qobject.h>
#include <qfile.h>
#include <qvaluelist.h>

class KProcess;
class KShellProcess;
class K3bDvdContent;
class K3bDvdCopy;
class K3bDvdAudioGain;
class QWidget;
class K3bExternalBin;
 /**
  *@author Sebastian Trueg
  */

class K3bDvdRippingProcess : public QObject {
    Q_OBJECT
public:
    K3bDvdRippingProcess( QWidget *parent );
    ~K3bDvdRippingProcess();
    void setDvdTitle( const QValueList<K3bDvdContent> &titles );
    void setDirectories( const QString& f, const QString& v, const QString& t ){ m_dirname = f; m_dirvob=v; m_dirtmp=t; };
    void setDevice( const QString& f ){ m_device = f; };
    //void setJob( K3bDvdCopy *job );
    void start( );
    void cancel();
    void setRipSize( double );
    static float tccatParsedBytes( char *text, int len);

signals:
    void interrupted();
    void finished( bool );
    void progressPercent( unsigned int );
    void rippedBytesPerPercent( unsigned long );

private slots:
    //void slotParseError( KProcess *p, char *text, int len);
    void slotParseOutput( KProcess *p, char *text, int len);
    void slotExited( KProcess* );
    void slotAudioProcessFinished();
private:
    QWidget *m_parent;
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
    int m_currentRipAngle;
    int m_currentVobIndex;
    int m_maxTitle;
    unsigned int m_percent;
    double m_rippedBytes;
    double m_titleBytes;
    double m_summaryBytes;
    double m_dataRateBytes;
    QString m_ripMode;
    bool m_processAudio;
    bool m_delAudioProcess;
    bool m_interrupted;
    K3bDvdAudioGain *m_audioProcess;
    //K3bExternalBin *m_tccatBin;

    void checkRippingMode();
    void startRippingProcess();
    QString prepareFilename();
    float getAudioGain();
    void saveConfig();
};

#endif

