/***************************************************************************
                          k3bsetup2task.h
                                   -
                       A K3bSetup task
                             -------------------
    begin                : Sun Aug 25 13:19:59 CEST 2002
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


#ifndef K3BSETUP2_TASK_H
#define K3BSETUP2_TASK_H

#include <tools/k3blistview.h>

#include <qstring.h>

class K3bSetup2Task : public K3bListViewItem
{
 public:
  K3bSetup2Task( const QString& text, K3bListView* parent );

  const QString& help() const { return m_help; }
  void setHelp( const QString& t );

  void setFinished( bool success, const QString& errorText = QString::null );

 private:
  QString m_help;
};

#endif
