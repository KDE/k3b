
#include "k3bdiskinfoview.h"

#include "k3bdiskinfo.h"

#include <qlabel.h>
#include <qlayout.h>
#include <qfont.h>
#include <qcolor.h>

#include <kdialog.h>
#include <klocale.h>


K3bDiskInfoView::K3bDiskInfoView( QWidget* parent, const char* name )
  : K3bCdContentsView( parent, name )
{
  QFont f( font() );
  f.setPointSize( f.pointSize() + 4 );
  setFont( f );

  setPaletteBackgroundColor( white );

  QHBoxLayout* layout = new QHBoxLayout( this );
  layout->setAutoAdd( true );
  layout->setMargin( KDialog::marginHint() );

  m_label = new QLabel( this );
}


K3bDiskInfoView::~K3bDiskInfoView()
{
}


void K3bDiskInfoView::displayInfo( const K3bDiskInfo& info )
{
  if( info.noDisk ) {
    m_label->setText( i18n("No disk in drive") );
  }
  else if( info.empty ) {
    m_label->setText( i18n("Disk seems to be empty") );
  }
  else {
    m_label->setText( i18n("!!! Disk is not empty. Some view should have shown up !!! ") );
  }
}


#include "k3bdiskinfoview.moc"

