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

#include <kxmlguiclient.h>

class K3bDoc;
class KActionCollection;
class K3bFillStatusDisplay;


/** 
 *
 */
class K3bView : public QWidget, public KXMLGUIClient
{
  Q_OBJECT

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

  //  virtual KActionCollection* actionCollection() const;

  void setMainWidget( QWidget* );

 protected:
  K3bFillStatusDisplay* fillStatusDisplay() const { return m_fillStatusDisplay; }

 private:
  K3bDoc* m_doc;
  //  KActionCollection* m_actionCollection;
  K3bFillStatusDisplay* m_fillStatusDisplay;
};

#endif // K3BVIEW_H
