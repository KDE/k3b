/* 
 *
 * $Id: $
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



#ifndef K3BSETUPTAB_H
#define K3BSETUPTAB_H

#include <qvariant.h>
#include <qwidget.h>


class QLabel;
class QGridLayout;
class K3bSetup;
class K3bSetupWizard;
class QPixmap;


class K3bSetupTab : public QWidget
{ 
    Q_OBJECT

 public:
  /**
   * create a new K3bSetupTab and insert it into the K3bSetupWizard
   */
  K3bSetupTab( int, int, const QString& info, K3bSetupWizard* parent, const char* = 0 );
  ~K3bSetupTab();

  virtual void readSettings();
  virtual bool saveSettings();

  virtual bool appropriate();

  virtual void aboutToShow();

  void setPixmap( const QPixmap& );

 protected:
  void setMainWidget( QWidget* );
  K3bSetup* setup() { return m_setup; }

 private:
  class PrivatePicLabel;
  PrivatePicLabel* m_labelSetupLogo;

  QWidget* m_mainWidget;

  QGridLayout* m_mainLayout;

  K3bSetup* m_setup;

  bool m_initialized;
};

#endif
