/***************************************************************************
                          k3bsetup.cpp  -  Main Wizard Widget
                             -------------------
    begin                : Sun Aug 25 13:19:59 CEST 2002
    copyright            : (C) 2002 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "k3bsetup2.h"
#include "k3bsetup2finishpage.h"
#include "k3bsetup2task.h"
#include "fstab/k3bsetup2fstabwidget.h"

#include <tools/k3bglobals.h>
#include <tools/k3blistview.h>

#include <device/k3bdevicemanager.h>
#include <tools/k3bexternalbinmanager.h>

#include <klocale.h>
#include <kpushbutton.h>
#include <kstdguiitem.h>
#include <kdebug.h>
#include <kmessagebox.h>

#include <qlabel.h>
#include <qframe.h>
#include <qlayout.h>
#include <qwidgetstack.h>
#include <qfont.h>




K3bSetup2::K3bSetup2( QWidget* parent, const char* name, bool modal, WFlags flags )
  : KDialog( parent, name, modal, flags )
{
  m_externalBinManager = new K3bExternalBinManager( this );
  m_deviceManager = new K3bDeviceManager( m_externalBinManager, this );

  setupGui();
  m_numberOfPages = 0;
  setupPages();

  connect( m_nextButton, SIGNAL(clicked()), this, SLOT(next()) );
  connect( m_backButton, SIGNAL(clicked()), this, SLOT(back()) );
  connect( m_finishButton, SIGNAL(clicked()), this, SLOT(accept()) );
  connect( m_closeButton, SIGNAL(clicked()), this, SLOT(close()) );

  connect( m_taskView, SIGNAL(editorButtonClicked( K3bListViewItem*, int )),
	   this, SLOT(slotTaskViewButtonClicked(K3bListViewItem*, int )) );

  showPage( 1 );
}


K3bSetup2::~K3bSetup2()
{
}


void K3bSetup2::setupGui()
{
  m_headerLabel = new QLabel( this );
  QLabel* nameLabel = new QLabel( "K3bSetup 2", this );

  QFont f( m_headerLabel->font() );
  f.setBold(true);
  m_headerLabel->setFont(f);
  nameLabel->setFont(f);
  nameLabel->setAlignment( AlignVCenter | AlignRight );

  m_pageStack = new QWidgetStack( this );

  m_closeButton = new KPushButton( KStdGuiItem::close(), this );
  m_nextButton = new KPushButton( KStdGuiItem::forward(), this );
  m_backButton = new KPushButton( KStdGuiItem::back(), this );
  m_finishButton = new KPushButton( KStdGuiItem::apply(), this );

  m_taskView = new K3bListView( this );
  m_taskView->addColumn( i18n("Tasks to perform with current settings") );
  m_taskView->addColumn( i18n("Status") );
  m_taskView->setFullWidth( true );

  // setup button row
  QHBoxLayout* buttonLayout = new QHBoxLayout;
  buttonLayout->setMargin( 0 );
  buttonLayout->setSpacing( spacingHint() );
  buttonLayout->addWidget( m_closeButton );
  buttonLayout->addItem( new QSpacerItem( 1, 1, QSizePolicy::Expanding ) );
  buttonLayout->addWidget( m_backButton );
  buttonLayout->addWidget( m_nextButton );
  buttonLayout->addWidget( m_finishButton );

  // create the main layout
  QGridLayout* mainGrid = new QGridLayout( this );
  mainGrid->setSpacing( spacingHint() );
  mainGrid->setMargin( marginHint() );

  mainGrid->addWidget( m_headerLabel, 0, 0 );
  mainGrid->addWidget( nameLabel, 0, 1 );
  QFrame* line = new QFrame( this );
  line->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  mainGrid->addMultiCellWidget( line, 1, 1, 0, 1 );
  mainGrid->addMultiCellWidget( m_pageStack, 2, 2, 0, 1 );
  line = new QFrame( this );
  line->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  mainGrid->addMultiCellWidget( line, 3, 3, 0, 1 );
  mainGrid->addMultiCellLayout( buttonLayout, 4, 4, 0, 1 );
  mainGrid->addMultiCellWidget( m_taskView, 5, 5, 0, 1 );
}


void K3bSetup2::setupPages()
{
  K3bSetup2Page* page = new K3bSetup2FstabWidget( m_deviceManager, m_taskView, m_pageStack );
  addPage( page, 1 );
  page = new K3bSetup2FinishPage( m_pageStack );
  addPage( page, 2 );
}


void K3bSetup2::init()
{
  // TODO: show a dialog

  m_externalBinManager->search();
  m_deviceManager->scanbus();

  // load all pages
  for( int i = 1; m_pageStack->widget(i); ++i )
    ((K3bSetup2Page*)m_pageStack->widget(i))->load(0);
}


void K3bSetup2::addPage( K3bSetup2Page* page, int id )
{
  m_pageStack->addWidget( page, id );
  m_numberOfPages++;
}


void K3bSetup2::accept()
{
  // TODO: let the pages set the status of
  // their tasks to "done"

  ((K3bSetup2FinishPage*)m_pageStack->widget(m_numberOfPages))->showBusy(true);
  m_closeButton->setDisabled(true);
  m_backButton->setDisabled(true);
  m_finishButton->setDisabled(true);

  // save all pages
  bool success = true;
  for( int i = 1; m_pageStack->widget(i); ++i )
    success = success && ((K3bSetup2Page*)m_pageStack->widget(i))->save(0);

  ((K3bSetup2FinishPage*)m_pageStack->widget(m_numberOfPages))->showBusy(false);
  m_closeButton->setEnabled(true);
  m_backButton->setEnabled(true);
  m_finishButton->setEnabled(true);

  if( success ) {
    KMessageBox::information( this, i18n("All settings have been successfully saved."), i18n("Success") );
    KDialog::accept();
  }
  else
    KMessageBox::error( this, i18n("An error occured while applying the tasks. "
				   "See the taskview for further details."), i18n("Error") );
}


void K3bSetup2::close()
{
  KDialog::close();
}


void K3bSetup2::next()
{
  if( m_visiblePageIndex < m_numberOfPages ) {
    showPage( m_visiblePageIndex+1 );
  }
}


void K3bSetup2::back()
{
  if( m_visiblePageIndex > 1 ) {
    showPage( m_visiblePageIndex-1 );
  }
}


void K3bSetup2::showPage( int id )
{
  m_pageStack->raiseWidget(id);
  m_headerLabel->setText( i18n("Step %1 of %2").arg(id).arg(m_numberOfPages) );
  m_visiblePageIndex = id;
  if( m_visiblePageIndex == m_numberOfPages ) {
    m_finishButton->show();
    m_nextButton->hide();
  }
  else {
    m_finishButton->hide();
    m_nextButton->show();
  }

  m_backButton->setDisabled(m_visiblePageIndex == 1);
}


void K3bSetup2::slotTaskViewButtonClicked( K3bListViewItem* item, int )
{
  K3bSetup2Task* task = (K3bSetup2Task*)item;
  KMessageBox::information( this, task->help(), task->text(0) + QString(" (%1)").arg(i18n("Help")) );
}

#include "k3bsetup2.moc"
