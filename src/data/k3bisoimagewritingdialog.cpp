/***************************************************************************
                          k3bisoimagewritingdialog.cpp  -  description
                             -------------------
    begin                : Fri Nov 30 2001
    copyright            : (C) 2001 by Sebastian Trueg
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

#include "k3bisoimagewritingdialog.h"
#include "../k3b.h"
#include "../device/k3bdevicemanager.h"
#include "../device/k3bdevice.h"
#include "../k3bwriterselectionwidget.h"
#include "../k3bburnprogressdialog.h"
#include "k3bisoimagejob.h"

#include <klocale.h>
#include <kmessagebox.h>
#include <klineedit.h>
#include <kfiledialog.h>

#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qcombobox.h>
#include <qframe.h>
#include <qlayout.h>
#include <qptrlist.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qfont.h>
#include <qfontmetrics.h>
#include <qtoolbutton.h>



K3bIsoImageWritingDialog::K3bIsoImageWritingDialog( QWidget* parent, const char* name, bool modal )
  : KDialogBase( parent, name, modal, i18n("Write ISO image to cd"), KDialogBase::Ok|KDialogBase::Close,
		 KDialogBase::Ok, true )
{
  setupGui();
  setButtonBoxOrientation( Qt::Vertical );

  m_job = 0;
}


K3bIsoImageWritingDialog::~K3bIsoImageWritingDialog()
{
  delete m_job;
}


void K3bIsoImageWritingDialog::setupGui()
{
  QFrame* frame = makeMainWidget();

  m_writerSelectionWidget = new K3bWriterSelectionWidget( frame );

  // image group box
  // -----------------------------------------------------------------------
  QGroupBox* groupImage = new QGroupBox( i18n("Image to write"), frame );
  groupImage->setColumnLayout(0, Qt::Vertical );
  groupImage->layout()->setSpacing( 0 );
  groupImage->layout()->setMargin( 0 );
  QHBoxLayout* groupImageLayout = new QHBoxLayout( groupImage->layout() );
  groupImageLayout->setAlignment( Qt::AlignTop );
  groupImageLayout->setSpacing( spacingHint() );
  groupImageLayout->setMargin( marginHint() );

  m_editImagePath = new KLineEdit( groupImage );
  m_buttonFindImageFile = new QToolButton( groupImage );
  m_buttonFindImageFile->setText( "..." );
  m_labelImageSize = new QLabel( groupImage );
  m_labelImageSize->font().setBold(true);
  m_labelImageSize->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  m_labelImageSize->setMinimumWidth( m_labelImageSize->fontMetrics().width( "000 000 kb" ) );
  QLabel* textSize = new QLabel( i18n("Size:"), groupImage );

  groupImageLayout->addWidget( m_editImagePath );
  groupImageLayout->addWidget( m_buttonFindImageFile );
  groupImageLayout->addWidget( textSize );
  groupImageLayout->addWidget( m_labelImageSize );
  // -----------------------------------------------------------------------


  // options
  // -----------------------------------------------------------------------
  QGroupBox* groupOptions = new QGroupBox( i18n("Options"), frame );
  groupOptions->setColumnLayout(0, Qt::Vertical );
  groupOptions->layout()->setSpacing( 0 );
  groupOptions->layout()->setMargin( 0 );
  QGridLayout* groupOptionsLayout = new QGridLayout( groupOptions->layout() );
  groupOptionsLayout->setAlignment( Qt::AlignTop );
  groupOptionsLayout->setSpacing( spacingHint() );
  groupOptionsLayout->setMargin( marginHint() );

  m_checkBurnProof = new QCheckBox( i18n("Burnproof"), groupOptions );
  m_checkDummy = new QCheckBox( i18n("Simulate"), groupOptions );
  m_checkDao = new QCheckBox( i18n("Disc at once"), groupOptions );

  groupOptionsLayout->addWidget( m_checkDummy, 0, 0 );
  groupOptionsLayout->addWidget( m_checkDao, 0, 1 );
  groupOptionsLayout->addWidget( m_checkBurnProof, 1, 0 );
  // -----------------------------------------------------------------------



  QGridLayout* grid = new QGridLayout( frame );
  grid->setSpacing( spacingHint() );
  grid->setMargin( marginHint() );

  grid->addWidget( m_writerSelectionWidget, 0, 0 );
  grid->addWidget( groupImage, 1, 0 );
  grid->addWidget( groupOptions, 2, 0 );

  connect( m_editImagePath, SIGNAL(textChanged(const QString&)), this, SLOT(updateImageSize(const QString&)) );
  connect( m_buttonFindImageFile, SIGNAL(clicked()), this, SLOT(slotFindImageFile()) );


  if( m_writerSelectionWidget->writerDevice()->burnproof() )
    m_checkBurnProof->setChecked( true );
  else
    m_checkBurnProof->setDisabled( true );
  m_checkDao->setChecked( true );
  m_checkDummy->setChecked( false );
}


void K3bIsoImageWritingDialog::slotOk()
{
  // check if the image exists
  if( !QFile::exists( m_editImagePath->text() ) ) {
    KMessageBox::error( this, i18n("Could not find file %1").arg(m_editImagePath->text()) );
    return;
  }

  // TODO: check if it is really an iso-image

  // create the job
  if( m_job == 0 )
    m_job = new K3bIsoImageJob();

  m_job->setWriter( m_writerSelectionWidget->writerDevice() );
  m_job->setSpeed( m_writerSelectionWidget->writerSpeed() );
  m_job->setBurnproof( m_checkBurnProof->isChecked() );
  m_job->setDummy( m_checkDummy->isChecked() );
  m_job->setDao( m_checkDao->isChecked() );
  m_job->setImagePath( m_editImagePath->text() );

  // create a progresswidget
  K3bBurnProgressDialog* d = new K3bBurnProgressDialog( k3bMain(), "burnProgress", false );
  connect( d, SIGNAL(closed()), this, SLOT(show()) );

  d->setJob( m_job );
  hide();
  d->show();

  m_job->start();
}


void K3bIsoImageWritingDialog::updateImageSize( const QString& path )
{
  if( QFile::exists( path ) ) {
    QFileInfo info( path );
    if( info.isFile() ) {
      QString s = QString::number( info.size()/1024 );
      int i = s.length() - 3;
      while( i > 0 ) {
	s.insert( i, ' ' );
	i -= 3;
      }
      m_labelImageSize->setText( s + " kb" );
    }
  }
  else {
    m_labelImageSize->setText( "0 kb" );
  }
}


void K3bIsoImageWritingDialog::slotFindImageFile()
{
  QString newPath( KFileDialog::getOpenFileName( m_editImagePath->text(), QString::null, this, i18n("Choose iso image file") ) );
  if( !newPath.isEmpty() )
    m_editImagePath->setText( newPath );
}


void K3bIsoImageWritingDialog::slotWriterChanged()
{
  if( !m_writerSelectionWidget->writerDevice()->burnproof() ) {
    m_checkBurnProof->setChecked( false );
    m_checkBurnProof->setDisabled( true );
  }
  else {
    m_checkBurnProof->setEnabled( true );
  }
}


#include "k3bisoimagewritingdialog.moc"
