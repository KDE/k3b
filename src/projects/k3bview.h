/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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


#ifndef K3BVIEW_H
#define K3BVIEW_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// include files for Qt
#include <qwidget.h>

class K3bDoc;
class K3bProjectBurnDialog;
class KActionCollection;
class K3bFillStatusDisplay;


/** 
 *
 */
class K3bView : public QWidget
{
  Q_OBJECT

    friend class K3bDoc;

 public:
  /** 
   *
   */
  K3bView( K3bDoc* pDoc, QWidget* parent, const char *name = 0 );
  ~K3bView();
	
  /** 
   * returns a pointer to the document connected to the view
   * @deprecated use doc()
   */
  K3bDoc* getDocument() const { return m_doc; }
  K3bDoc* doc() const { return m_doc; }
	
  virtual void burnDialog( bool withWritingButton = true ) = 0;

  virtual KActionCollection* actionCollection() const;

  void setMainWidget( QWidget* );

 protected:
  /** overwritten QWidget::closeEvent() to catch closing views. Does nothing, as the closeEvents for
   * K3bView's are processed by K3bMainWindow::eventFilter(), so this overwitten closeEvent is necessary
   * and has to be empty. Don't overwrite this method !
   */
  virtual void closeEvent(QCloseEvent* e);

  K3bFillStatusDisplay* fillStatusDisplay() const { return m_fillStatusDisplay; }

 private:
  K3bDoc* m_doc;
  KActionCollection* m_actionCollection;
  K3bFillStatusDisplay* m_fillStatusDisplay;
};

#endif // K3BVIEW_H
