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


#include "k3bsystemproblemdialog.h"
#include "tools/k3btitlelabel.h"
#include <k3bstdguiitems.h>

#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qlabel.h>

#include <klocale.h>
#include <kstandarddirs.h>
#include <ktextedit.h>
#include <kconfig.h>
#include <kapplication.h>



K3bSystemProblem::K3bSystemProblem( int t,
				    const QString& p,
				    const QString& d,
				    const QString& s,
				    bool k )
  : type(t),
    problem(p),
    details(d),
    solution(s),
    solvableByK3bSetup(k)
{
}


K3bSystemProblemDialog::K3bSystemProblemDialog( const QValueList<K3bSystemProblem>& problems,
						QWidget* parent, 
						const char* name )
  : KDialog( parent, name )
{
  setCaption( i18n("System Configuration Problems") );

  // setup the title
  // ---------------------------------------------------------------------------------------------------
  QFrame* headerFrame = K3bStdGuiItems::purpleFrame( this );
  QHBoxLayout* layout4 = new QHBoxLayout( headerFrame ); 
  layout4->setMargin( 2 ); // to make sure the frame gets displayed
  layout4->setSpacing( 0 );
  QLabel* pixmapLabelLeft = new QLabel( headerFrame, "pixmapLabelLeft" );
  pixmapLabelLeft->setPaletteBackgroundColor( QColor( 205, 210, 255 ) );
  pixmapLabelLeft->setPixmap( QPixmap(locate( "appdata", "pics/diskinfo_left.png" )) );
  pixmapLabelLeft->setScaledContents( FALSE );
  layout4->addWidget( pixmapLabelLeft );
  K3bTitleLabel* labelTitle = new K3bTitleLabel( headerFrame, "m_labelTitle" );
  labelTitle->setTitle( i18n("System Configuration Problems"), i18n("1 problem", "%n problems", problems.count() ) );
  labelTitle->setPaletteBackgroundColor( QColor( 205, 210, 255 ) );
  layout4->addWidget( labelTitle );
  layout4->setStretchFactor( labelTitle, 1 );
  QLabel* pixmapLabelRight = new QLabel( headerFrame, "pixmapLabelRight" );
  pixmapLabelRight->setPaletteBackgroundColor( QColor( 205, 210, 255 ) );
  pixmapLabelRight->setPixmap( QPixmap(locate( "appdata", "pics/diskinfo_right.png" )) );
  pixmapLabelRight->setScaledContents( FALSE );
  layout4->addWidget( pixmapLabelRight );


  m_closeButton = new QPushButton( i18n("Close"), this );
  connect( m_closeButton, SIGNAL(clicked()), this, SLOT(close()) );
  m_checkDontShowAgain = new QCheckBox( i18n("Don't show again"), this );


  // setup the problem view
  // ---------------------------------------------------------------------------------------------------
  KTextEdit* view = new KTextEdit( this );
  view->setReadOnly(true);
  view->setTextFormat(RichText);


  // layout everything
  QGridLayout* grid = new QGridLayout( this );
  grid->setMargin( marginHint() );
  grid->setSpacing( spacingHint() );
  grid->addMultiCellWidget( headerFrame, 0, 0, 0, 1 );
  grid->addMultiCellWidget( view, 1, 1, 0, 1 );
  grid->addWidget( m_checkDontShowAgain, 2, 0 );
  grid->addWidget( m_closeButton, 2, 1 );
  grid->setColStretch( 0, 1 );
  grid->setRowStretch( 1, 1 );

  QString text = "<html>";

  for( QValueList<K3bSystemProblem>::const_iterator it = problems.begin();
       it != problems.end(); ++it ) {
    const K3bSystemProblem& p = *it;

    text.append( "<p><b>" );
    if( p.type == K3bSystemProblem::CRITICAL )
      text.append( "<span style=\"color:red\">" );
    text.append( p.problem );
    if( p.type == K3bSystemProblem::CRITICAL )
      text.append( "</span>" );
    text.append( "</b><br>" );
    text.append( p.details + "<br>" );
    text.append( "<i>" + i18n("Solution") + "</i>: " + p.solution );
    text.append( "</p>" );
  }

  text.append( "</html>" );

  view->setText(text);
  view->setCursorPosition(0,0);
  view->ensureCursorVisible();
}


void K3bSystemProblemDialog::closeEvent( QCloseEvent* e )
{
  if( m_checkDontShowAgain->isChecked() ) {
    kapp->config()->setGroup( "General Options" );
    kapp->config()->writeEntry( "check system config", false );
  }

  e->accept();
}

#include "k3bsystemproblemdialog.moc"
