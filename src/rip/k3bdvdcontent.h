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


#ifndef K3BDVDCONTENT_H
#define K3BDVDCONTENT_H

#include <qstring.h>
#include <qstringlist.h>
#include <qdatetime.h>
#include <qsize.h>

/**
  *@author Sebastian Trueg
  */

class K3bDvdContent {
public: 
    K3bDvdContent();
    ~K3bDvdContent();
    void setInput( const QString& i) { m_sinput = i; };
    void setMode( const QString& i) { m_smode = i; };
    void setRes( const QString& );
    void setAspect( const QString& );
    void setAspectExtension( const QString& s){ m_saspectExtension = s; };
    void setAspectAnamorph( const QString& s){ m_saspectAnamorph = s; }
    void setTime( const QString& );
    void setVideo( const QString& i) { m_saudio = i; };
    void setAudio( const QString& i) { m_svideo = i; };
    void setFrames( const QString& );
    void setFramerate( const QString& );
    void setAudioList( const QStringList & );
    void setNormalize( float f ) { m_normalize = f; };
    void setMaxChapter( int i ){ m_maxChapters = i; };
    void setMaxAudio( int i ){ m_maxAudio = i; };
    void setMaxAngle( unsigned int i ){ m_maxAngle = i; };
    //void setSelectedChapters( QStringList );
    //void setSelectedAudio( QStringList );

    void setTitleNumber( int t ){ m_title = t; };
    void setTitleSet( int t ){ m_titleset = t; };

    QString getInput() { return m_sinput; };
    QString getMode() { return m_smode.upper(); };
    QString getStrRes() { return m_sres; };
    QString getStrAspect() { return m_saspect; };
    QString getStrAspectAnamorph() { return m_saspectAnamorph; };
    QString getStrAspectExtension() { return m_saspectExtension; };
    QString getStrTime() { return m_time.toString(); };
    QString getVideo() { return m_svideo; };
    QString getAudio() { return m_saudio; };
    QString getStrFrames() { return m_sframes; };
    QString getStrFramerate() { return m_sframerate; };
    QStringList* getAudioList() { return &m_audioList; };
    QStringList* getAngles() { return &m_selectedAngle; };
    QStringList* getSelectedChapter() { return &m_selectedChapters; };
    unsigned int getMaxAngle() { return m_maxAngle; }
    QSize& getRes() { return m_res; };
    QSize& getAspect() { return m_aspect; };
    QTime& getTime() { return m_time; };
    long getFrames() { return m_frames; };
    float getFramerate() { return m_framerate; };

    float getNormalize( ) { return m_normalize; };
    int getMaxChapters( ){ return m_maxChapters; };
    int getMaxAudio(  ){ return m_maxAudio; };
    int getTitleNumber(  ){ return m_title; };
    int getTitleSet(  ){ return m_titleset; };

    bool isAllChapters() { return m_allChapters; };
    bool isAllAudio() { return m_allAudio; };
    bool isAllAngle();

    void addAngle( const QString& );
private:
    // view strings
    QString m_sinput, m_smode, m_sres, m_saspect, m_stime;
    QString m_saspectAnamorph;
    QString m_saspectExtension;
    QString m_svideo, m_saudio, m_sframes, m_sframerate;
    QStringList m_audioList;
    // content values
    long m_frames;
    float m_framerate;
    // in seconds
    QTime m_time;
    float m_normalize;
    int m_maxChapters;
    int m_maxAudio;
    unsigned int m_maxAngle;
    int m_title;
    int m_titleset;
    // commaseparated list of entry numbers
    QStringList m_selectedChapters;
    QStringList m_selectedAudio;
    QStringList m_selectedAngle;
    QSize m_res;
    QSize m_aspect;
    // helper
    bool m_allAudio;
    bool m_allChapters;
    bool m_allAngle;
};

#endif
