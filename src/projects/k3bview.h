/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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


#ifndef K3BVIEW_H
#define K3BVIEW_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// include files for Qt
#include <qwidget.h>
#include <q3ptrdict.h>

#include <kxmlguiclient.h>
#include <kurl.h>

class K3bDoc;
class KActionCollection;
class K3bFillStatusDisplay;
class K3bProjectBurnDialog;
class K3bToolBox;
class K3bProjectPlugin;


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
  virtual ~K3bView();
	
  /** 
   * returns a pointer to the document connected to the view
   * @deprecated use doc()
   */
  K3bDoc* getDocument() const { return m_doc; }
  K3bDoc* doc() const { return m_doc; }

  void setMainWidget( QWidget* );

 public slots:
  /**
   * Default impl. brings up the burnDialog via newBurnDialog() with writing
   */
  virtual void slotBurn();

  /**
   * Default impl. brings up the burnDialog via newBurnDialog() without writing
   */
  virtual void slotProperties();

  /**
   * Add an url to the doc. The default implementation simply calls 
   * addUrls.
   */
  virtual void addUrl( const KUrl& );

  /**
   * Add urls to the doc. The default implementation calls doc()->addUrls.
   */
  virtual void addUrls( const KUrl::List& );

 protected:
  /**
   * Protected since the BurnDialog is not part of the API.
   */
  virtual K3bProjectBurnDialog* newBurnDialog( QWidget* = 0, const char* = 0 ) = 0;

  /**
   * Call this to add the projectplugin buttons to the toolbox. It is not called 
   * automatically to make it possible to add other buttons before.
   *
   * @param projectType the type of the project (@see K3bProjectPlugin)
   */
  void addPluginButtons( int projectType );

  K3bFillStatusDisplay* fillStatusDisplay() const { return m_fillStatusDisplay; }
  K3bToolBox* toolBox() const { return m_toolBox; }

 private slots:
  void slotPluginButtonClicked();

 private:
  K3bDoc* m_doc;
  K3bFillStatusDisplay* m_fillStatusDisplay;
  K3bToolBox* m_toolBox;

  Q3PtrDict<K3bProjectPlugin> m_plugins;
};

#endif // K3BVIEW_H
