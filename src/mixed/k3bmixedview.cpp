#include "k3bmixedview.h"

#include "k3bmixeddoc.h"
#include "k3bmixedburndialog.h"

#include <audio/k3baudiodoc.h>
#include <data/k3bdatadirtreeview.h>
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
  m_dataDirTreeView = new K3bDataDirTreeView( this, doc->dataDoc(), splitter );
  m_widgetStack = new QWidgetStack( splitter );
  m_dataFileView = new K3bDataFileView( this, m_dataDirTreeView, doc->dataDoc(), m_widgetStack );
  m_dataDirTreeView->setFileView( m_dataFileView );
  m_audioListView = new K3bAudioListView( this, doc->audioDoc(), m_widgetStack );

  m_fillStatusDisplay = new K3bFillStatusDisplay( doc, this );

  QVBoxLayout* box = new QVBoxLayout( this );
  box->addWidget( splitter );
  box->addWidget( m_fillStatusDisplay );
  box->setStretchFactor( splitter, 1 );
  box->setSpacing( 5 );
  box->setMargin( 2 );

  m_audioTreeItem = new QListViewItem( m_dataDirTreeView, m_dataDirTreeView->root(), i18n("Audio Tracks") );
  m_audioTreeItem->setPixmap( 0, SmallIcon("sound") );

  connect( m_dataDirTreeView, SIGNAL(selectionChanged(QListViewItem*)), 
	   this, SLOT(treeSelectionChanged(QListViewItem*)) );
  connect( m_dataDirTreeView, SIGNAL(urlsDropped(const KURL::List&, QListViewItem*)),
	   this, SLOT(slotUrlsDropped(const KURL::List&, QListViewItem*)) );

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
}


K3bMixedView::~K3bMixedView()
{
}


void K3bMixedView::treeSelectionChanged( QListViewItem* item )
{
  if( item == m_audioTreeItem )
    m_widgetStack->raiseWidget( m_audioListView );
  else
    m_widgetStack->raiseWidget( m_dataFileView );
}


void K3bMixedView::slotUrlsDropped( const KURL::List& urls, QListViewItem* item )
{
  if( item == m_audioTreeItem )
    m_doc->audioDoc()->addUrls( urls );
}


void K3bMixedView::burnDialog( bool withWritingButton )
{
  K3bMixedBurnDialog d( m_doc, this );
  d.exec( withWritingButton );
}

#include "k3bmixedview.moc"
