/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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


#ifndef K3B_MSF_EDIT_H
#define K3B_MSF_EDIT_H


#include <qspinbox.h>
#include <qstring.h>
#include <qvalidator.h>

#include <k3bmsf.h>


class K3bMsfValidator : public QRegExpValidator
{
 public:
  K3bMsfValidator( QObject* parent = 0, const char* name = 0 );
};


class K3bMsfEdit : public QSpinBox
{
  Q_OBJECT

 public:
  K3bMsfEdit( QWidget* parent = 0, const char* name = 0 );
  ~K3bMsfEdit();

  void setFrameStyle( int style );
  void setLineWidth(int);

  K3b::Msf msfValue() const;

 signals:
  void valueChanged( const K3b::Msf& );

 public slots:
  void setValue( int v );
  void setText( const QString& );
  void setMsfValue( const K3b::Msf& );
  void stepUp();
  void stepDown();

 protected:
  QString mapValueToText( int );
  int mapTextToValue( bool* ok );
  int currentStepValue() const;

 private slots:
  void slotValueChanged( int );
};


#endif
