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


#include "k3bisoimagewritingdialog.h"
#include "k3biso9660imagewritingjob.h"

#include <device/k3bdevicemanager.h>
#include <device/k3bdevice.h>
#include <k3bwriterselectionwidget.h>
#include <k3bburnprogressdialog.h>
#include <kcutlabel.h>
#include <k3bstdguiitems.h>
#include <tools/k3bmd5job.h>

#include <kapplication.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <klineedit.h>
#include <kfiledialog.h>
#include <kstdguiitem.h>
#include <kguiitem.h>
#include <kiconloader.h>
#include <kconfig.h>
#include <kio/global.h>
#include <kurl.h>
#include <kactivelabel.h>
#include <kprogress.h>

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
#include <qpushbutton.h>
#include <qtabwidget.h>
#include <qtooltip.h>
#include <qwhatsthis.h>


K3bIsoImageWritingDialog::K3bIsoImageWritingDialog( QWidget* parent, const char* name, bool modal )
  : KDialogBase( parent, name, modal, i18n("Write Image to CD"), User1|User2,
		 User1, false, KGuiItem( i18n("Write"), "write", i18n("Start writing") ), KStdGuiItem::close() )
{
  setupGui();
  setButtonBoxOrientation( Qt::Vertical );

  m_job = 0;
  m_md5Job = new K3bMd5Job( this );
  connect( m_md5Job, SIGNAL(finished(bool)),
	   this, SLOT(slotMd5JobFinished(bool)) );
  connect( m_md5Job, SIGNAL(percent(int)),
	   m_md5ProgressWidget, SLOT(setProgress(int)) );


  m_checkBurnProof->setChecked( true ); // enabled by default

  slotWriterChanged();

  kapp->config()->setGroup("General Options");
  m_editImagePath->setText( kapp->config()->readEntry( "last written image", "" ) );
  updateImageSize( m_editImagePath->text() );
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
  QGroupBox* groupImage = new QGroupBox( i18n("Image to Write"), frame );
  groupImage->setColumnLayout(0, Qt::Vertical );
  groupImage->layout()->setSpacing( 0 );
  groupImage->layout()->setMargin( 0 );
  QGridLayout* groupImageLayout = new QGridLayout( groupImage->layout() );
  groupImageLayout->setAlignment( Qt::AlignTop );
  groupImageLayout->setSpacing( spacingHint() );
  groupImageLayout->setMargin( marginHint() );

  m_editImagePath = new KLineEdit( groupImage );
  m_buttonFindImageFile = new QToolButton( groupImage );
  m_buttonFindImageFile->setIconSet( SmallIconSet( "fileopen" ) );
  m_labelImageSize = new QLabel( groupImage );

  QFont f( m_labelImageSize->font() );
  f.setBold(true);
  f.setItalic( true );
  m_labelImageSize->setFont( f );
  m_labelImageSize->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  m_labelImageSize->setMinimumWidth( m_labelImageSize->fontMetrics().width( "000 000 kb" ) );
  m_labelImageSize->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );

  m_isoInfoWidget = new QWidget( groupImage );

  QGridLayout* isoInfoLayout = new QGridLayout( m_isoInfoWidget );
  isoInfoLayout->setSpacing( spacingHint() );
  isoInfoLayout->setMargin( 0 );

  //  m_labelIsoId = new KCutLabel( m_isoInfoWidget );
  m_labelIsoSystemId = new KCutLabel( m_isoInfoWidget );
  m_labelIsoVolumeId = new KCutLabel( m_isoInfoWidget );
  m_labelIsoVolumeSetId = new KCutLabel( m_isoInfoWidget );
  m_labelIsoPublisherId = new KCutLabel( m_isoInfoWidget );
  m_labelIsoPreparerId = new KCutLabel( m_isoInfoWidget );
  m_labelIsoApplicationId = new KCutLabel( m_isoInfoWidget );

//   isoInfoLayout->addWidget( new QLabel( i18n("Id:"), m_isoInfoWidget ), 0, 0 );
//   isoInfoLayout->addWidget( m_labelIsoId, 0, 1 );
  isoInfoLayout->addWidget( new QLabel( i18n("System Id:"), m_isoInfoWidget ), 0, 0 );
  isoInfoLayout->addWidget( m_labelIsoSystemId, 0, 1 );
  isoInfoLayout->addWidget( new QLabel( i18n("Volume Id:"), m_isoInfoWidget ), 1, 0 );
  isoInfoLayout->addWidget( m_labelIsoVolumeId, 1, 1 );
  isoInfoLayout->addWidget( new QLabel( i18n("Volume Set Id:"), m_isoInfoWidget ), 2, 0 );
  isoInfoLayout->addWidget( m_labelIsoVolumeSetId, 2, 1 );
  isoInfoLayout->addWidget( new QLabel( i18n("Publisher Id:"), m_isoInfoWidget ), 0, 3 );
  isoInfoLayout->addWidget( m_labelIsoPublisherId, 0, 4 );
  isoInfoLayout->addWidget( new QLabel( i18n("Preparer Id:"), m_isoInfoWidget ), 1, 3 );
  isoInfoLayout->addWidget( m_labelIsoPreparerId, 1, 4 );
  isoInfoLayout->addWidget( new QLabel( i18n("Application Id:"), m_isoInfoWidget ), 2, 3 );
  isoInfoLayout->addWidget( m_labelIsoApplicationId, 2, 4 );

  QFrame* isoInfoSpacerLine = new QFrame( m_isoInfoWidget );
  isoInfoSpacerLine->setFrameStyle( QFrame::HLine | QFrame::Sunken );

  isoInfoLayout->addMultiCellWidget( isoInfoSpacerLine, 5, 5, 0, 4 );

  isoInfoLayout->addColSpacing( 2, 10 );

  f = m_labelIsoApplicationId->font();
  f.setBold( true );
  //  m_labelIsoId->setFont( f );
  m_labelIsoSystemId->setFont( f );
  m_labelIsoVolumeId->setFont( f );
  m_labelIsoVolumeSetId->setFont( f );
  m_labelIsoPublisherId->setFont( f );
  m_labelIsoPreparerId->setFont( f );
  m_labelIsoApplicationId->setFont( f );


  m_generalInfoLabel = new KCutLabel( groupImage );
  m_generalInfoLabel->setFont( f );

  QFrame* imageSpacerLine = new QFrame( groupImage );
  imageSpacerLine->setFrameStyle( QFrame::HLine | QFrame::Sunken );

  groupImageLayout->addWidget( m_editImagePath, 0, 0 );
  groupImageLayout->addWidget( m_buttonFindImageFile, 0, 1 );
  groupImageLayout->addWidget( new QLabel( i18n("Size:"), groupImage ), 0, 2 );
  groupImageLayout->addWidget( m_labelImageSize, 0, 3 );
  groupImageLayout->addMultiCellWidget( imageSpacerLine, 1, 1, 0, 3 );
  groupImageLayout->addMultiCellWidget( m_isoInfoWidget, 2, 2, 0, 3 );
  groupImageLayout->addMultiCellWidget( m_generalInfoLabel, 3, 3, 0, 3 );

  m_md5ProgressWidget = new KProgress( 100, groupImage );
  m_md5ProgressWidget->setFormat( i18n("Calculating...") );
  m_md5Label = new KActiveLabel( groupImage );

  QHBoxLayout* infoBox = new QHBoxLayout;
  infoBox->setMargin(0);
  infoBox->setSpacing( spacingHint() );
  infoBox->addWidget( new QLabel( i18n("MD5 sum:"), groupImage ) );
  infoBox->addWidget( m_md5ProgressWidget );
  infoBox->addWidget( m_md5Label );

  groupImageLayout->addMultiCellLayout( infoBox, 4, 4, 0, 3 );

  m_md5ProgressWidget->hide();
  // -----------------------------------------------------------------------


  // options
  // -----------------------------------------------------------------------
  QTabWidget* optionTabbed = new QTabWidget( frame );

  QWidget* optionTab = new QWidget( optionTabbed );
  QGridLayout* groupOptionsLayout = new QGridLayout( optionTab );
  groupOptionsLayout->setAlignment( Qt::AlignTop );
  groupOptionsLayout->setSpacing( spacingHint() );
  groupOptionsLayout->setMargin( marginHint() );

  m_checkBurnProof = K3bStdGuiItems::burnproofCheckbox( optionTab );
  m_checkDummy = K3bStdGuiItems::simulateCheckbox( optionTab );
  m_checkDao = K3bStdGuiItems::daoCheckbox( optionTab );

  groupOptionsLayout->addWidget( m_checkDummy, 0, 0 );
  groupOptionsLayout->addWidget( m_checkDao, 1, 0 );
  groupOptionsLayout->addWidget( m_checkBurnProof, 2, 0 );

  QWidget* advancedTab = new QWidget( optionTabbed );
  QGridLayout* advancedTabLayout = new QGridLayout( advancedTab );
  advancedTabLayout->setAlignment( Qt::AlignTop );
  advancedTabLayout->setSpacing( spacingHint() );
  advancedTabLayout->setMargin( marginHint() );

  m_checkNoFix = K3bStdGuiItems::startMultisessionCheckBox( advancedTab );

  advancedTabLayout->addWidget( m_checkNoFix, 0, 0 );


  optionTabbed->addTab( optionTab, i18n("Options") );
  optionTabbed->addTab( advancedTab, i18n("Advanced") );
  // -----------------------------------------------------------------------



  QGridLayout* grid = new QGridLayout( frame );
  grid->setSpacing( spacingHint() );
  grid->setMargin( 0 );

  grid->addWidget( m_writerSelectionWidget, 0, 0 );
  grid->addWidget( groupImage, 1, 0 );
  grid->addWidget( optionTabbed, 2, 0 );

  grid->setRowStretch( 2, 1 );

  connect( m_editImagePath, SIGNAL(textChanged(const QString&)), this, SLOT(updateImageSize(const QString&)) );
  connect( m_buttonFindImageFile, SIGNAL(clicked()), this, SLOT(slotFindImageFile()) );

  m_checkDao->setChecked( true );
  m_checkDummy->setChecked( false );

  slotWriterChanged();
}


void K3bIsoImageWritingDialog::slotUser1()
{
  // check if the image exists
  if( !QFile::exists( m_editImagePath->text() ) ) {
    KMessageBox::error( this, i18n("Could not find file %1").arg(m_editImagePath->text()) );
    return;
  }

  m_md5Job->cancel();

  // save the path
  kapp->config()->setGroup("General Options");
  kapp->config()->writeEntry( "last written image", m_editImagePath->text() );

  // create the job
  if( m_job == 0 )
    m_job = new K3bIso9660ImageWritingJob();

  m_job->setBurnDevice( m_writerSelectionWidget->writerDevice() );
  m_job->setSpeed( m_writerSelectionWidget->writerSpeed() );
  m_job->setBurnproof( m_checkBurnProof->isChecked() );
  m_job->setSimulate( m_checkDummy->isChecked() );
  m_job->setDao( m_checkDao->isChecked() );
  m_job->setNoFix( m_checkNoFix->isChecked() );

  m_job->setImagePath( m_editImagePath->text() );

  m_job->setWritingApp( m_writerSelectionWidget->writingApp() );

  // create a progresswidget
  K3bBurnProgressDialog d( kapp->mainWidget(), "burnProgress", false );

  d.setJob( m_job );
  hide();

  m_job->start();
  d.exec();

  slotClose();
}


void K3bIsoImageWritingDialog::slotUser2()
{
  slotClose();
}


void K3bIsoImageWritingDialog::updateImageSize( const QString& path )
{
  m_bIsoImage = false;


  QFileInfo info( path );
  if( info.isFile() ) {

    if( m_lastCheckedFile != path ) {
      // recalculate the md5sum
      m_md5Job->setFile( path );
      m_md5Job->start();
      m_md5Label->hide();
      m_md5ProgressWidget->show();
    }
    else
      slotMd5JobFinished(true);

    m_lastCheckedFile = path;

    // do not show the size here since path could be a cue file
    long imageSize = info.size();


    // ------------------------------------------------
    // Test for iso9660 image
    // ------------------------------------------------
    char buf[17*2048];

    QFile f( path );
    if( f.open( IO_Raw | IO_ReadOnly ) ) {
      if( f.readBlock( buf, 17*2048 ) == 17*2048 ) {
	// check if this is an ios9660-image
	// the beginning of the 16th sector needs to have the following format:

	// first byte: 1
	// second to 11th byte: 67, 68, 48, 48, 49, 1 (CD001)
	// 12th byte: 0

	m_bIsoImage = ( buf[16*2048] == 1 &&
			buf[16*2048+1] == 67 &&
			buf[16*2048+2] == 68 &&
			buf[16*2048+3] == 48 &&
			buf[16*2048+4] == 48 &&
			buf[16*2048+5] == 49 &&
			buf[16*2048+6] == 1 &&
			buf[16*2048+7] == 0 );
      }

      f.close();
    }


    // ------------------------------------------------
    // Show file size
    // ------------------------------------------------
    m_labelImageSize->setText( KIO::convertSize( imageSize ) );


    if( m_bIsoImage ) {
      QString str = QString::fromLocal8Bit( &buf[16*2048+1], 5 ).stripWhiteSpace();
//       m_labelIsoId->setText( str.isEmpty() ? QString("-") : str );
      str = QString::fromLocal8Bit( &buf[16*2048+8], 32 ).stripWhiteSpace();
      m_labelIsoSystemId->setText( str.isEmpty() ? QString("-") : str );
      str = QString::fromLocal8Bit( &buf[16*2048+40], 32 ).stripWhiteSpace();
      m_labelIsoVolumeId->setText( str.isEmpty() ? QString("-") : str );
      str = QString::fromLocal8Bit( &buf[16*2048+190], 128 ).stripWhiteSpace();
      m_labelIsoVolumeSetId->setText( str.isEmpty() ? QString("-") : str );
      str = QString::fromLocal8Bit( &buf[16*2048+318], 128 ).stripWhiteSpace();
      m_labelIsoPublisherId->setText( str.isEmpty() ? QString("-") : str );
      str = QString::fromLocal8Bit( &buf[16*2048+446], 128 ).stripWhiteSpace();
      m_labelIsoPreparerId->setText( str.isEmpty() ? QString("-") : str );
      str = QString::fromLocal8Bit( &buf[16*2048+574], 128 ).stripWhiteSpace();
      m_labelIsoApplicationId->setText( str.isEmpty() ? QString("-") : str );

      m_isoInfoWidget->show();

      m_generalInfoLabel->setText( i18n("Seems to be an ISO9660 image") );
    }
    else {
      m_isoInfoWidget->hide();
      m_generalInfoLabel->setText( i18n("Seems not to be a usable image file (or raw image)") );
    }

    // enable the Write-Button
    actionButton( User1 )->setEnabled( true );
  }
  else {
    m_isoInfoWidget->hide();
    m_labelImageSize->setText( "0 kb" );
    m_generalInfoLabel->setText( i18n("No file") );
    m_md5Label->setText( "" );

    // Disable the Write-Button
    actionButton( User1 )->setDisabled( true );
  }
}


void K3bIsoImageWritingDialog::slotFindImageFile()
{
  QString newPath( KFileDialog::getOpenFileName( m_editImagePath->text(), QString::null, this, i18n("Choose ISO image file or cue/bin combination") ) );
  if( !newPath.isEmpty() )
    m_editImagePath->setText( newPath );
}


void K3bIsoImageWritingDialog::slotWriterChanged()
{
  if (m_writerSelectionWidget->writerDevice())
    if( !m_writerSelectionWidget->writerDevice()->burnproof() ) {
      m_checkBurnProof->setChecked( false );
      m_checkBurnProof->setDisabled( true );
    }
    else {
      m_checkBurnProof->setEnabled( true );
    }
}


void K3bIsoImageWritingDialog::setImage( const KURL& url )
{
  m_editImagePath->setText( url.path() );
}


void K3bIsoImageWritingDialog::slotMd5JobFinished( bool success )
{
  if( success ) {
    m_md5Label->setText( m_md5Job->hexDigest() );
  }
  else {
    m_md5Label->setText( "" );
    m_lastCheckedFile = "";
  }

  m_md5Label->show();
  m_md5ProgressWidget->hide();
}

#include "k3bisoimagewritingdialog.moc"
