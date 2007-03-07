/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS // needed for *_MAX macros in dvdread headers
#endif

#include "k3bvideodvd.h"

#include <k3bdevice.h>

#include <qfile.h>

#include <klocale.h>

#include <inttypes.h> // needed by dvdreads headers
#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_types.h>
#include <dvdread/ifo_read.h>


// I don't get this stuff, I should read something about VideoDVD some day...
#define CONVERT_TIME(x) (((x & 0xf0) >> 3) * 5 + (x & 0x0f))
#define CONVERT_FRAME(x) (((x & 0x30) >> 3) * 5 + (x & 0x0f))


K3bVideoDVD::VideoDVD::VideoDVD()
{
}


K3bVideoDVD::VideoDVD::~VideoDVD()
{
}


bool K3bVideoDVD::VideoDVD::valid() const
{
  return ( m_device != 0 );
}


bool K3bVideoDVD::VideoDVD::open( K3bDevice::Device* dev )
{
  m_device = 0;
  m_titles.clear();

  //
  // Initialize libdvdread
  //
  dvd_reader_t* dvdReaderT = DVDOpen( QFile::encodeName(dev->blockDeviceName()) );
  if( !dvdReaderT ) {
    kdDebug() << "(K3bVideoDVD) Could not open device " << dev->blockDeviceName() << endl;
    return false;
  }

  //
  // Read volume id
  //
  char v[33];
  if( DVDUDFVolumeInfo( dvdReaderT, v, 33, 0, 0 ) != 0 &&
      DVDISOVolumeInfo( dvdReaderT, v, 33, 0, 0 ) != 0 ) {
    kdDebug() << "(K3bVideoDVD) Could not read volume info." << endl;
    DVDClose( dvdReaderT );
    return false;
  }
  m_volumeIdentifier = QString::fromLatin1( v, 32 );

  //
  // Open the VMG info
  //
  ifo_handle_t* vmg = ifoOpen( dvdReaderT, 0 );
  if( !vmg ) {
    kdDebug() << "(K3bVideoDVD) Can't open VMG info." << endl;
    DVDClose( dvdReaderT );
    return false;
  }

  //
  // parse titles
  //
  m_titles.resize( vmg->tt_srpt->nr_of_srpts );
  for( unsigned int i = 0; i < vmg->tt_srpt->nr_of_srpts; ++i ) {
    title_info_t& title = vmg->tt_srpt->title[i];

    //    m_titles[i].m_videoDVD = this;

    //
    // general title info
    //
    m_titles[i].m_titleNum   = i+1;
    m_titles[i].m_numPTTs    = title.nr_of_ptts;
    m_titles[i].m_numAngles  = title.nr_of_angles;
    m_titles[i].m_titleSet   = title.title_set_nr;
    m_titles[i].m_ttn        = title.vts_ttn;

    //
    // Open the title set the current title is a part of
    //
    ifo_handle_t* titleIfo = ifoOpen( dvdReaderT, vmg->tt_srpt->title[i].title_set_nr );
    if( !titleIfo ) {
      kdDebug() << "(K3bVideoDVD) Can't open Title ifo." << endl;
      ifoClose( vmg );
      DVDClose( dvdReaderT );
      return false;
    }

    //
    // Length of this title
    //
    // the program chain number of the first partoftitle of the current title (FIXME: but a title may contain multiple pgcs)
    int pgc_id = titleIfo->vts_ptt_srpt->title[ m_titles[i].ttn() - 1 ].ptt[0].pgcn;
    // (first?) program chain of the first partoftitle of the current title
    pgc_t* cur_pgc = titleIfo->vts_pgcit->pgci_srp[ pgc_id - 1 ].pgc;

    m_titles[i].m_playbackTime = Time( CONVERT_TIME(cur_pgc->playback_time.hour),
				       CONVERT_TIME(cur_pgc->playback_time.minute),
				       CONVERT_TIME(cur_pgc->playback_time.second),
				       CONVERT_FRAME(cur_pgc->playback_time.frame_u) );

    //
    // Video stream information
    //
    m_titles[i].m_videoStream.m_permittedDf = titleIfo->vtsi_mat->vts_video_attr.permitted_df;
    m_titles[i].m_videoStream.m_displayAspectRatio = titleIfo->vtsi_mat->vts_video_attr.display_aspect_ratio;
    m_titles[i].m_videoStream.m_videoFormat = titleIfo->vtsi_mat->vts_video_attr.video_format;
    m_titles[i].m_videoStream.m_mpegVersion = titleIfo->vtsi_mat->vts_video_attr.mpeg_version;
    m_titles[i].m_videoStream.m_filmMode = titleIfo->vtsi_mat->vts_video_attr.film_mode;
    m_titles[i].m_videoStream.m_letterboxed = titleIfo->vtsi_mat->vts_video_attr.letterboxed;
    m_titles[i].m_videoStream.m_pictureSize = titleIfo->vtsi_mat->vts_video_attr.picture_size;
    m_titles[i].m_videoStream.m_bitRate = titleIfo->vtsi_mat->vts_video_attr.bit_rate;

    //
    // Audio stream information
    //
    m_titles[i].m_audioStreams.resize( titleIfo->vtsi_mat->nr_of_vts_audio_streams );
    for( unsigned int j = 0; j < titleIfo->vtsi_mat->nr_of_vts_audio_streams; ++j ) {
      m_titles[i].m_audioStreams[j].m_format = titleIfo->vtsi_mat->vts_audio_attr[j].audio_format;
      m_titles[i].m_audioStreams[j].m_applicationMode = titleIfo->vtsi_mat->vts_audio_attr[j].application_mode;
      m_titles[i].m_audioStreams[j].m_quantization = titleIfo->vtsi_mat->vts_audio_attr[j].quantization;
      m_titles[i].m_audioStreams[j].m_sampleFrequency = titleIfo->vtsi_mat->vts_audio_attr[j].sample_frequency;
      m_titles[i].m_audioStreams[j].m_codeExtension = titleIfo->vtsi_mat->vts_audio_attr[j].code_extension;
      m_titles[i].m_audioStreams[j].m_multiChannelExt = titleIfo->vtsi_mat->vts_audio_attr[j].multichannel_extension;
      m_titles[i].m_audioStreams[j].m_channels = titleIfo->vtsi_mat->vts_audio_attr[j].channels+1;
      if( titleIfo->vtsi_mat->vts_audio_attr[j].lang_type == 1 )
	m_titles[i].m_audioStreams[j].m_langCode.sprintf( "%c%c", 
							  titleIfo->vtsi_mat->vts_audio_attr[j].lang_code>>8, 
							  titleIfo->vtsi_mat->vts_audio_attr[j].lang_code & 0xff );
      else
	m_titles[i].m_audioStreams[j].m_langCode = QString::null;
    }

    //
    // SubPicture stream information
    //
    m_titles[i].m_subPictureStreams.resize( titleIfo->vtsi_mat->nr_of_vts_subp_streams );
    for( unsigned int j = 0; j < titleIfo->vtsi_mat->nr_of_vts_subp_streams; ++j ) {
      m_titles[i].m_subPictureStreams[j].m_codeMode = titleIfo->vtsi_mat->vts_subp_attr[j].code_mode;
      m_titles[i].m_subPictureStreams[j].m_codeExtension = titleIfo->vtsi_mat->vts_subp_attr[j].code_extension;
      if( titleIfo->vtsi_mat->vts_subp_attr[j].type == 1 )
	m_titles[i].m_subPictureStreams[j].m_langCode.sprintf( "%c%c", 
							       titleIfo->vtsi_mat->vts_subp_attr[j].lang_code>>8, 
							       titleIfo->vtsi_mat->vts_subp_attr[j].lang_code & 0xff );
      else
	m_titles[i].m_subPictureStreams[j].m_langCode = QString::null;
    }

    //
    // add chapter info
    //
    m_titles[i].m_ptts.resize( m_titles[i].numPTTs() );
    for( unsigned int j = 0; j < m_titles[i].numPTTs(); ++j ) {
      m_titles[i].m_ptts[j].m_pttNum = j+1;
      m_titles[i].m_ptts[j].m_playbackTime = Time( CONVERT_TIME(cur_pgc->cell_playback[j].playback_time.hour),
						   CONVERT_TIME(cur_pgc->cell_playback[j].playback_time.minute),
						   CONVERT_TIME(cur_pgc->cell_playback[j].playback_time.second),
						   CONVERT_FRAME(cur_pgc->cell_playback[j].playback_time.frame_u) );
      m_titles[i].m_ptts[j].m_firstSector = cur_pgc->cell_playback[j].first_sector;
      m_titles[i].m_ptts[j].m_lastSector = cur_pgc->cell_playback[j].last_sector;
    }

    ifoClose( titleIfo );
  }

  ifoClose( vmg );
  DVDClose( dvdReaderT );

  //
  // Setting the device makes this a valid instance
  //
  m_device = dev;

  return true;
}


const K3bVideoDVD::Title& K3bVideoDVD::VideoDVD::title( unsigned int num ) const
{
  return m_titles[num];
}


const K3bVideoDVD::Title& K3bVideoDVD::VideoDVD::operator[]( unsigned int num ) const
{
  return title( num );
}


void K3bVideoDVD::VideoDVD::debug() const
{
  kdDebug() << "VideoDVD information:" << endl
	    << "=====================" << endl
	    << "Volume ID: " << volumeIdentifier() << endl << endl;

  for( unsigned int i = 0; i < numTitles(); ++i ) {
    kdDebug() << "Title " << title(i).titleNumber() << " (" << title(i).playbackTime().toString() << ")" << endl
	      << "   Chapters: " << title(i).numPTTs() << endl
	      << "   Angles:   " << title(i).numAngles() << endl
	      << "   VTS,TTN:  " << title(i).titleSet() << "," << title(i).ttn() << endl
	      << "   Audio Streams:" << endl;
    for( unsigned int j = 0; j < title(i).numAudioStreams(); ++j )
      kdDebug() << "      " << title(i).audioStream(j).langCode() << ": " 
		<< audioFormatString( title(i).audioStream(j).format() ) << ", "
		<< audioCodeExtensionString( title(i).audioStream(j).codeExtension() ) << endl;
    kdDebug() << "   SubPicture Streams:" << endl;
    for( unsigned int j = 0; j < title(i).numSubPictureStreams(); ++j )
      kdDebug() << "      " << title(i).subPictureStream(j).langCode() << ": " 
		<< subPictureCodeModeString( title(i).subPictureStream(j).codeMode() ) << ", "
		<< subPictureCodeExtensionString( title(i).subPictureStream(j).codeExtension() ) << endl;
  }
}


QString K3bVideoDVD::audioFormatString( int format )
{
  switch( format ) {
  case AUDIO_FORMAT_AC3:
    return i18n("AC3");
  case AUDIO_FORMAT_MPEG1:
    return i18n("MPEG1");
  case AUDIO_FORMAT_MPEG2EXT:
    return i18n("MPEG2 Extended");
  case AUDIO_FORMAT_LPCM:
    return i18n("LPCM");
  case AUDIO_FORMAT_DTS:
    return i18n("DTS");
  default:
    return i18n("unknown audio format");
  }
}


QString K3bVideoDVD::audioCodeExtensionString( int ext )
{
  switch( ext ) {
  case AUDIO_CODE_EXT_UNSPECIFIED:
    return i18n("Unspecified");
  case AUDIO_CODE_EXT_NORMAL:
    return i18n("Normal");
  case AUDIO_CODE_EXT_VISUALLY_IMPAIRED:
    return i18n("For the visually impaired");
  case AUDIO_CODE_EXT_DIR_COMMENTS_1:
    return i18n("Director's comments 1");
  case AUDIO_CODE_EXT_DIR_COMMENTS_2:
    return i18n("Director's comments 2");
  default:
    return i18n("unknown audio code extension");
  }
}


QString K3bVideoDVD::subPictureCodeModeString( int mode )
{
  switch( mode ) {
  case SUBPIC_CODE_MODE_RLE:
    return i18n("RLE");
  case SUBPIC_CODE_MODE_EXT:
    return i18n("Extended");
  default:
    return i18n("unknown coding mode");
  }
}


QString K3bVideoDVD::subPictureCodeExtensionString( int ext )
{
  switch( ext ) {
  case SUBPIC_CODE_EXT_UNSPECIFIED:
    return i18n("Unspecified");
  case SUBPIC_CODE_EXT_CAPTION_NORMAL_SIZE:
    return i18n("Caption with normal size character");
  case SUBPIC_CODE_EXT_CAPTION_BIGGER_SIZE:
    return i18n("Caption with bigger size character");
  case SUBPIC_CODE_EXT_CAPTION_FOR_CHILDREN:
    return i18n("Caption for children");
  case SUBPIC_CODE_EXT_CLOSED_CAPTION_NORMAL_SIZE:
    return i18n("Closed caption with normal size character");
  case SUBPIC_CODE_EXT_CLOSED_CAPTION_BIGGER_SIZE:
    return i18n("Closed caption with bigger size character");
  case SUBPIC_CODE_EXT_CLOSED_CAPTION_FOR_CHILDREN:
    return i18n("Closed caption for children");
  case SUBPIC_CODE_EXT_FORCED_CAPTION:
    return i18n("Forced caption");
  case SUBPIC_CODE_EXT_DIR_COMMENTS_NORMAL_SIZE:
    return i18n("Director's comments with normal size characters");
  case SUBPIC_CODE_EXT_DIR_COMMENTS_BIGGER_SIZE:
    return i18n("Director's comments with bigger size characters");
  case SUBPIC_CODE_EXT_DIR_COMMENTS_FOR_CHILDREN:
    return i18n("Director's comments for children");
  default:
    return i18n("unknown code extension");
  }
}

