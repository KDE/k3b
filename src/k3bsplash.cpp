#include "k3bsplash.h"

#include <qapplication.h>
#include <qlistbox.h>
#include <qpixmap.h>
#include <qevent.h>

#include <kstddirs.h>


K3bSplash::K3bSplash( QWidget* parent, const char* name )
  : QWidget( parent, name, WType_Modal|WStyle_Customize|WStyle_NoBorder|WDestructiveClose )
{
  QPixmap pixmap( "/home/trueg/priv/1/0002.jpg" );

  setBackgroundPixmap( pixmap );

  m_infoBox = new QListBox( this );

  m_infoBox->move( 600, 400 );
  m_infoBox->setFixedSize( 300, 200 );

  setFixedSize( pixmap.size() );

  int w = QApplication::desktop()->width();
  int h = QApplication::desktop()->height();

  move( (w - pixmap.width()) / 2, (h - pixmap.height()) / 2 );
}


K3bSplash::~K3bSplash()
{
}


void K3bSplash::mousePressEvent( QMouseEvent* )
{
  close();
}


void K3bSplash::addInfo( const QString& s )
{
  m_infoBox->insertItem( s );
}
