/***************************************************************************
                          k3bdvdcodecdata.h  -  description
                             -------------------
    begin                : Mon Apr 1 2002
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

#ifndef K3BDVDCODECDATA_H
#define K3BDVDCODECDATA_H

#include <qstring.h>
#include <qstringlist.h>

#include <qdatetime.h>

/**
  *@author Sebastian Trueg
  */

class K3bDvdCodecData {
public: 
    K3bDvdCodecData();
    ~K3bDvdCodecData();
    void setProjectFile( const QString &file );
    void setAviFile( const QString &file ){ m_aviFile = file; };
    QString getAviFile(){ return m_aviFile; };
     // setters for project parser
     void setTitle( const QString& t ){ m_title= t;};
     void setFrames( const QString& f ){ m_frames = f;};
     void setLength( const QString& );
     void setAudioGain( const QString& a ){ m_audioGain = a;} ;
     void setAspectRatio( const QString& );
     void setWidth( const QString& );
     void setHeight( const QString& );
     void setChapters( const QString& c ){ m_chapters = c;};
     void addLanguage( const QString& );
     // getters for project parsed datas
     QString getFrames(){ return m_frames; }
     QStringList getAudioLanguages(){ return m_listAudio; };
     QString getSize();
     QString getAspectRatio(){ return m_aspectRatio;};
     QString getLength() { return m_length; };
     QTime getTime(){ return m_timeLength; };
     //
     QString getProjectDir(){ return m_projectDir; };

private:
    QString m_projectFile;
    QString m_projectDir;
    QString m_aviFile;

    QString m_title;
    QString m_frames;
    QString m_length;
    QTime m_timeLength;
    QString m_audioGain;
    QString m_aspectRatio;
    float m_fAspectRatio;
    QString m_width;
    int m_iWidth;
    QString m_height;
    int m_iHeight;
    QString m_chapters;
    QStringList m_listAudio;

    void loadData();
};

#endif
