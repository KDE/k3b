
#include "k3bdiskinfoview.h"

#include "k3bdiskinfo.h"

#include <qlabel.h>


K3bDiskInfoView::K3bDiskInfoView( QWidget* parent, const char* name )
  : K3bCdContentsView( parent, name )
{
  QLabel* label = new QLabel( "cd info", this );
}


K3bDiskInfoView::~K3bDiskInfoView()
{
}


void K3bDiskInfoView::displayInfo( const K3bDiskInfo& info )
{
}


#include "k3bdiskinfoview.moc"

