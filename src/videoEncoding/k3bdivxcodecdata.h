/* 
 *
 * $Id$
 * Copyright (C) 2003 Thomas Froescher <tfroescher@k3b.org>
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


#ifndef K3BDIVXCODECDATA_H
#define K3BDIVXCODECDATA_H

#include <qstring.h>
#include <qstringlist.h>

#include <qdatetime.h>


class K3bDivxCodecData {
public:
    K3bDivxCodecData();
    ~K3bDivxCodecData();
    // file/dir settings
    /**
    * Sets the project file and uses the path as project path.
    */
    void setProjectFile( const QString &file );
    void setAviFile( const QString &file ){ m_aviFile = file; };
    QString getAviFile(){ return m_aviFile; };
    QString getProjectDir(){ return m_projectDir; };
    QString getProjectFile(){ return m_projectFile; };
    bool projectLoaded(){ return m_projectLoaded; }

    // setters for project parser
    void setTitle( const QString& t ){ m_title= t;};
    void setFrames( const QString& f ){ m_frames = f;};
    void setLength( const QString& );
    void setAudioGain( const QString& a ){ m_audioGain = a;} ;
    void setAspectRatio( const QString& );
    void setAspectRatioAnamorph( const QString& );
    void setAspectRatioExtension( const QString& v ){ m_aspectExtension=v; }
    void setWidth( const QString& );
    void setHeight( const QString& );
    void setChapters( const QString& c ){ m_chapters = c;};
    void addLanguage( const QString& );
    void addLanguageAc3Bitrate( const QString& );
    void resetAudioLanguages( );
    void setFramerate( const QString& f ){ m_framerate = f; }
    void setMp3CodecMode( int id ){ m_mp3CodecMode = id; }
    // getters for project parsed datas
    QString getTitle(){ return m_title; }
    QString getFrames(){ return m_frames; }
    int getChapters(){ return m_chapters.toInt(); }
    long getFramesValue();
    QStringList getAudioLanguages(){ return m_listAudio; };
    const QString& getAudioLanguageAc3Bitrate( int i ) const;
    QString getSize();
    QString getAspectRatio(){ return m_aspectRatio;};
    float getAspectRatioValue(){ return m_fAspectRatio; }
    QString getAspectRatioExtension(){ return m_aspectExtension; }
    bool isAnamorph() { return m_anamorph; }
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
    // bitrates/codec stuff
    void setAudioBitrate( int kbits ){ m_audioBitrate = kbits; }
    void setVideoBitrate( int kbits ){ m_videoBitrate = kbits; }
    QString getParaAudioBitrate();
    QString getParaVideoBitrate();
    void setCodec( const QString& codec ){ m_codec = codec; }
    void setCodecMode( int m ){ m_codecMode = m; }
    QString getParaCodec();
    int getCodecMode(){ return m_codecMode; }
    //additional encoding settings
    void setDeinterlace( int mode ){ m_deinterlaceMode = mode; }
    QString getParaDeinterlace();
    int getDeinterlace() { return m_deinterlaceMode; }
    void setAudioLanguage( int lang ){ m_audioLanguage = lang; }
    QString getParaAudioLanguage();
    void setAudioResample( int buttonState );
    QString getParaAudioResample(){ return m_audioResample; }
    void setYuv( int buttonState );
    QString getParaYuv(){ return m_yuv; }
    void setKeyframes( const QString &kf ){ m_keyframes = kf; }
    QString getKeyframes() { return m_keyframes; }
    QString getAudioGain( ){ return m_audioGain; }
    void setCrispness( int value );
    QString getCrispness() { return m_crispness; }
    QString getParaAudioGain();
    int getAudioLanguage() { return m_audioLanguage; };
    bool isTcDvdMode() { return m_tcDvdMode; }
    void setTcDvdMode(bool enabled) { m_tcDvdMode=enabled; };
    // ac3 settings
    void setAc3( int );
    QString getParaAc3() { return m_ac3; }
    bool isAc3Set(){ return m_useAc3; }

    // advanced/debug stuff
    void setNormalize( bool b ){ m_useNormalize = b; }
    bool isNormalize() { return m_useNormalize; }
    void setShutdown( bool b ){ m_shutdown = b; }
    bool isShutdown(){ return m_shutdown; }
    //
    int getVideoBitrateValue();
    float getFramerateValue();
private:
    // file/dir settings
    QString m_projectFile;
    QString m_projectDir;
    QString m_aviFile;

    QString m_title;
    QString m_frames;
    QString m_framerate;
    QString m_length;
    QTime m_timeLength;
    QString m_audioGain;
    // aspect ration
    QString m_aspectRatio;
    float m_fAspectRatio;
    bool m_anamorph;
    QString m_aspectExtension;

    QString m_width;
    int m_iWidth;
    QString m_height;
    int m_iHeight;
    QString m_chapters;
    // languages
    QStringList m_listAudio;
    QStringList m_listAudioAc3Bitrate;
    // cropping data
    int m_cropTop;
    int m_cropLeft;
    int m_cropBottom;
    int m_cropRight;
    // resizeing data
    int m_resizeHeight;
    int m_resizeWidth;
    // bitrates/codec stuff
    int m_audioBitrate;
    int m_videoBitrate;
    int m_codecMode; // 1pass, 2pass
    int m_mp3CodecMode; // 0=cbr, 1=vbr
    QString m_ac3; // nothing or " -A -N 0x2000"
    QString m_codec;
    // add encoding settings
    int m_deinterlaceMode; // 0-4
    int m_audioLanguage; // 0-x
    QString m_audioResample;
    QString m_yuv;
    QString m_keyframes;
    QString m_crispness;
    bool m_tcDvdMode;
    bool m_projectLoaded;
    bool m_useAc3;
    bool m_useNormalize;
    bool m_shutdown;

    void loadData();
};

#endif
