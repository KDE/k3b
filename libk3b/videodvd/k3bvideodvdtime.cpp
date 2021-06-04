/*
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bvideodvdtime.h"

K3b::VideoDVD::Time::Time()
    : m_hour(0),
      m_minute(0),
      m_second(0),
      m_frame(0)
{
}


K3b::VideoDVD::Time::Time( unsigned short hour,
                           unsigned short min,
                           unsigned short sec,
                           unsigned short frame,
                           double frameRate )
    : m_hour(hour),
      m_minute(min),
      m_second(sec),
      m_frame(frame),
      m_frameRate(frameRate)
{
    // FIXME: how to handle the frames?
    m_minute += m_second/60;
    m_second = m_second % 60;
    m_hour += m_minute/60;
    m_minute = m_minute % 60;
}


double K3b::VideoDVD::Time::totalSeconds() const
{
    double s = (double)second();
    s += 60.0 * (double)minute();
    s += 3600.0 * (double)hour();

    return s + double( frame() ) / frameRate();
}


unsigned int K3b::VideoDVD::Time::totalFrames() const
{
    double f = (double)second();
    f += 60.0 * (double)minute();
    f += 3600.0 * (double)hour();

    return unsigned( f * frameRate() ) + frame();
}


QString K3b::VideoDVD::Time::toString( bool includeFrames ) const
{
    if( includeFrames )
        return QString::asprintf( "%02d:%02d:%02d.%02d",
                                  hour(),
                                  minute(),
                                  second(),
                                  frame() );
    else
        return QString::asprintf( "%02d:%02d:%02d",
                                  hour(),
                                  minute(),
                                  second() + ( frame() > 0 ? 1 : 0 ) );
}
