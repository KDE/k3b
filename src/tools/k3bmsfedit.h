/***************************************************************************
                          k3bmsfedit.h  -  
         An edit widget for MSF (minute,second,frame) values
                             -------------------
    begin                : Mon Mar 26 15:30:59 CEST 2001
    copyright            : (C) 2001 by Sebastian Trueg
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

#ifndef K3B_MSF_EDIT_H
#define K3B_MSF_EDIT_H


#include <qspinbox.h>
#include <qstring.h>


class K3bMsfEdit : public QSpinBox
{
  Q_OBJECT

 public:
  K3bMsfEdit( QWidget* parent = 0, const char* name = 0 );
  ~K3bMsfEdit();

  void setFrameStyle( int style );
  void setLineWidth(int);

 public slots:
  void setText( const QString& );

 protected:
  QString mapValueToText( int );
  int mapTextToValue( bool* ok );
};


#endif
