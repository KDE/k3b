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

#include "k3bmixedview.h"

#include "k3bmixeddoc.h"
#include "k3bmixedburndialog.h"
#include "k3bmixeddirtreeview.h"

#include <k3baudiodoc.h>
#include <k3bdataviewitem.h>
#include <k3bdatafileview.h>
#include <k3bdatadoc.h>
#include <audiolistview.h>
#include <k3baudiodoc.h>
#include <k3bfillstatusdisplay.h>

#include <qwidgetstack.h>
#include <qsplitter.h>
#include <qlayout.h>
#include <qvaluelist.h>

#include <kdialog.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kdebug.h>


K3bMixedView::K3bMixedView( K3bMixedDoc* doc, QWidget* parent, const char* name )
  : K3bView( doc, parent, name ), m_doc(doc)
{
  QSplitter* splitter = new QSplitter( this );
  m_mixedDirTreeView = new K3bMixedDirTreeView( this, doc, splitter );
  m_widgetStack = new QWidgetStack( splitter );
  m_dataFileView = new K3bDataFileView( this, m_mixedDirTreeView, doc->dataDoc(), m_widgetStack );
  m_mixedDirTreeView->setFileView( m_dataFileView );
  m_audioListView = new K3bAudioListView( this, doc->audioDoc(), m_widgetStack );

  setMainWidget( splitter );

  connect( m_mixedDirTreeView, SIGNAL(audioTreeSelected()), 
	   this, SLOT(slotAudioTreeSelected()) );
  connect( m_mixedDirTreeView, SIGNAL(dataTreeSelected()), 
	   this, SLOT(slotDataTreeSelected()) );

  m_widgetStack->raiseWidget( m_dataFileView );

  // split
  QValueList<int> sizes = splitter->sizes();
  int all = sizes[0] + sizes[1];
  sizes[1] = all*2/3;
  sizes[0] = all - sizes[1];
  splitter->setSizes( sizes );

  m_mixedDirTreeView->updateContents();
  m_dataFileView->updateContents();
}


K3bMixedView::~K3bMixedView()
{
}


void K3bMixedView::slotAudioTreeSelected()
{
  m_widgetStack->raiseWidget( m_audioListView );
}


void K3bMixedView::slotDataTreeSelected()
{
  m_widgetStack->raiseWidget( m_dataFileView );
}


K3bDirItem* K3bMixedView::currentDir() const
{
  if( m_widgetStack->visibleWidget() == m_dataFileView )
    return m_dataFileView->currentDir();
  else
    return 0;
}


#include "k3bmixedview.moc"
