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
#include "../device/k3bdevicemanager.h"
#include "../device/k3bdevice.h"
#include "../k3bwriterselectionwidget.h"
#include "../k3bburnprogressdialog.h"
#include "k3bisoimagejob.h"
#include "../kcutlabel.h"

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

  connect( m_checkUseCueFile, SIGNAL(toggled(bool)), this, SLOT(slotCueBinChecked(bool)) );

  m_job = 0;

  m_checkBurnProof->setChecked( true ); // enabled by default

  slotWriterChanged();

  kapp->config()->setGroup("General Options");
  m_editImagePath->setText( kapp->config()->readEntry( "last written image", "" ) );
  updateImageSize( m_editImagePath->text() );



  // ToolTips
  // --------------------------------------------------------------------------------
  QToolTip::add( m_checkDummy, i18n("Only simulate the writing process") );
  QToolTip::add( m_checkDao, i18n("Write in disk at once mode") );
  QToolTip::add( m_checkBurnProof, i18n("Enable BURN-PROOF to avoid buffer underruns") );
  QToolTip::add( m_checkUseCueFile, i18n("") );
  QToolTip::add( m_checkNoFix, i18n("") );

  // What's This info
  // --------------------------------------------------------------------------------
  QWhatsThis::add( m_checkDummy, i18n("<p>If this option is checked K3b will perform all writing steps with the "
				      "laser turned off."
				      "<p>This is useful for example to test a higher writing speed "
				      " or if your system is able to write on-the-fly.") );
  QWhatsThis::add( m_checkDao, i18n("<p>If this option is checked K3b will write the cd in disk at once mode as "
				    "compared to track at once (TAO)."
				    "<p>It is always recommended to use DAO where possible."
				    "<p><b>Caution:</b> Only in DAO mode track pregaps other than 2 seconds are "
				    "supported.") );
  QWhatsThis::add( m_checkBurnProof, i18n("<p>If this option is checked K3b enables <em>BURN-PROOF</em>. This is "
					  "a feature of the cd writer which avoids buffer underruns.") );
  QWhatsThis::add( m_checkUseCueFile, i18n("") );
  QWhatsThis::add( m_checkNoFix, i18n("") );
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

  m_labelIsoId = new KCutLabel( m_isoInfoWidget );
  m_labelIsoSystemId = new KCutLabel( m_isoInfoWidget );
  m_labelIsoVolumeId = new KCutLabel( m_isoInfoWidget );
  m_labelIsoVolumeSetId = new KCutLabel( m_isoInfoWidget );
  m_labelIsoPublisherId = new KCutLabel( m_isoInfoWidget );
  m_labelIsoPreparerId = new KCutLabel( m_isoInfoWidget );
  m_labelIsoApplicationId = new KCutLabel( m_isoInfoWidget );

  isoInfoLayout->addWidget( new QLabel( i18n("Id:"), m_isoInfoWidget ), 0, 0 );
  isoInfoLayout->addWidget( m_labelIsoId, 0, 1 );
  isoInfoLayout->addWidget( new QLabel( i18n("System Id:"), m_isoInfoWidget ), 1, 0 );
  isoInfoLayout->addWidget( m_labelIsoSystemId, 1, 1 );
  isoInfoLayout->addWidget( new QLabel( i18n("Volume Id:"), m_isoInfoWidget ), 2, 0 );
  isoInfoLayout->addWidget( m_labelIsoVolumeId, 2, 1 );
  isoInfoLayout->addWidget( new QLabel( i18n("Volume Set Id:"), m_isoInfoWidget ), 3, 0 );
  isoInfoLayout->addWidget( m_labelIsoVolumeSetId, 3, 1 );
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
  m_labelIsoId->setFont( f );
  m_labelIsoSystemId->setFont( f );
  m_labelIsoVolumeId->setFont( f );
  m_labelIsoVolumeSetId->setFont( f );
  m_labelIsoPublisherId->setFont( f );
  m_labelIsoPreparerId->setFont( f );
  m_labelIsoApplicationId->setFont( f );


  m_generalInfoLabel = new QLabel( groupImage );
  m_generalInfoLabel->setAlignment( int( QLabel::WordBreak | QLabel::AlignVCenter | QLabel::AlignLeft ) );
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
  // -----------------------------------------------------------------------


  // options
  // -----------------------------------------------------------------------
  QTabWidget* optionTabbed = new QTabWidget( frame );

  QWidget* optionTab = new QWidget( optionTabbed );
  QGridLayout* groupOptionsLayout = new QGridLayout( optionTab );
  groupOptionsLayout->setAlignment( Qt::AlignTop );
  groupOptionsLayout->setSpacing( spacingHint() );
  groupOptionsLayout->setMargin( marginHint() );

  m_checkBurnProof = new QCheckBox( i18n("Burnproof"), optionTab );
  m_checkDummy = new QCheckBox( i18n("Simulate"), optionTab );
  m_checkDao = new QCheckBox( i18n("Disc at once"), optionTab );

  groupOptionsLayout->addWidget( m_checkDummy, 0, 0 );
  groupOptionsLayout->addWidget( m_checkDao, 1, 0 );
  groupOptionsLayout->addWidget( m_checkBurnProof, 2, 0 );

  QWidget* advancedTab = new QWidget( optionTabbed );
  QGridLayout* advancedTabLayout = new QGridLayout( advancedTab );
  advancedTabLayout->setAlignment( Qt::AlignTop );
  advancedTabLayout->setSpacing( spacingHint() );
  advancedTabLayout->setMargin( marginHint() );

  //  m_checkRawWrite = new QCheckBox( i18n("Raw writing"), advancedTab );
  m_checkUseCueFile = new QCheckBox( i18n("Use cue-file"), advancedTab );
  m_checkNoFix = new QCheckBox( i18n("Do not close session"), advancedTab );

  advancedTabLayout->addWidget( m_checkNoFix, 0, 0 );
  //  advancedTabLayout->addWidget( m_checkRawWrite, 1, 0 );
  advancedTabLayout->addWidget( m_checkUseCueFile, 1, 0 );


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

  // save the path
  kapp->config()->setGroup("General Options");
  kapp->config()->writeEntry( "last written image", m_editImagePath->text() );

//   if( m_bIsoImage && m_checkRawWrite->isChecked() && !m_checkUseCueFile->isChecked() )
//     if( KMessageBox::warningContinueCancel( this, i18n("Writing an Iso9660 image in raw mode will lead to an unusable disk. "
// 						       "Are you sure you want to continue?") )
// 	== KMessageBox::Cancel )
//       return;
					  

  // create the job
  if( m_job == 0 )
    m_job = new K3bIsoImageJob();

  m_job->setWriter( m_writerSelectionWidget->writerDevice() );
  m_job->setSpeed( m_writerSelectionWidget->writerSpeed() );
  m_job->setBurnproof( m_checkBurnProof->isChecked() );
  m_job->setDummy( m_checkDummy->isChecked() );
  m_job->setDao( m_checkDao->isChecked() );
  //  m_job->setRawWrite( m_checkRawWrite->isChecked() );
  m_job->setNoFix( m_checkNoFix->isChecked() );

  if( m_checkUseCueFile->isChecked() ) {
    m_job->setImagePath( m_cuePath );
    m_job->setWriteCueBin( true );
  }
  else {
    m_job->setImagePath( m_editImagePath->text() );
    m_job->setWriteCueBin( false );
  }

  // create a progresswidget
  K3bBurnProgressDialog d( this, "burnProgress", false );

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
  m_bCueBinAvailable = false;


  QFileInfo info( path );
  if( info.isFile() ) {

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
    // Test for cue/bin
    // ------------------------------------------------

    // TODO: check if cue-file has only one FILE-statement (otherwise say: unusabe cue-file: xxx.cue)

    if( info.extension( false ) == "bin" || info.extension( false ) == "cue" ) {
      // search the corresponding file
      QString basename = path.left( path.length() - 3 );

      // cdrdao does not use the FILE statement but the basename + the bin extension
      bool binAvail = QFile::exists( basename + "bin" );
      bool cueAvail = QFile::exists( basename + "cue" );

      if( binAvail && cueAvail ) {
	// TODO: check if the cue-file has only one FILE-statement as needed for cdrdao
	m_bCueBinAvailable = true;
	m_cuePath = basename + "cue";
	m_binPath = basename + "bin";

	imageSize = QFileInfo( m_binPath ).size();
      }
    }

    // ------------------------------------------------
    // Test for cdrecord-clone toc
    // ------------------------------------------------

    // TODO: check if xxx and xxx.toc could be found and if so enable clone option (only if cdrecord-clone is available)



    // ------------------------------------------------
    // Show file size
    // ------------------------------------------------
    m_labelImageSize->setText( KIO::convertSize( imageSize ) );

    
    if( m_bIsoImage ) {
      QString str = QString::fromLatin1( &buf[16*2048+1], 5 ).stripWhiteSpace();
      m_labelIsoId->setText( str.isEmpty() ? QString("-") : str );
      str = QString::fromLatin1( &buf[16*2048+8], 32 ).stripWhiteSpace();
      m_labelIsoSystemId->setText( str.isEmpty() ? QString("-") : str );
      str = QString::fromLatin1( &buf[16*2048+40], 32 ).stripWhiteSpace();
      m_labelIsoVolumeId->setText( str.isEmpty() ? QString("-") : str );
      str = QString::fromLatin1( &buf[16*2048+190], 128 ).stripWhiteSpace();
      m_labelIsoVolumeSetId->setText( str.isEmpty() ? QString("-") : str );
      str = QString::fromLatin1( &buf[16*2048+318], 128 ).stripWhiteSpace();
      m_labelIsoPublisherId->setText( str.isEmpty() ? QString("-") : str );
      str = QString::fromLatin1( &buf[16*2048+446], 128 ).stripWhiteSpace();
      m_labelIsoPreparerId->setText( str.isEmpty() ? QString("-") : str );
      str = QString::fromLatin1( &buf[16*2048+574], 128 ).stripWhiteSpace();
      m_labelIsoApplicationId->setText( str.isEmpty() ? QString("-") : str );
      
      m_isoInfoWidget->show();
      
      m_generalInfoLabel->setText( i18n("Seems to be an iso9660 image") );
      m_checkUseCueFile->setChecked( false );
      //      m_checkRawWrite->setChecked( false );
    }
    else {
      m_isoInfoWidget->hide();
      if( m_bCueBinAvailable ) {
	m_generalInfoLabel->setText( i18n("Usable cue/bin combination found:\n%1\n%2").arg(m_cuePath).arg(m_binPath) );
	m_checkUseCueFile->setChecked( true );
      }
      else {
	m_generalInfoLabel->setText( i18n("Seems not to be a usable image file (or raw image)") );
      }
    }

    if( m_bCueBinAvailable ) {
      m_checkUseCueFile->setEnabled( true );
    }
    else {
      m_checkUseCueFile->setChecked( false );
      m_checkUseCueFile->setEnabled( false );
    }
  
    // enable the Write-Button
    actionButton( User1 )->setEnabled( true );
  }
  else {
    m_isoInfoWidget->hide();
    m_labelImageSize->setText( "0 kb" );
    m_generalInfoLabel->setText( i18n("No file") );
    
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
  if( !m_writerSelectionWidget->writerDevice()->burnproof() ) {
    m_checkBurnProof->setChecked( false );
    m_checkBurnProof->setDisabled( true );
  }
  else {
    m_checkBurnProof->setEnabled( true );
  }
}


void K3bIsoImageWritingDialog::slotCueBinChecked( bool c )
{
  // raw writing is set in the cue-file so there is no use in allowing to set it when using one
  // cdrdao has no option to not fixate a cd (DAO)
  if( c ) {
    //    m_checkRawWrite->setChecked( false );
    m_checkNoFix->setChecked( false );
  }
  //  m_checkRawWrite->setDisabled( c );
  m_checkNoFix->setDisabled( c );
}


#include "k3bisoimagewritingdialog.moc"
