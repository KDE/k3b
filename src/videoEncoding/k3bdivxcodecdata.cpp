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

/***************************************************************************
                          k3bdvdcodecdata.cpp  -  description
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

#include "k3bdivxcodecdata.h"
#include "k3bdivxprojectparser.h"
#include "k3bdivxavextend.h"

#include "kio/global.h"
#include <kdebug.h>

K3bDivxCodecData::K3bDivxCodecData(){
    m_iWidth=0;
    m_iHeight=0;
    m_resizeHeight=0;
    m_resizeWidth=0;
    m_cropTop=0;
    m_cropLeft=0;
    m_cropBottom=0;
    m_cropRight=0;
    m_crispness="100";
    m_keyframes="300";
    m_audioGain="1.0";
    m_audioBitrate=128;
    m_anamorph = false;
    m_mp3CodecMode = 0; // cbr
    m_audioLanguage=0;
    m_useNormalize = true;
    m_shutdown = false;
    m_tcDvdMode = false;
}

K3bDivxCodecData::~K3bDivxCodecData(){
}

void K3bDivxCodecData::setProjectFile( const QString& file ){

    m_projectFile = file;
    int index = file.findRev("/");
    if( index == -1){
        m_projectDir = "";
    } else {
        m_projectDir = file.left( index );
    }
    kdDebug() << "ProjectDir: " << m_projectDir << endl;
    loadData();
}

void K3bDivxCodecData::loadData( ){
    K3bDivXProjectParser handler( this );
    QFile xmlFile( m_projectFile );
    QXmlInputSource source( xmlFile );
    QXmlSimpleReader reader;
    reader.setContentHandler( &handler );
    if( !reader.parse( source ) ){
        m_projectLoaded = false;
    } else {
        m_projectLoaded = true;
    }
}

void K3bDivxCodecData::setLength( const QString& l){
    m_length = l;
    m_timeLength = QTime::fromString( l );
    kdDebug() << "Time " << m_timeLength.toString() << endl;
}
void K3bDivxCodecData::setAspectRatioAnamorph( const QString& a){
    if( a == "anamorph" ){
        m_anamorph = true;
    } else {
        m_anamorph = false;
    }
}
void K3bDivxCodecData::setAspectRatio( const QString& a){
    m_aspectRatio = a;
    int index = a.find(":");
    QString tmp = a.mid(0, index );
    float w = tmp.toFloat();
    tmp = a.mid( index+1 );
    float h = tmp.toFloat();
    if( h > 0 ){
        m_fAspectRatio =  w / h;
    } else{
         kdDebug() << "(K3bDivxCodecData) error in aspect ratio." << endl;
         m_fAspectRatio = 1.0;
    }
}
void K3bDivxCodecData::setWidth( const QString& w){
    m_width = w;
    m_iWidth = w.toInt();
}
void K3bDivxCodecData::setHeight( const QString& h){
     m_height = h;
     m_iHeight = h.toInt();
}
void K3bDivxCodecData::addLanguage( const QString& l){
    m_listAudio << l;
}

void K3bDivxCodecData::addLanguageAc3Bitrate ( const QString& l){
    m_listAudioAc3Bitrate << l;
    m_useAc3 = true;
}

const QString& K3bDivxCodecData::getAudioLanguageAc3Bitrate( int i ) const{
   return m_listAudioAc3Bitrate[ i ]; 
}

QString K3bDivxCodecData::getSize(){
    return m_width + "x" + m_height;
}
QString K3bDivxCodecData::getParaDeinterlace(){
     // this must be in sync with k3bdivxavextend kombobox deinterlace
     QString result = "";
     if( m_deinterlaceMode <= 4 ){
        result = " -I " + QString::number(m_deinterlaceMode);
     } else if( m_deinterlaceMode == K3bDivxAVExtend::SMARTDEINTER ) {
        result = " -J smartdeinter=diffmode=2:highq=1:cubic=1";
     } else if( m_deinterlaceMode == K3bDivxAVExtend::DILYUVMMX ) {
        result =  " -J dilyuvmmx";
     }
     return result;
}
QString K3bDivxCodecData::getParaAudioLanguage(){
     return " -a " + QString::number(m_audioLanguage);
}

void K3bDivxCodecData::setAudioResample( int buttonState ){
    if( buttonState == 0 ){
        m_audioResample = "";
    } else if( buttonState == 2 ){
        m_audioResample = " -E 44100";
    }
}
void K3bDivxCodecData::setYuv( int buttonState ){
    if( buttonState == 0 ){
        m_yuv = "";
    } else if( buttonState == 2 ){
        m_yuv = " -V";
    }
}

void K3bDivxCodecData::setAc3( int buttonState ){
    if( buttonState == 0 ){
        m_ac3 = "";
    } else if( buttonState == 2 ){
        m_ac3 = " -A -N 0x2000";
    }
}

void K3bDivxCodecData::setCrispness( int value ){
    m_crispness = QString::number( value );
}

int K3bDivxCodecData::getVideoBitrateValue(){
    return m_videoBitrate;
}

float K3bDivxCodecData::getFramerateValue(){
     return m_framerate.toFloat();
}
QString K3bDivxCodecData::getParaAudioGain(){
    return " -s " + m_audioGain;
}
QString K3bDivxCodecData::getParaCodec(){
    return " -y " + m_codec;
}

QString K3bDivxCodecData::getParaAudioBitrate(){
    return " -b " + QString::number(m_audioBitrate) + "," + QString::number( m_mp3CodecMode );
}
QString K3bDivxCodecData::getParaVideoBitrate(){
      return " -w " + QString::number( m_videoBitrate );
}
long K3bDivxCodecData::getFramesValue(){
    return m_frames.toLong();
}
void K3bDivxCodecData::resetAudioLanguages(){
    m_listAudio.clear();
    m_audioLanguage=0;
    m_listAudioAc3Bitrate.clear();
    m_useAc3 = false;
}

