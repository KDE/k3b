/***************************************************************************
                          k3bemptydiscwaiter.h  -  description
                             -------------------
    begin                : Mon Oct 22 2001
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

#ifndef K3BEMPTYDISCWAITER_H
#define K3BEMPTYDISCWAITER_H

#include <kdialogbase.h>

class QTimer;
class K3bDevice;
class QPushButton;
class QCloseEvent;

/**
 * Tests for an empty cd in a given device.
 * emits signal discReady if an empty disc was found.
 * K3bEmptyDiscWaiter will go on testing until the
 * slot canceled was called or an empty disc was found.
 * After emitting one of the two signals K3bEmptyDiscWaiter
 * will delete itself. There is no need to delete it.
 * @author Sebastian Trueg
 */

class K3bEmptyDiscWaiter : public KDialogBase
{
 Q_OBJECT

 public: 
  K3bEmptyDiscWaiter( K3bDevice* device, QWidget* parent = 0, const char* name = 0 );
  ~K3bEmptyDiscWaiter();

 signals:
  void canceled();
  void discReady();

 public slots:
  void waitForEmptyDisc();

 protected slots:
  void slotCancel();
  void slotUser1();
  void slotTestForEmptyCd();

 protected:
  void closeEvent( QCloseEvent* ) {}

 private:
  QTimer* m_timer;
  K3bDevice* m_device;
  QPushButton* m_buttonCancel;
  QPushButton* m_buttonForce;
};

#endif
