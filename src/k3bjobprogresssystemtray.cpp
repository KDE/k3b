/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bjobprogresssystemtray.h"

#include <k3bjob.h>

#include <kpixmap.h>
#include <kpixmapeffect.h>
#include <kiconloader.h>
#include <kwin.h>
#include <kstandarddirs.h>
#include <kpopupmenu.h>
#include <klocale.h>
#include <kapplication.h>

#include <qpainter.h>
#include <qevent.h>
#include <qregion.h>
#include <qpointarray.h>
#include <qtooltip.h>


class K3bJobProgressSystemTray::Private
{
public:
  Private() {
    progress = 0;
    lastAnimatedProgress = -1;
    job = 0;
    popupMenu = 0;
  }

  int progress;
  int lastAnimatedProgress;
  KPixmap pixmap;
  KPopupMenu* popupMenu;
  K3bJob* job;
};


K3bJobProgressSystemTray::K3bJobProgressSystemTray( QWidget* parent, const char* name )
  : QWidget( parent, name, WType_TopLevel )
{
  d = new Private();

  // d->popupMenu = new KPopupMenu( this );
//   d->popupMenu->insertTitle( kapp->miniIcon(), i18n("K3b - The CD Kreator") );

  KWin::setSystemTrayWindowFor( winId(), parent->winId() );
}

K3bJobProgressSystemTray::~K3bJobProgressSystemTray()
{
  delete d;
}


void K3bJobProgressSystemTray::setJob( K3bJob* j )
{
  if( d->job )
    disconnect( d->job );

  d->job = j;
  d->progress = 0;
  d->lastAnimatedProgress = -1;
  connect( j, SIGNAL(percent(int)),
	   this, SLOT(slotProgress(int)) );
  connect( j, SIGNAL(finished(bool)),
	   this, SLOT(slotFinished(bool)) );

  QToolTip::remove( this );
  QToolTip::add( this, "K3b - " + d->job->jobDescription() );

  update();
}


void K3bJobProgressSystemTray::slotProgress( int p )
{
  d->progress = p;
  update();
}


void K3bJobProgressSystemTray::slotFinished( bool success )
{
  QToolTip::remove( this );
  if( success )
    QToolTip::add( this, "K3b - " + i18n("Successfully finished") );
  else
    QToolTip::add( this, "K3b - " + i18n("Finished with errors") );
}


void K3bJobProgressSystemTray::paintEvent( QPaintEvent* )
{
  if( d->lastAnimatedProgress < d->progress ) {
    d->lastAnimatedProgress = d->progress;

    static KPixmap logo = MainBarIcon( "k3b", 24 );
    if( logo.height() != 25 )
      logo.resize( 25, 25 ); // much better to handle since 4*25=100 ;) // FIXME

    d->pixmap = logo;

    if( d->progress < 100 ) {
      int percent = d->progress;

      //      KPixmapEffect::intensity( darkLogo, -0.8 );
      KPixmapEffect::toGray( d->pixmap );

      QPointArray pa(7);
      int size = 7;
      pa.setPoint( 0, 13, 0 );  // always the first point
      // calculate the second point
      // if percent > 13 it is the upper right edge
      if( percent > 13 ) {

	// upper right edge
	pa.setPoint( 1, 25, 0 );
	if( percent > 38 ) {

	  // lower right edge
	  pa.setPoint( 2, 25, 25 );
	  if( percent > 38+25 ) {

	    // lower left edge
	    pa.setPoint( 3, 0, 25 );
	    if( percent > 38+25+25 ) {

	      // upper left edge
	      pa.setPoint( 4, 0, 0 );
	      pa.setPoint( 5, percent - (38+25+25), 0 );
	      size = 7;
	    }
	    else {
	      pa.setPoint( 4, 0, 25 - (percent - (38+25)) );
	      size = 6;
	    }
	  }
	  else {
	    pa.setPoint( 3, 25 - (percent-38), 25 );
	    size = 5;
	  }
	}
	else {
	  pa.setPoint( 2, 25, percent-13 );
	  size = 4;
	}
      }
      else {
	pa.setPoint( 1, percent == 0 ? 13 : 12+percent, 0 );
	size = 3;
      }

      pa.setPoint( size-1, 13, 13 );
      pa.resize( size );


      //       for( int i = 0; i < pa.size(); ++i ) {
      // 	printf("(%i, %i) ", pa.point(i).x(), pa.point(i).y() );
      //       }
      //       printf("\n");

      QPainter p( &(d->pixmap) );

      p.setClipRegion( QRegion( pa ) );
      
      p.drawPixmap( 0, 0, logo );
      p.end();
    }
  }

  QPainter p(this);
  p.drawPixmap( 0, 0, d->pixmap );
}


void K3bJobProgressSystemTray::mousePressEvent( QMouseEvent* e )
{
  if( rect().contains( e->pos() ) ) {
    KWin::setActiveWindow( parentWidget()->winId() );
  }
}


#include "k3bjobprogresssystemtray.moc"
