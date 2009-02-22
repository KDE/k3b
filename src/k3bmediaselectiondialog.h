/* 
 *
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_MEDIA_SELECTION_DIALOG_H_
#define _K3B_MEDIA_SELECTION_DIALOG_H_

#include <kdialog.h>
#include <k3bmedium.h>

namespace K3b {
    class MediaSelectionComboBox;
}
namespace Device {
  class Device;
}

namespace K3b {
class MediaSelectionDialog : public KDialog
{
  Q_OBJECT

 public:
  /**
   * Do not use the constructor. Use the static method instead.
   */
  MediaSelectionDialog( QWidget* parent = 0, 
			   const QString& title = QString(), 
			   const QString& text = QString(), 
			   bool modal = false );
  ~MediaSelectionDialog();

  /**
   * \see MediaSelectionComboBox::setWantedMediumType()
   */
  void setWantedMediumType( int type );

  /**
   * \see MediaSelectionComboBox::setWantedMediumState()
   */
  void setWantedMediumState( int state );

  /**
   * \see MediaSelectionComboBox::setWantedMediumContent()
   */
  void setWantedMediumContent( int state );

  /**
   * Although the dialog is used to select a medium the result is the
   * device containing that medium.
   */
  Device::Device* selectedDevice() const;

  /**
   * \deprecated
   *
   * Select a medium.
   * If only one medium of the wanted type is found the method returns immideately
   * without showing the dialog.
   */
  static Device::Device* selectMedium( int type, int state, QWidget* parent = 0,
					  const QString& title = QString(), 
					  const QString& text = QString(),
					  bool* canceled = 0 );

  static Device::Device* selectMedium( int type, int state, int content = Medium::CONTENT_ALL,
					  QWidget* parent = 0,
					  const QString& title = QString(), 
					  const QString& text = QString(),
					  bool* canceled = 0 );

 private Q_SLOTS:
  void slotSelectionChanged( Device::Device* );

 private:
  MediaSelectionComboBox* m_combo;
};
}

#endif
