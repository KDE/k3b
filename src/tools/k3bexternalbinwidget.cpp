#include "k3bexternalbinwidget.h"
#include "k3bexternalbinmanager.h"

#include <klistview.h>

#include <qpushbutton.h>
#include <kdebug.h>
#include <klocale.h>
#include <keditlistbox.h>
#include <qtabwidget.h>
#include <qlayout.h>
#include <kdialog.h>
#include <qheader.h>
#include <qlabel.h>
#include <qstringlist.h>
#include <qmap.h>
#include <kiconloader.h>
#include <qfont.h>
#include <qpainter.h>


class K3bExternalBinViewItem;


class K3bExternalBinWidget::K3bExternalProgramViewItem : public KListViewItem
{
public:
  K3bExternalProgramViewItem( K3bExternalProgram* p, QListView* parent );

  K3bExternalProgram* program() const { return m_program; }

protected:
  void paintCell( QPainter* p, const QColorGroup& cg, int column, int width, int align ) {
    if( column == 0 ) {
      QFont f( p->font() );
      f.setBold(true);
      p->setFont(f);
    }

    KListViewItem::paintCell(p, cg, column, width, align );
  }

private:
  K3bExternalProgram* m_program;
};



class K3bExternalBinWidget::K3bExternalBinViewItem : public KListViewItem
{
public:
  K3bExternalBinViewItem( K3bExternalBin* bin, K3bExternalProgramViewItem* parent );

  K3bExternalBin* bin() const { return m_bin; }
  K3bExternalProgramViewItem* parentProgramItem() const { return m_parent; }

  bool isDefault() const { return m_default; }
  void setDefault( bool b ) {
    m_default = b;
    if( b )
      setPixmap( 0, SmallIcon( "ok" ) );
    else
      setPixmap( 0, SmallIcon( "gear" ) );
  }

private:
  K3bExternalBin* m_bin;
  K3bExternalProgramViewItem* m_parent;

  bool m_default;
};


K3bExternalBinWidget::K3bExternalProgramViewItem::K3bExternalProgramViewItem( K3bExternalProgram* p, QListView* parent )
  : KListViewItem( parent ), m_program(p)
{
  setText( 0, p->name() );
}


K3bExternalBinWidget::K3bExternalBinViewItem::K3bExternalBinViewItem( K3bExternalBin* bin, K3bExternalProgramViewItem* parent )
  : KListViewItem( parent ), m_bin( bin ), m_parent( parent )
{
  setText( 0, bin->path );
  setText( 1, bin->version );
  setText( 2, bin->features().join(", ") );
  
  setDefault(false);
}





// ///////////////////////////////////////////////////////////
// 
// K3BEXTERNALBINWIDGET
//
// //////////////////////////////////////////////////////////


K3bExternalBinWidget::K3bExternalBinWidget( K3bExternalBinManager* manager, QWidget* parent, const char* name )
  : QWidget( parent, name ), m_manager( manager )
{
  QGridLayout* mainGrid = new QGridLayout( this );
  mainGrid->setMargin( 0 );
  mainGrid->setSpacing( KDialog::spacingHint() );

  m_mainTabWidget = new QTabWidget( this );
  m_rescanButton = new QPushButton( i18n("Search"), this );
  mainGrid->addMultiCellWidget( m_mainTabWidget, 0, 0, 0, 1 );
  mainGrid->addWidget( m_rescanButton, 1, 1 );
  mainGrid->setColStretch( 0, 1 );
  mainGrid->setRowStretch( 0, 1 );


  // setup program tab
  // ------------------------------------------------------------
  QWidget* programTab = new QWidget( m_mainTabWidget );
  QGridLayout* programTabLayout = new QGridLayout( programTab );
  programTabLayout->setMargin( KDialog::marginHint() );
  programTabLayout->setSpacing( KDialog::spacingHint() );
  m_programView = new KListView( programTab );
  m_defaultButton = new QPushButton( i18n("Set Default"), programTab );
  programTabLayout->addMultiCellWidget( m_programView, 1, 1, 0, 1 );
  programTabLayout->addWidget( m_defaultButton, 0, 1 );
  programTabLayout->addWidget( new QLabel( i18n("Use the 'Default' button to change the versions K3b should use."), 
					   programTab ), 0, 0 );
  programTabLayout->setColStretch( 0, 1 );
  programTabLayout->setRowStretch( 1, 1 );

  m_programView->addColumn( i18n("Path") );
  m_programView->addColumn( i18n("Version") );
  m_programView->addColumn( i18n("Features") );
  m_programView->setAllColumnsShowFocus(true);
  m_programView->setFullWidth(true);

  m_mainTabWidget->addTab( programTab, i18n("Programs") );


  // setup parameters tab
  // ------------------------------------------------------------
  QWidget* parametersTab = new QWidget( m_mainTabWidget );
  QGridLayout* parametersTabLayout = new QGridLayout( parametersTab );
  parametersTabLayout->setMargin( KDialog::marginHint() );
  parametersTabLayout->setSpacing( KDialog::spacingHint() );
  m_parameterView = new KListView( parametersTab );
  parametersTabLayout->addWidget( m_parameterView, 1, 0 );
  parametersTabLayout->addWidget( new QLabel( i18n("User Parameters have to be separated by comma."), parametersTab ), 0, 0 );

  parametersTabLayout->setRowStretch( 1, 1 );

  m_parameterView->addColumn( i18n("Program") );
  m_parameterView->addColumn( i18n("Parameters") );
  m_parameterView->setAllColumnsShowFocus(true);
  m_parameterView->setFullWidth(true);
  m_parameterView->setDefaultRenameAction( QListView::Accept );

  m_mainTabWidget->addTab( parametersTab, i18n("User Parameters") );


  // setup search path tab
  // ------------------------------------------------------------
  QWidget* searchPathTab = new QWidget( m_mainTabWidget );
  QGridLayout* searchPathTabLayout = new QGridLayout( searchPathTab );
  searchPathTabLayout->setMargin( KDialog::marginHint() );
  searchPathTabLayout->setSpacing( KDialog::spacingHint() );
  m_searchPathBox = new KEditListBox( i18n("Search Path"), searchPathTab, "searchPathBox", true );
  searchPathTabLayout->addWidget( m_searchPathBox, 0, 0 );
  searchPathTabLayout->addWidget( new QLabel( i18n("<qt><b>Hint:</b> to force K3b to use another than the "
						   "default name for the executable specify it in the search path.</qt>"),
					      searchPathTab ), 1, 0 );

  searchPathTabLayout->setRowStretch( 0, 1 );

  m_mainTabWidget->addTab( searchPathTab, i18n("Search Path") );


  m_programRootItems.setAutoDelete( true );


  connect( m_rescanButton, SIGNAL(clicked()), this, SLOT(rescan()) );
  connect( m_defaultButton, SIGNAL(clicked()), this, SLOT(slotSetDefaultButtonClicked()) );
  connect( m_programView, SIGNAL(selectionChanged(QListViewItem*)), this, SLOT(slotProgramSelectionChanged(QListViewItem*)) );

  slotProgramSelectionChanged( 0 );
}


K3bExternalBinWidget::~K3bExternalBinWidget()
{
}

void K3bExternalBinWidget::rescan()
{
  saveSearchPath();
  m_manager->search();
  load();
}


void K3bExternalBinWidget::load()
{
  m_programRootItems.clear();
  m_parameterView->clear();

  // load programs
  const QMap<QString, K3bExternalProgram*>& map = m_manager->programs();
  for( QMap<QString, K3bExternalProgram*>::const_iterator it = map.begin(); it != map.end(); ++it ) {
    K3bExternalProgram* p = it.data();

    K3bExternalProgramViewItem* pV = new K3bExternalProgramViewItem( p, m_programView );
    m_programRootItems.append( pV );
    // populate it
    QPtrListIterator<K3bExternalBin> binIt( p->bins() );
    for( ; binIt.current(); ++binIt ) {
      K3bExternalBin* b = *binIt;
      
      K3bExternalBinViewItem* bV = new K3bExternalBinViewItem( b, pV );
      if( b == p->defaultBin() )
	bV->setDefault(true);

      pV->setOpen(true);
    }

    if( p->bins().isEmpty() )
      pV->setText( 0, p->name() + i18n(" (not found)") );

    // load user parameters
    K3bExternalProgramViewItem* paraV = new K3bExternalProgramViewItem( p, m_parameterView );
    paraV->setText( 1, p->userParameters().join( ", " ) );
    paraV->setRenameEnabled( 1, true );
  }



  // load search path
  m_searchPathBox->clear();
  m_searchPathBox->insertStringList( m_manager->searchPath() );
}


void K3bExternalBinWidget::save()
{
  saveSearchPath();


  // save the default programs
  QListViewItemIterator progIt( m_programView );
  while( progIt.current() ) {
    if( K3bExternalBinViewItem* bV = dynamic_cast<K3bExternalBinViewItem*>( progIt.current() ) ) {
      if( bV->isDefault() )
	bV->parentProgramItem()->program()->setDefault( bV->bin() );
    }

    ++progIt;
  }


  // save the user parameters
  QListViewItemIterator paraIt( m_parameterView );
  while( paraIt.current() ) {
    K3bExternalProgramViewItem* pV = (K3bExternalProgramViewItem*)paraIt.current();
    pV->program()->setUserParameters( QStringList::split(",", pV->text(1) ) );

    ++paraIt;
  }
}


void K3bExternalBinWidget::saveSearchPath()
{
  m_manager->setSearchPath( m_searchPathBox->items() );
}


void K3bExternalBinWidget::slotSetDefaultButtonClicked()
{
  // check if we are on a binItem
  K3bExternalBinViewItem* item = dynamic_cast<K3bExternalBinViewItem*>( m_programView->selectedItem() );
  if( item ) {
    // remove all default flags
    K3bExternalBinViewItem* bi = (K3bExternalBinViewItem*)item->parentProgramItem()->firstChild();
    QListViewItemIterator it( bi );
    while( it.current()->parent() == item->parentProgramItem() ) {
      ((K3bExternalBinViewItem*)it.current())->setDefault(false);
      ++it;
    }

    item->setDefault(true);
  }
}


void K3bExternalBinWidget::slotProgramSelectionChanged( QListViewItem* item )
{
  K3bExternalBinViewItem* bV = dynamic_cast<K3bExternalBinViewItem*>( item );
  if( bV ) {
    if( bV->isDefault() )
      m_defaultButton->setEnabled(false);
    else
      m_defaultButton->setEnabled(true);
  }
  else
    m_defaultButton->setEnabled(false);
}

#include "k3bexternalbinwidget.moc"
