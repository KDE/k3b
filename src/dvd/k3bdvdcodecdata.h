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
    // file/dir settings
    void setProjectFile( const QString &file );
    void setAviFile( const QString &file ){ m_aviFile = file; };
    QString getAviFile(){ return m_aviFile; };
    QString getProjectDir(){ return m_projectDir; };
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
    float getAspectRatioValue(){ return m_fAspectRatio; }
    QString getLength() { return m_length; };
    QTime getTime(){ return m_timeLength; };
    int getWidthValue(){ return m_iWidth; }
    int getHeightValue(){ return m_iHeight; }
    QString getWidth(){ return m_width; }
    QString getHeight(){ return m_height; }
    //
    // cropping getter/setter
    int getCropTop(){ return m_cropTop; }
    int getCropLeft(){ return m_cropLeft; }
    int getCropBottom(){ return m_cropBottom; }
    int getCropRight(){ return m_cropRight; }
    void setCropTop(int v){ m_cropTop=v; }
    void setCropLeft(int v){ m_cropLeft=v; }
    void setCropBottom(int v){ m_cropBottom=v; }
    void setCropRight(int v){ m_cropRight=v; }
    // setter/getter resize parameters
    void setResizeHeight( int v ){ m_resizeHeight = v;}
    void setResizeWidth( int v ){ m_resizeWidth = v;}
    int getResizeHeight( ){ return m_resizeHeight;}
    int getResizeWidth( ){ return m_resizeWidth;}
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
    // cropping data
    int m_cropTop;
    int m_cropLeft;
    int m_cropBottom;
    int m_cropRight;
    // resizeing data
    int m_resizeHeight;
    int m_resizeWidth;
    void loadData();
};

#endif
