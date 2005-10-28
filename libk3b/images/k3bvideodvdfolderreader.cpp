/* 
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bvideodvdfolderreader.h"
#include "k3bvideodvdfolderimagesource.h"

#include <qdir.h>

#include <klocale.h>
#include <kdebug.h>


K3bVideoDVDFolderReader::K3bVideoDVDFolderReader()
  : K3bImageReaderBase(),
    m_bOpen( false )
{
}


K3bVideoDVDFolderReader::K3bVideoDVDFolderReader( const QString& file )
  : K3bImageReaderBase()
{
  open( file );
}


K3bVideoDVDFolderReader::~K3bVideoDVDFolderReader()
{
}


bool K3bVideoDVDFolderReader::open( const QString& file )
{
  close();

  // check the dir for all video dvd files and set m_volumeId
  QDir mainDir( file );
  if( mainDir.exists() ) {
    if( mainDir.cd( "VIDEO_TS" ) ) {
      // FIXME: now search for the stuff we need
      

      setImageFileName( file );
      m_bOpen = true;
      m_volumeId = mainDir.dirName();
    }
  }

  return isOpen();
}


void K3bVideoDVDFolderReader::close()
{
  m_bOpen = false;
  setImageFileName( QString::null );
}


bool K3bVideoDVDFolderReader::isOpen() const
{
  return m_bOpen;
}


QString K3bVideoDVDFolderReader::imageType() const
{
  return "videodvdfolder";
}


QString K3bVideoDVDFolderReader::imageTypeComment() const
{
  return i18n("Video DVD folder");
}


QString K3bVideoDVDFolderReader::metaInformation() const
{
  // FIXME: number of titles should be enough. No ifo parsing.
  //        we should gather this information in open()
  return QString::null;
}


K3bImageSource* K3bVideoDVDFolderReader::createImageProvider( K3bJobHandler* hdl, QObject* parent ) const
{
  return new K3bVideoDVDFolderImageSource( this, hdl, parent );
}
