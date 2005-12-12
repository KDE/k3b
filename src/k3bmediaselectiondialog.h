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

#ifndef _K3B_MEDIA_SELECTION_DIALOG_H_
#define _K3B_MEDIA_SELECTION_DIALOG_H_

#include <kdialogbase.h>

namespace K3bDevice {
  class Device;
}

class K3bMediaSelectionDialog : public KDialogBase
{
  Q_OBJECT

 public:
  /**
   * Do not use the constructor. Use the static method instead.
   */
  K3bMediaSelectionDialog( QWidget* parent = 0, 
			   const QString& title = QString::null, 
			   const QString& text = QString::null, 
			   bool modal = false );
  ~K3bMediaSelectionDialog();

  /**
   * \see K3bMediaSelectionComboBox::setWantedMediumType()
   */
  void setWantedMediumType( int type );

  /**
   * \see K3bMediaSelectionComboBox::setWantedMediumState()
   */
  void setWantedMediumState( int state );

  /**
   * Although the dialog is used to select a medium the result is the
   * device containing that medium.
   */
  K3bDevice::Device* selectedDevice() const;

  /**
   * Select a medium.
   * If only one medium of the wanted type is found the method returns immideately
   * without showing the dialog.
   */
  static K3bDevice::Device* selectMedium( int type, int state, QWidget* parent = 0,
					  const QString& title = QString::null, 
					  const QString& text = QString::null );

  // FIXME: make K3bDevice::ContentType bitwise combinable and rename it
  //        to TocType
  enum CDTocType {
    TOC_AUDIO = 0x1,
    TOC_MIXED = 0x2,
    TOC_DATA = 0x4
  };

  /**
   * Use this to select a specific non-empty CD type like an audio CD.
   *
   * If only one medium of the wanted type is found the method returns immideately
   * without showing the dialog.
   *
   * \param bitwise combination of CDTocType
   */
  static K3bDevice::Device* selectCDMedium( int content, QWidget* parent = 0,
					    const QString& title = QString::null, 
					    const QString& text = QString::null );

 private slots:
  void slotSelectionChanged( K3bDevice::Device* );

 private:
  class MediumCombo;
  MediumCombo* m_combo;
};

#endif
