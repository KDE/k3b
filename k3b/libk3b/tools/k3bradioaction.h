/* 
 *
 * $Id$
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

#ifndef _K3B_RADIO_ACTION_H_
#define _K3B_RADIO_ACTION_H_

#include <kactionclasses.h>
#include "k3b_export.h"

/**
 * This differs from KRadioAction only in the boolean 
 * flag which says if it should always emit the signals 
 * even if it was checked twice.
 *
 * Docu copied from kdelibs
 */
class LIBK3B_EXPORT K3bRadioAction : public KToggleAction
{
  Q_OBJECT

 public:
  /**
   * Constructs a radio action with text and potential keyboard
   * accelerator but nothing else. Use this only if you really
   * know what you are doing.
   *
   * @param text The text that will be displayed.
   * @param cut The corresponding keyboard accelerator (shortcut).
   * @param parent This action's parent.
   * @param name An internal name for this action.
   */
  K3bRadioAction( const QString& text, const KShortcut& cut = KShortcut(), QObject* parent = 0, const char* name = 0 );

  /**
   *  @param text The text that will be displayed.
   *  @param cut The corresponding keyboard accelerator (shortcut).
   *  @param receiver The SLOT's parent.
   *  @param slot The SLOT to invoke to execute this action.
   *  @param parent This action's parent.
   *  @param name An internal name for this action.
   */
  K3bRadioAction( const QString& text, const KShortcut& cut,
                  const QObject* receiver, const char* slot, QObject* parent, const char* name = 0 );

  /**
   *  @param text The text that will be displayed.
   *  @param pix The icons that go with this action.
   *  @param cut The corresponding keyboard accelerator (shortcut).
   *  @param parent This action's parent.
   *  @param name An internal name for this action.
   */
  K3bRadioAction( const QString& text, const QIconSet& pix, const KShortcut& cut = KShortcut(),
                  QObject* parent = 0, const char* name = 0 );

  /**
   *  @param text The text that will be displayed.
   *  @param pix The dynamically loaded icon that goes with this action.
   *  @param cut The corresponding keyboard accelerator (shortcut).
   *  @param parent This action's parent.
   *  @param name An internal name for this action.
   */
  K3bRadioAction( const QString& text, const QString& pix, const KShortcut& cut = KShortcut(),
                  QObject* parent = 0, const char* name = 0 );

  /**
   *  @param text The text that will be displayed.
   *  @param pix The icons that go with this action.
   *  @param cut The corresponding keyboard accelerator (shortcut).
   *  @param receiver The SLOT's parent.
   *  @param slot The SLOT to invoke to execute this action.
   *  @param parent This action's parent.
   *  @param name An internal name for this action.
   */
  K3bRadioAction( const QString& text, const QIconSet& pix, const KShortcut& cut,
                  const QObject* receiver, const char* slot, QObject* parent, const char* name = 0 );

  /**
   *  @param text The text that will be displayed.
   *  @param pix The dynamically loaded icon that goes with this action.
   *  @param cut The corresponding keyboard accelerator (shortcut).
   *  @param receiver The SLOT's parent.
   *  @param slot The SLOT to invoke to execute this action.
   *  @param parent This action's parent.
   *  @param name An internal name for this action.
   */
  K3bRadioAction( const QString& text, const QString& pix, const KShortcut& cut,
                  const QObject* receiver, const char* slot,
                  QObject* parent, const char* name = 0 );

  /**
   *  @param parent This action's parent.
   *  @param name An internal name for this action.
   */
  K3bRadioAction( QObject* parent = 0, const char* name = 0 );

  /**
   * @param b if true the action will always emit the activated signal
   *          even if the toggled state did not change. The default is false.
   *          which is the same behaviour as KRadioAction
   */
  void setAlwaysEmitActivated( bool b ) { m_alwaysEmit = b; }

 protected:
  virtual void slotActivated();

 private:
  bool m_alwaysEmit;
};

#endif
