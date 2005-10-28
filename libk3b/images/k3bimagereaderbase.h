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


#ifndef _K3B_IMAGEREADER_BASE_H_
#define _K3B_IMAGEREADER_BASE_H_

#include <qstring.h>

#include <k3b_export.h>

class K3bImageSource;
class K3bJobHandler;
class QObject;


// TODO: maybe include a toc in the reader

/**
 * Class K3bImageReaderBase
 * There are basicly two kind of images (from the handling point of view):
 * 
 * \li Images that can properly be extracted by K3b and be used through a normal
 *     K3bImageSource as created by createImageSource().
 * 
 * \li Images that need a special job for writing. These images can have any contents 
 *     and thus cannot be handled generically yet.
 */
class LIBK3B_EXPORT K3bImageReaderBase
{
 public:
  K3bImageReaderBase();
  virtual ~K3bImageReaderBase();
  
  virtual bool open( const QString& file ) = 0;

  virtual bool isOpen() const = 0;

  /**
   * Close the image.
   * Default implementation does noting.
   */
  virtual void close();

  virtual const QString& imageFileName() const { return m_imageFileName; }
      
  /**
   * The tocfile that belongs to the image. For example a cue file or a cdrdao toc or a cdrecord clone toc file.
   * Returns an empty string for simple images like iso9660 images.
   */
  virtual const QString& tocFile() const { return m_tocFile; }
  
  /**
   * Type of the image in one word.
   */
  virtual QString imageType() const = 0;

  /**
   * User readable type of the image. Needs to be internationalized.
   */
  virtual QString imageTypeComment() const = 0;

  /**
   * 
   */
  enum MediaType {
    CD_IMAGE = 1,
    DVD_IMAGE = 2,
    UNSURE = 4
  };

  /**
   * if (mediaType() & UNSURE) is true it might be best to allow the user to choose.
   * The UNSURE flag is basicly only useful for ISO9660 images with a size between
   * 800 and 1000 MB since there are 900 MB CDs.
   */
  virtual int mediaType() const { return CD_IMAGE; }

  /**
   * True if the image type needs special handling with a special job of some kind.
   * This means it canot be written using an image source job created by createImageSource().
   */
  virtual bool needsSpecialHandling() const { return false; }
  
  /**
   * Returns information about the image embedded in html code to allow any easy
   * representation of this read-only data.
   *
   * Example: volume, system, and so on ids of an Iso9660 image.
   *
   * Default implementation returns the size of the image file.
   */
  virtual QString metaInformation() const;

  /**
   * Create an image source which is able to create a K3b usable image.
   * The caller needs to take care of deleting the source.
   *
   * The default implementation returns a simple image source which just 
   * streams the whole image file as set via setImageFileName().
   */
  virtual K3bImageSource* createImageSource( K3bJobHandler*, QObject* parent = 0 ) const;

 protected:
  void setImageFileName( const QString file ) { m_imageFileName = file; }
  void setTocFile( const QString file ) { m_tocFile = file; }

private:
  QString m_imageFileName;
  QString m_tocFile;
};

#endif

