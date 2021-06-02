/* 

    SPDX-FileCopyrightText: 2006 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bvideodvdvideostream.h"

unsigned int K3b::VideoDVD::VideoStream::pictureWidth() const
{
  switch( pictureSize() ) {
  case VIDEO_PICTURE_SIZE_720:
    return 720;
  case VIDEO_PICTURE_SIZE_704:
    return 704;
  case VIDEO_PICTURE_SIZE_352:
  case VIDEO_PICTURE_SIZE_352_2:
    return 352;
  default:
    return 0;
  }
}


unsigned int K3b::VideoDVD::VideoStream::pictureHeight() const
{
  int height = 480;
  if( format() != 0 )
    height = 576;
  if( pictureSize() == VIDEO_PICTURE_SIZE_352_2 )
    height /= 2;

  return height;
}


unsigned int K3b::VideoDVD::VideoStream::realPictureWidth() const
{
  double aspectRatio = 0.0;
  if( displayAspectRatio() == K3b::VideoDVD::VIDEO_ASPECT_RATIO_4_3 )
    aspectRatio = 4.0/3.0;
  else
    aspectRatio = 16.0/9.0;
  return (int)(aspectRatio * (double)realPictureHeight());
}


unsigned int K3b::VideoDVD::VideoStream::realPictureHeight() const
{
  return pictureHeight();
}
