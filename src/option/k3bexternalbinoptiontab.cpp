
#include "k3bexternalbinoptiontab.h"
#include "../tools/k3bexternalbinmanager.h"

#include <kmessagebox.h>
#include <kdialog.h>
#include <klocale.h>
#include <kiconloader.h>
#include <klistview.h>

#include <qgroupbox.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qfile.h>


// TODO: possible solution: make changes direct and check versions
//       and if the user cancels reread the config... not perfect but will work.


K3bExternalBinOptionTab::K3bExternalBinOptionTab( K3bExternalBinManager* manager, QWidget* parent, const char* name )
  : QWidget( parent, name )
{
  m_manager = manager;

  QGridLayout* frameLayout = new QGridLayout( this );
  frameLayout->setSpacing( KDialog::spacingHint() );
  frameLayout->setMargin( KDialog::marginHint() );

  m_viewPrograms = new KListView( this, "m_viewPrograms" );
  m_viewPrograms->addColumn( i18n( "found" ) );
  m_viewPrograms->addColumn( i18n( "Program" ) );
  m_viewPrograms->addColumn( i18n( "version" ) );
  m_viewPrograms->addColumn( i18n( "path" ) );
  m_viewPrograms->addColumn( i18n( "additional parameters" ) );

  // set the second column renameable
  m_viewPrograms->setItemsRenameable( true );
  m_viewPrograms->setRenameable( 0, false );
  m_viewPrograms->setRenameable( 1, false );
  m_viewPrograms->setRenameable( 2, false );
  m_viewPrograms->setRenameable( 3, true );
  m_viewPrograms->setRenameable( 4, true );

  frameLayout->addMultiCellWidget( m_viewPrograms, 1, 1, 0, 1 );

  m_buttonSearch = new QPushButton( this, "m_buttonSearch" );
  m_buttonSearch->setText( i18n( "Search" ) );

  frameLayout->addWidget( m_buttonSearch, 2, 1 );

  QLabel* m_labelInfo = new QLabel( this, "m_labelInfo" );
  m_labelInfo->setText( i18n( "Please specify the paths to the external programs that K3b needs to work properly or press \"Search\" to let K3b search for the programs." ) );
  m_labelInfo->setScaledContents( FALSE );
  m_labelInfo->setAlignment( int( QLabel::WordBreak | QLabel::AlignVCenter | QLabel::AlignLeft ) );

  frameLayout->addMultiCellWidget( m_labelInfo, 0, 0, 0, 1 );
  QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
  frameLayout->addItem( spacer, 2, 0 );

  // TODO: connect the search button to a suitable slot
  m_buttonSearch->setDisabled( true );


  connect( m_viewPrograms, SIGNAL(itemRenamed(QListViewItem*, const QString&, int)),
	   this, SLOT(slotItemRenamed(QListViewItem*, const QString&, int)) );
}


K3bExternalBinOptionTab::~K3bExternalBinOptionTab()
{
}


void K3bExternalBinOptionTab::readSettings()
{
  // clear the view before adding anything!
  m_viewPrograms->clear();
  KListViewItem *item;
  for( int i=0; i < NUM_BIN_PROGRAMS; i++ ) {
    item =  new KListViewItem( m_viewPrograms );
    item->setPixmap( 0, m_manager->foundBin( binPrograms[ i ] ) ? SmallIcon("ok") : SmallIcon("stop") );
    item->setText( 1, binPrograms[ i ] );
    item->setText( 3, m_manager->binPath( binPrograms[ i ] ) );
    K3bExternalBin* cdrecordBin = m_manager->binObject( binPrograms[ i ] );
    if( cdrecordBin ) {
        item->setText( 2, cdrecordBin->version );
        item->setText( 4, cdrecordBin->parameters );
    }
  }
  /*

  KListViewItem* item = new KListViewItem( m_viewPrograms );
  item->setPixmap( 0, m_manager->foundBin( "cdrecord" ) ? SmallIcon("ok") : SmallIcon("stop") );
  item->setText( 1, "cdrecord" );
  item->setText( 3, m_manager->binPath( "cdrecord" ) );
  K3bExternalBin* cdrecordBin = m_manager->binObject( "cdrecord" );
  if( cdrecordBin ) {
    item->setText( 2, cdrecordBin->version );
    item->setText( 4, cdrecordBin->parameters );
  }


  item = new KListViewItem( m_viewPrograms, item );
  item->setPixmap( 0, m_manager->foundBin( "mkisofs" ) ? SmallIcon("ok") : SmallIcon("stop") );
  item->setText( 1, "mkisofs" );
  item->setText( 3, m_manager->binPath( "mkisofs" ) );
  K3bExternalBin* mkisofsBin = m_manager->binObject( "mkisofs" );
  if( mkisofsBin ) {
    item->setText( 2, mkisofsBin->version );
    item->setText( 4, mkisofsBin->parameters );
  }


  item = new KListViewItem( m_viewPrograms, item );
  item->setPixmap( 0, m_manager->foundBin( "cdrdao" ) ? SmallIcon("ok") : SmallIcon("stop") );
  item->setText( 1, "cdrdao" );
  item->setText( 3, m_manager->binPath( "cdrdao" ) );
  K3bExternalBin* cdrdaoBin = m_manager->binObject( "cdrdao" );
  if( cdrdaoBin ) {
    item->setText( 2, cdrdaoBin->version );
    item->setText( 4, cdrdaoBin->parameters );
  }


<<<<<<< k3bexternalbinoptiontab.cpp
  item = new KListViewItem( m_viewPrograms, item );
  item->setPixmap( 0, m_manager->foundBin( "mpg123" ) ? SmallIcon("ok") : SmallIcon("stop") );
  item->setText( 1, "mpg123" );
  item->setText( 3, m_manager->binPath( "mpg123" ) );
  K3bExternalBin* mpg123Bin = m_manager->binObject( "mpg123" );
  if( mpg123Bin ) {
    item->setText( 2, mpg123Bin->version );
    item->setText( 4, mpg123Bin->parameters );
  }


  item = new KListViewItem( m_viewPrograms, item );
  item->setPixmap( 0, m_manager->foundBin( "sox" ) ? SmallIcon("ok") : SmallIcon("stop") );
  item->setText( 1, "sox" );
  item->setText( 3, m_manager->binPath( "sox" ) );
  K3bExternalBin* soxBin = m_manager->binObject( "sox" );
  if( soxBin ) {
    item->setText( 2, soxBin->version );
    item->setText( 4, soxBin->parameters );
  }
    */
//   item = new KListViewItem( m_viewPrograms, item );
//   item->setPixmap( 0, m_manager->foundBin( "mpg123" ) ? SmallIcon("ok") : SmallIcon("stop") );
//   item->setText( 1, "mpg123" );
//   item->setText( 3, m_manager->binPath( "mpg123" ) );
//   K3bExternalBin* mpg123Bin = m_manager->binObject( "mpg123" );
//   if( mpg123Bin ) {
//     item->setText( 2, mpg123Bin->version );
//     item->setText( 4, mpg123Bin->parameters );
//   }


//   item = new KListViewItem( m_viewPrograms, item );
//   item->setPixmap( 0, m_manager->foundBin( "sox" ) ? SmallIcon("ok") : SmallIcon("stop") );
//   item->setText( 1, "sox" );
//   item->setText( 3, m_manager->binPath( "sox" ) );
//   K3bExternalBin* soxBin = m_manager->binObject( "sox" );
//   if( soxBin ) {
//     item->setText( 2, soxBin->version );
//     item->setText( 4, soxBin->parameters );
//   }
}


void K3bExternalBinOptionTab::saveSettings()
{
  QListViewItemIterator it( m_viewPrograms );
  for( ; it.current(); ++it ) {
    QListViewItem* item = it.current();
    K3bExternalBin* binO = m_manager->binObject( item->text(1) );
    if( binO ) {
      binO->path = item->text(3);
      binO->parameters = item->text(4);
    }
  }
}


void K3bExternalBinOptionTab::slotItemRenamed( QListViewItem* item, const QString& newText, int col )
{
  QString bin = item->text(1);
  if( newText.mid( newText.findRev('/')+1 ) == bin &&
      QFile::exists( newText ) ) {
    item->setPixmap( 0, SmallIcon("ok") );
  }
  else {
    item->setPixmap( 0, SmallIcon("stop") );
  }
}


#include "k3bexternalbinoptiontab.moc"
