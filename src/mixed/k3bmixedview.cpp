#include "k3bmixedview.h"

#include "k3bmixeddoc.h"
#include "k3bmixedburndialog.h"
#include "k3bmixeddirtreeview.h"

#include <audio/k3baudiodoc.h>
#include <data/k3bdataviewitem.h>
#include <data/k3bdatafileview.h>
#include <data/k3bdatadoc.h>
#include <audio/audiolistview.h>
#include <audio/k3baudiodoc.h>
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

  m_fillStatusDisplay = new K3bFillStatusDisplay( doc, this );

  QVBoxLayout* box = new QVBoxLayout( this );
  box->addWidget( splitter );
  box->addWidget( m_fillStatusDisplay );
  box->setStretchFactor( splitter, 1 );
  box->setSpacing( 5 );
  box->setMargin( 2 );

  connect( m_mixedDirTreeView, SIGNAL(audioTreeSelected()), 
	   this, SLOT(slotAudioTreeSelected()) );
  connect( m_mixedDirTreeView, SIGNAL(dataTreeSelected()), 
	   this, SLOT(slotDataTreeSelected()) );

  connect( m_audioListView, SIGNAL(lengthReady()), m_fillStatusDisplay, SLOT(update()) );
  connect( m_doc->audioDoc(), SIGNAL(newTracks()), m_fillStatusDisplay, SLOT(update()) );
  connect( m_doc->dataDoc(), SIGNAL(itemRemoved(K3bDataItem*)), m_fillStatusDisplay, SLOT(update()) );
  connect( m_doc->dataDoc(), SIGNAL(newFileItems()), m_fillStatusDisplay, SLOT(update()) );


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


void K3bMixedView::burnDialog( bool withWritingButton )
{
  K3bMixedBurnDialog d( m_doc, this );
  d.exec( withWritingButton );
}


K3bDirItem* K3bMixedView::currentDir() const
{
  if( m_widgetStack->visibleWidget() == m_dataFileView )
    return m_dataFileView->currentDir();
  else
    return 0;
}


#include "k3bmixedview.moc"
