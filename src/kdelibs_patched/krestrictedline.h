/*
 *
 * $Id$
 *
 * Definition of KRestrictedLine
 *
 * Copyright (C) 1997 Michael Wiedmann, <mw@miwie.in-berlin.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef KRESTRICTEDLINE_H
#define KRESTRICTEDLINE_H

#include <klineedit.h>

/** 
 * The KRestrictedLine widget is a variant of @ref QLineEdit which
 * accepts only a restricted set of characters as input. 
 * All other characters will be discarded and the signal @ref #invalidChar() 
 * will be emitted for each of them.
 *
 * Valid characters can be passed as a QString to the constructor
 * or set afterwards via @ref #setValidChars().
 * The default key bindings of @ref QLineEdit are still in effect.
 *
 * @short A line editor for restricted character sets.
 * @author Michael Wiedmann <mw@miwie.in-berlin.de>
 * @version 0.0.1
 */
class KRestrictedLine : public KLineEdit
{
  Q_OBJECT
  Q_PROPERTY( QString validChars READ validChars WRITE setValidChars )
  
public:

  /**
   * Constructor: This contructor takes three - optional - arguments.
   *  The first two parameters are simply passed on to @ref QLineEdit.
   *  @param parent   pointer to the parent widget
   *  @param name     pointer to the name of this widget
   *  @param valid    pointer to set of valid characters 
   */
  KRestrictedLine( QWidget *parent=0, const char *name=0, 
		   const QString& valid = QString::null);

  /**
   * Destructs the restricted line editor.
   */
  ~KRestrictedLine();

  /**
   * All characters in the string valid are treated as 
   * acceptable characters.
   * Sets the mode to check for valid characters.
   */
  void setValidChars(const QString& valid);

  /**
   * All characters in the srting are treated as
   * inaccepable characters.
   * Sets the mode to check for invalid characters.
   */
  void setInvalidChars(const QString& invalid);

  /**
   * @return the string of acceptable characters.
   */
  QString validChars() const;

  /**
   * @return the string of inacceptable characters.
   */
  QString invalidChars() const;
  
signals:

  /**
   * Emitted when an invalid character was typed.
   */
  void	invalidChar(int);

protected:
  /**
   * @reimplemented
   */
  void	keyPressEvent( QKeyEvent *e );

private:
  /// QString of valid characters for this line
  QString	qsValidChars;
  QString       qsInvalidChars;

  bool          bCheckForInvalidChars;
};

#endif // KRESTRICTEDLINE_H
