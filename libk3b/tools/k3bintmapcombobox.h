/* 
 *
 * $Id: k3bwritingmodewidget.cpp 554512 2006-06-24 07:25:39Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_INT_MAP_COMBOBOX_H_
#define _K3B_INT_MAP_COMBOBOX_H_

#include <kcombobox.h>

#include "k3b_export.h"

/**
 * The K3bIntMapComboBox allows a simple selection of integer
 * values.
 *
 * The K3bIntMapComboBox will create a WhatsThis help automatically from
 * the description texts (if all are set). The ToolTip has to be set manually.
 */
class LIBK3B_EXPORT K3bIntMapComboBox : public KComboBox
{
  Q_OBJECT

 public:
  K3bIntMapComboBox( QWidget* parent = 0, const char* name = 0 );
  ~K3bIntMapComboBox();

  int selectedValue() const;

 signals:
  /**
   * Emitted if the selected value changes by user interaction.
   */
  void valueChanged( int );

  /**
   * Emitted if the current highlighted value changed by user interaction.
   */
  void valueHighlighted( int );

 public slots:
  /**
   * If \a v has not been added via insertItem the selection will not be changed
   */
  void setSelectedValue( int v );

  void clear();

  /**
   * Insert a new item
   * \param value The integer value to insert
   * \param text The text to be displayed in the combobox
   * \param description The text to be used to describe the item in the whatsthis help
   * \param index The position where to inserts the item. The item will be appended if index is negative.
   *
   * \return true if the item could be inserted. False if the value had already been inserted.
   */
  bool insertItem( int value, const QString& text, const QString& description, int index = -1 );

  void addGlobalWhatsThisText( const QString& top, const QString& bottom );

 private slots:
  void slotItemActivated( int );
  void slotItemHighlighted( int );

 private:
  void updateWhatsThis();

  class Private;
  Private* d;
};

#endif
