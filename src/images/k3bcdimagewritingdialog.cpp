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


#include "k3bcdimagewritingdialog.h"
#include "k3biso9660imagewritingjob.h"
#include "k3bbinimagewritingjob.h"
#include "k3bcuefileparser.h"
#include "k3bclonetocreader.h"
#include <cdcopy/k3bclonejob.h>

#include <k3bdevicemanager.h>
#include <k3bdevice.h>
#include <k3bwriterselectionwidget.h>
#include <k3bburnprogressdialog.h>
#include <kcutlabel.h>
#include <k3bstdguiitems.h>
#include <tools/k3bmd5job.h>
#include <k3bdatamodewidget.h>
#include <k3bglobals.h>
#include <k3bwritingmodewidget.h>
#include <k3bcore.h>
#include <k3blistview.h>
#include <k3biso9660.h>
#include <k3bthememanager.h>

#include <kapplication.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kurlrequester.h>
#include <kfiledialog.h>
#include <kstdguiitem.h>
#include <kguiitem.h>
#include <kiconloader.h>
#include <kconfig.h>
#include <kio/global.h>
#include <kurl.h>
#include <klineeditdlg.h>

#include <qheader.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qptrlist.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qfont.h>
#include <qfontmetrics.h>
#include <qpushbutton.h>
#include <qtabwidget.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qspinbox.h>
#include <qmap.h>
#include <qptrqueue.h>


class K3bCdImageWritingDialog::Private
{
public:
  Private()
    : md5SumItem(0) {
  }

  K3bListViewItem* md5SumItem;
  QString lastCheckedFile;

  K3bMd5Job* md5Job;

  int foundImageType;

  QMap<int,int> imageTypeSelectionMap;
  QMap<int,int> imageTypeSelectionMapRev;
  QString imageFile;
  QString tocFile;
};


K3bCdImageWritingDialog::K3bCdImageWritingDialog( QWidget* parent, const char* name, bool modal )
  : K3bInteractionDialog( parent, name, 
			  i18n("Burn CD Image"), 
			  "iso cue toc",
			  START_BUTTON|CANCEL_BUTTON,
			  START_BUTTON,
			  modal )
{
  d = new Private();

  setupGui();

  d->md5Job = new K3bMd5Job( this );
  connect( d->md5Job, SIGNAL(finished(bool)),
	   this, SLOT(slotMd5JobFinished(bool)) );
  connect( d->md5Job, SIGNAL(percent(int)),
	   this, SLOT(slotMd5JobPercent(int)) );

  connect( m_writerSelectionWidget, SIGNAL(writerChanged()),
	   this, SLOT(slotToggleAll()) );
  connect( m_writerSelectionWidget, SIGNAL(writingAppChanged(int)),
	   this, SLOT(slotToggleAll()) );
  connect( m_comboImageType, SIGNAL(activated(int)),
	   this, SLOT(slotToggleAll()) );
  connect( m_writingModeWidget, SIGNAL(writingModeChanged(int)),
	   this, SLOT(slotToggleAll()) );
  connect( m_editImagePath, SIGNAL(textChanged(const QString&)), 
	   this, SLOT(slotUpdateImage(const QString&)) );
  connect( m_checkDummy, SIGNAL(toggled(bool)),
	   this, SLOT(slotToggleAll()) );

  slotLoadUserDefaults();
}


K3bCdImageWritingDialog::~K3bCdImageWritingDialog()
{
  kdDebug() << "(K3bCdImageWritingDialog) destrcution" << endl;
  delete d;
}


void K3bCdImageWritingDialog::setupGui()
{
  QWidget* frame = mainWidget();

  // image
  // -----------------------------------------------------------------------
  QGroupBox* groupImageUrl = new QGroupBox( 1, Qt::Horizontal, i18n("Image to Burn"), frame );
  m_editImagePath = new KURLRequester( groupImageUrl );
  m_editImagePath->setCaption( i18n("Choose Image File") );
  m_editImagePath->setFilter( i18n("*.iso *.toc *.ISO *.TOC *.cue *.CUE|Image Files") 
			      + "\n"
			      + i18n("*.iso *.ISO|ISO9660 Image Files")
			      + "\n"
			      + i18n("*.cue *.CUE|Cue Files")
			      + "\n"
			      + i18n("*.toc *.TOC|Cdrdao TOC Files and Cdrecord Clone Images")
			      + "\n" 
			      + i18n("*|All Files") );
  
  QGroupBox* groupImageType = new QGroupBox( 1, Qt::Horizontal, i18n("Image Type"), frame );
  m_comboImageType = new QComboBox( groupImageType );
  m_comboImageType->insertItem( i18n("Auto Detection") );
  m_comboImageType->insertItem( i18n("ISO9660 Image") );
  m_comboImageType->insertItem( i18n("Cue/Bin Image") );
  m_comboImageType->insertItem( i18n("Cdrdao TOC File") );
  m_comboImageType->insertItem( i18n("Cdrecord Clone Image") );
  d->imageTypeSelectionMap[1] = IMAGE_ISO;
  d->imageTypeSelectionMap[2] = IMAGE_CUE_BIN;
  d->imageTypeSelectionMap[3] = IMAGE_CDRDAO_TOC;
  d->imageTypeSelectionMap[4] = IMAGE_CDRECORD_CLONE;
  d->imageTypeSelectionMapRev[IMAGE_ISO] = 1;
  d->imageTypeSelectionMapRev[IMAGE_CUE_BIN] = 2;
  d->imageTypeSelectionMapRev[IMAGE_CDRDAO_TOC] = 3;
  d->imageTypeSelectionMapRev[IMAGE_CDRECORD_CLONE] = 4;


  // image info
  // -----------------------------------------------------------------------
  m_infoView = new K3bListView( frame );
  m_infoView->addColumn( "key" );
  m_infoView->addColumn( "value" );
  m_infoView->header()->hide();
  m_infoView->setNoItemText( i18n("No image file selected") );
  m_infoView->setSorting( -1 );
  m_infoView->setAlternateBackground( QColor() );
  m_infoView->setFullWidth(true);
  m_infoView->setSelectionMode( QListView::NoSelection );
  m_infoView->setDoubleClickForEdit( false ); // just for the md5 button

  connect( m_infoView, SIGNAL(editorButtonClicked( K3bListViewItem*, int )),
	   this, SLOT(slotMd5SumCompare()) );


  // options
  // -----------------------------------------------------------------------
  QTabWidget* optionTabbed = new QTabWidget( frame );

  QWidget* optionTab = new QWidget( optionTabbed );
  QGridLayout* optionTabLayout = new QGridLayout( optionTab );
  optionTabLayout->setAlignment( Qt::AlignTop );
  optionTabLayout->setSpacing( spacingHint() );
  optionTabLayout->setMargin( marginHint() );

  m_writerSelectionWidget = new K3bWriterSelectionWidget( false, optionTab );

  QGroupBox* writingModeGroup = new QGroupBox( 1, Vertical, i18n("Writing Mode"), optionTab );
  writingModeGroup->setInsideMargin( marginHint() );
  m_writingModeWidget = new K3bWritingModeWidget( writingModeGroup );


  // copies --------
  QGroupBox* groupCopies = new QGroupBox( 2, Qt::Horizontal, i18n("Copies"), optionTab );
  groupCopies->setInsideSpacing( spacingHint() );
  groupCopies->setInsideMargin( marginHint() );
  QLabel* pixLabel = new QLabel( groupCopies );
  pixLabel->setPixmap( k3bthememanager->currentTheme()->pixmap( "k3b_cd_copy" ) );
  pixLabel->setScaledContents( false );
  m_spinCopies = new QSpinBox( groupCopies );
  m_spinCopies->setMinValue( 1 );
  m_spinCopies->setMaxValue( 99 );
  // -------- copies

  QGroupBox* optionGroup = new QGroupBox( 3, Vertical, i18n("Options"), optionTab );
  optionGroup->setInsideMargin( marginHint() );
  optionGroup->setInsideSpacing( spacingHint() );
  m_checkDummy = K3bStdGuiItems::simulateCheckbox( optionGroup );
  m_checkVerify = K3bStdGuiItems::verifyCheckBox( optionGroup );

  optionTabLayout->addMultiCellWidget( m_writerSelectionWidget, 0, 0, 0, 1 );
  optionTabLayout->addWidget( writingModeGroup, 1, 0 );
  optionTabLayout->addWidget( groupCopies, 2, 0 );
  optionTabLayout->addMultiCellWidget( optionGroup, 1, 2, 1, 1 );
  optionTabLayout->setRowStretch( 2, 1 );
  optionTabLayout->setColStretch( 1, 1 );

  optionTabbed->addTab( optionTab, i18n("Options") );



  // advanced ---------------------------------
  QWidget* advancedTab = new QWidget( optionTabbed );
  QGridLayout* advancedTabLayout = new QGridLayout( advancedTab );
  advancedTabLayout->setAlignment( Qt::AlignTop );
  advancedTabLayout->setSpacing( spacingHint() );
  advancedTabLayout->setMargin( marginHint() );
    
  m_dataModeWidget = new K3bDataModeWidget( advancedTab );
  m_checkNoFix = K3bStdGuiItems::startMultisessionCheckBox( advancedTab );
    
  advancedTabLayout->addWidget( new QLabel( i18n("Data mode:"), advancedTab ), 0, 0 );
  advancedTabLayout->addWidget( m_dataModeWidget, 0, 1 );
  advancedTabLayout->addMultiCellWidget( m_checkNoFix, 1, 1, 0, 2 );
  advancedTabLayout->setRowStretch( 2, 1 );
  advancedTabLayout->setColStretch( 2, 1 );
    
  optionTabbed->addTab( advancedTab, i18n("Advanced") );
  // -----------------------------------------------------------------------




  QGridLayout* grid = new QGridLayout( frame );
  grid->setSpacing( spacingHint() );
  grid->setMargin( 0 );

  grid->addWidget( groupImageUrl, 0, 0 );
  grid->addWidget( groupImageType, 0, 1 );
  grid->addMultiCellWidget( m_infoView, 1, 1, 0, 1 );
  grid->addMultiCellWidget( optionTabbed, 2, 2, 0, 1 );

  grid->setRowStretch( 1, 1 );
}


void K3bCdImageWritingDialog::slotStartClicked()
{
  d->md5Job->cancel();

  // save the path
  KConfig* c = k3bcore->config();
  c->setGroup( "image writing" );
  if( c->readPathEntry( "last written image" ).isEmpty() )
    c->writePathEntry( "last written image", m_editImagePath->url() );

  if( d->imageFile.isEmpty() )
    d->imageFile = m_editImagePath->url();
  if( d->tocFile.isEmpty() )
    d->tocFile = m_editImagePath->url();

  // create the job
  K3bBurnJob* job = 0;
  switch( currentImageType() ) {
  case IMAGE_CDRECORD_CLONE:
    {
      K3bCloneJob* _job = new K3bCloneJob( this );
      _job->setWriterDevice( m_writerSelectionWidget->writerDevice() );
      _job->setImagePath( d->imageFile );
      _job->setSimulate( m_checkDummy->isChecked() );
      _job->setWriteSpeed( m_writerSelectionWidget->writerSpeed() );
      _job->setCopies( m_checkDummy->isChecked() ? 1 : m_spinCopies->value() );
      _job->setOnlyBurnExistingImage( true );
      
      job = _job;
    }
    break;

  case IMAGE_CUE_BIN:
    // for now the K3bBinImageWritingJob decides if it's a toc or a cue file
  case IMAGE_CDRDAO_TOC:
    {
      K3bBinImageWritingJob* job_ = new K3bBinImageWritingJob( this );

      job_->setWriter( m_writerSelectionWidget->writerDevice() );
      job_->setSpeed( m_writerSelectionWidget->writerSpeed() );
      job_->setTocFile( d->tocFile );
      job_->setSimulate(m_checkDummy->isChecked());
      job_->setMulti(m_checkNoFix->isChecked());
      job_->setCopies( m_checkDummy->isChecked() ? 1 : m_spinCopies->value() );
      
      job = job_;
    }
    break;

  case IMAGE_ISO:
    {
      K3bIso9660ImageWritingJob* job_ = new K3bIso9660ImageWritingJob();
      
      job_->setBurnDevice( m_writerSelectionWidget->writerDevice() );
      job_->setSpeed( m_writerSelectionWidget->writerSpeed() );
      job_->setSimulate( m_checkDummy->isChecked() );
      job_->setWritingMode( m_writingModeWidget->writingMode() );
      job_->setVerifyData( m_checkVerify->isChecked() );
      job_->setNoFix( m_checkNoFix->isChecked() );
      job_->setDataMode( m_dataModeWidget->dataMode() );
      job_->setImagePath( d->imageFile );
      job_->setCopies( m_checkDummy->isChecked() ? 1 : m_spinCopies->value() );
      
      job = job_;
    }
    break;

  default:
    kdDebug() << "(K3bCdImageWritingDialog) this should really not happen!" << endl;
    break;
  }

  if( job ) {
    job->setWritingApp( m_writerSelectionWidget->writingApp() );

    // create a progresswidget
    K3bBurnProgressDialog dlg( kapp->mainWidget(), "burnProgress", true );
    
    hide();
    
    dlg.startJob(job);
    
    show();

    delete job;
  }
}


void K3bCdImageWritingDialog::slotUpdateImage( const QString& path )
{
  // check the image types

  d->md5Job->cancel();
  m_infoView->clear();
  m_infoView->header()->resizeSection( 0, 20 );
  d->md5SumItem = 0;
  d->foundImageType = IMAGE_UNKNOWN;
  d->tocFile.truncate(0);
  d->imageFile.truncate(0);

  QFileInfo info( path );
  if( info.isFile() ) {

    // ------------------------------------------------
    // Test for iso9660 image
    // ------------------------------------------------
    K3bIso9660 isoF( path );
    if( isoF.open() ) {
      createIso9660InfoItems( &isoF );
      isoF.close();
      calculateMd5Sum( path );

      d->foundImageType = IMAGE_ISO;
      d->imageFile = path;
    }

    if( d->foundImageType == IMAGE_UNKNOWN ) {
     
      // check for cdrecord clone image
      // try both path and path.toc as tocfiles
      K3bCloneTocReader cr;

      if( path.right(4) == ".toc" ) {
	cr.openFile( path );
	if( cr.isValid() ) {
	  d->tocFile = path;
	  d->imageFile = cr.imageFilename();
	}
      }
      if( d->imageFile.isEmpty() ) {
	cr.openFile( path + ".toc" );
	if( cr.isValid() ) {
	  d->tocFile = cr.filename();
	  d->imageFile = cr.imageFilename();
	}
      }

      if( !d->imageFile.isEmpty() ) {
	// we have a cdrecord clone image
	createCdrecordCloneItems( d->tocFile, d->imageFile );
	calculateMd5Sum( d->imageFile );

	d->foundImageType = IMAGE_CDRECORD_CLONE;
      }
    }

    if( d->foundImageType == IMAGE_UNKNOWN ) {

      // check for cue/bin stuff
      // once again we try both path and path.cue
      K3bCueFileParser cp;

      if( path.right(4).lower() == ".cue" )
	cp.openFile( path );
      else if( path.right(4).lower() == ".bin" )
	cp.openFile( path.left( path.length()-3) + "cue" );

      if( cp.isValid() ) {
	d->tocFile = cp.filename();
	d->imageFile = cp.imageFilename();
      }
      
      if( d->imageFile.isEmpty() ) {
	cp.openFile( path + ".cue" );
	if( cp.isValid() ) {
	  d->tocFile = cp.filename();
	  d->imageFile = cp.imageFilename();
	}
      }

      if( !d->imageFile.isEmpty() ) {
	// we have a cdrecord clone image
	createCueBinItems( d->tocFile, d->imageFile );
	calculateMd5Sum( d->imageFile );

	d->foundImageType = IMAGE_CUE_BIN;
      }
    }

    if( d->foundImageType == IMAGE_UNKNOWN ) {
      // TODO: check for cdrdao tocfile
    }



    if( d->foundImageType == IMAGE_UNKNOWN ) {
      K3bListViewItem* item = new K3bListViewItem( m_infoView, m_infoView->lastItem(),
						   i18n("Seems not to be a usable image") );
      item->setForegroundColor( 0, Qt::red );
      item->setPixmap( 0, SmallIcon( "stop") );
    }
  }
  else {
    K3bListViewItem* item = new K3bListViewItem( m_infoView, m_infoView->lastItem(),
						 i18n("File not found") );
    item->setForegroundColor( 0, Qt::red );
    item->setPixmap( 0, SmallIcon( "stop") );
  }

  slotToggleAll();
}


void K3bCdImageWritingDialog::createIso9660InfoItems( K3bIso9660* isoF )
{
  K3bListViewItem* isoRootItem = new K3bListViewItem( m_infoView, m_infoView->lastItem(),
						      i18n("Detected:"),
						      i18n("Iso9660 image") );
  isoRootItem->setForegroundColor( 0, Qt::gray );
  isoRootItem->setPixmap( 0, SmallIcon( "cdimage") );

  K3bListViewItem* item = new K3bListViewItem( isoRootItem, m_infoView->lastItem(),
					       i18n("Filesize:"), KIO::convertSize( K3b::filesize(isoF->fileName()) ) );
  item->setForegroundColor( 0, Qt::gray );

  item = new K3bListViewItem( isoRootItem, 
			      m_infoView->lastItem(),
			      i18n("System Id:"), 
			      isoF->primaryDescriptor().systemId.isEmpty()
			      ? QString("-") 
			      : isoF->primaryDescriptor().systemId );
  item->setForegroundColor( 0, Qt::gray );

  item = new K3bListViewItem( isoRootItem, 
			      m_infoView->lastItem(),
			      i18n("Volume Id:"), 
			      isoF->primaryDescriptor().volumeId.isEmpty() 
			      ? QString("-") 
			      : isoF->primaryDescriptor().volumeId );
  item->setForegroundColor( 0, Qt::gray );

  item = new K3bListViewItem( isoRootItem, 
			      m_infoView->lastItem(),
			      i18n("Volume Set Id:"), 
			      isoF->primaryDescriptor().volumeSetId.isEmpty()
			      ? QString("-")
			      : isoF->primaryDescriptor().volumeSetId );
  item->setForegroundColor( 0, Qt::gray );

  item = new K3bListViewItem( isoRootItem, 
			      m_infoView->lastItem(),
			      i18n("Publisher Id:"), 
			      isoF->primaryDescriptor().publisherId.isEmpty() 
			      ? QString("-") 
			      : isoF->primaryDescriptor().publisherId );
  item->setForegroundColor( 0, Qt::gray );

  item = new K3bListViewItem( isoRootItem, 
			      m_infoView->lastItem(),
			      i18n("Preparer Id:"), 
			      isoF->primaryDescriptor().preparerId.isEmpty() 
			      ? QString("-") : isoF->primaryDescriptor().preparerId );
  item->setForegroundColor( 0, Qt::gray );

  item = new K3bListViewItem( isoRootItem, 
			      m_infoView->lastItem(),
			      i18n("Application Id:"), 
			      isoF->primaryDescriptor().applicationId.isEmpty()
			      ? QString("-") 
			      : isoF->primaryDescriptor().applicationId );
  item->setForegroundColor( 0, Qt::gray );

  isoRootItem->setOpen( true );
}


void K3bCdImageWritingDialog::createCdrecordCloneItems( const QString& tocFile, const QString& imageFile )
{
  K3bListViewItem* isoRootItem = new K3bListViewItem( m_infoView, m_infoView->lastItem(),
						      i18n("Detected:"),
						      i18n("Cdrecord clone image") );
  isoRootItem->setForegroundColor( 0, Qt::gray );
  isoRootItem->setPixmap( 0, SmallIcon( "cdimage") );

  K3bListViewItem* item = new K3bListViewItem( isoRootItem, m_infoView->lastItem(),
					       i18n("Filesize:"), KIO::convertSize( K3b::filesize(imageFile) ) );
  item->setForegroundColor( 0, Qt::gray );

  item = new K3bListViewItem( isoRootItem, 
			      m_infoView->lastItem(),
			      i18n("Image file:"), 
			      imageFile );
  item->setForegroundColor( 0, Qt::gray );

  item = new K3bListViewItem( isoRootItem, 
			      m_infoView->lastItem(),
			      i18n("TOC file:"), 
			      tocFile );
  item->setForegroundColor( 0, Qt::gray );

  isoRootItem->setOpen( true );
}


void K3bCdImageWritingDialog::createCueBinItems( const QString& cueFile, const QString& imageFile )
{
  K3bListViewItem* isoRootItem = new K3bListViewItem( m_infoView, m_infoView->lastItem(),
						      i18n("Detected:"),
						      i18n("Cue/bin image") );
  isoRootItem->setForegroundColor( 0, Qt::gray );
  isoRootItem->setPixmap( 0, SmallIcon( "cdimage") );

  K3bListViewItem* item = new K3bListViewItem( isoRootItem, m_infoView->lastItem(),
					       i18n("Filesize:"), KIO::convertSize( K3b::filesize(imageFile) ) );
  item->setForegroundColor( 0, Qt::gray );

  item = new K3bListViewItem( isoRootItem, 
			      m_infoView->lastItem(),
			      i18n("Image file:"), 
			      imageFile );
  item->setForegroundColor( 0, Qt::gray );

  item = new K3bListViewItem( isoRootItem, 
			      m_infoView->lastItem(),
			      i18n("Cue file:"), 
			      cueFile );
  item->setForegroundColor( 0, Qt::gray );

  isoRootItem->setOpen( true );
}


void K3bCdImageWritingDialog::slotToggleAll()
{
  kdDebug() << "(K3bCdImageWritingDialog) slotToggleAll() " << endl;

  if (m_writerSelectionWidget->writerDevice()) {

    // enable the Write-Button if we found a valid image or the user forced an image type
    m_buttonStart->setEnabled( currentImageType() != IMAGE_UNKNOWN 
			       && QFile::exists( m_editImagePath->url() ) );

    // cdrecord clone and cue both need DAO
    if( m_writerSelectionWidget->writingApp() != K3b::CDRDAO 
	&& currentImageType() == IMAGE_ISO )
      m_writingModeWidget->setSupportedModes( K3b::TAO|K3b::DAO|K3b::RAW ); // stuff supported by cdrecord
    else
      m_writingModeWidget->setSupportedModes( K3b::DAO );

    // some stuff is only available for iso images
    m_checkNoFix->setEnabled( currentImageType() == IMAGE_ISO );
    m_dataModeWidget->setEnabled( currentImageType() == IMAGE_ISO );
    if( m_checkDummy->isChecked() ) {
      m_checkVerify->setEnabled( false );
      m_checkVerify->setChecked( false );
    }
    else
      m_checkVerify->setEnabled( currentImageType() == IMAGE_ISO );
    m_spinCopies->setEnabled( !m_checkDummy->isChecked() );
  }
  else {
    m_buttonStart->setEnabled( false );
  }

  switch( currentImageType() ) {
  case IMAGE_CDRDAO_TOC:
    m_writerSelectionWidget->setSupportedWritingApps( K3b::CDRDAO );
    break;
  case IMAGE_CDRECORD_CLONE:
    m_writerSelectionWidget->setSupportedWritingApps( K3b::CDRECORD );
    break;
  default:
    m_writerSelectionWidget->setSupportedWritingApps( K3b::CDRECORD|K3b::CDRDAO );
    break;
  }

  K3bListViewItem* item = dynamic_cast<K3bListViewItem*>(m_infoView->firstChild());
  if( item )
    item->setForegroundColor( 1, 
			      currentImageType() != d->foundImageType 
			      ? Qt::red
			      : m_infoView->colorGroup().foreground() );
}


void K3bCdImageWritingDialog::setImage( const KURL& url )
{
  m_editImagePath->setURL( url.path() );
}


void K3bCdImageWritingDialog::calculateMd5Sum( const QString& file )
{
  if( !d->md5SumItem )
    d->md5SumItem = new K3bListViewItem( m_infoView, m_infoView->firstChild() );

  d->md5SumItem->setText( 0, i18n("Md5 Sum:") );
  d->md5SumItem->setForegroundColor( 0, Qt::gray );
  d->md5SumItem->setProgress( 1, 0 );
  d->md5SumItem->setPixmap( 0, SmallIcon( "exec") );
  d->md5SumItem->setButton( 1, false );

  if( file != d->lastCheckedFile ) {
    d->lastCheckedFile = file;
    d->md5Job->setFile( file );
    d->md5Job->start();
  }
  else
    slotMd5JobFinished( true );
}


void K3bCdImageWritingDialog::slotMd5JobPercent( int p )
{
  d->md5SumItem->setProgress( 1, p );
}


void K3bCdImageWritingDialog::slotMd5JobFinished( bool success )
{
  if( success ) {
    d->md5SumItem->setPixmap( 0, SmallIcon( "ok") );
    d->md5SumItem->setText( 1, d->md5Job->hexDigest() );
  }
  else {
    d->md5SumItem->setForegroundColor( 1, Qt::red );
    d->md5SumItem->setText( 1, i18n("Calculation failed") );
    d->md5SumItem->setPixmap( 0, SmallIcon( "stop") );
    d->lastCheckedFile = "";
  }

  d->md5SumItem->setButton( 1, success );
  d->md5SumItem->setDisplayProgressBar( 1, false );
}


void K3bCdImageWritingDialog::slotMd5SumCompare()
{
  bool ok;
  QString md5sumToCompare = KLineEditDlg::getText( i18n("MD5 Sum check"),
						   i18n("Please insert the MD5 Sum to compare:"),
						   QString::null,
						   &ok,
						   this );
  if( ok ) {
    if( md5sumToCompare.utf8() == d->md5Job->hexDigest() )
      KMessageBox::information( this, i18n("The MD5 Sum of %1 equals the specified.").arg(m_editImagePath->url()),
				i18n("MD5 Sums Equal") );
    else
      KMessageBox::sorry( this, i18n("The MD5 Sum of %1 differs from the specified.").arg(m_editImagePath->url()),
			  i18n("MD5 Sums Differ") );
  }
}


void K3bCdImageWritingDialog::slotLoadUserDefaults()
{
  KConfig* c = k3bcore->config();
  c->setGroup( "image writing" );

  m_writingModeWidget->loadConfig( c );
  m_checkDummy->setChecked( c->readBoolEntry("simulate", false ) );
  m_checkNoFix->setChecked( c->readBoolEntry("multisession", false ) );
  m_dataModeWidget->loadConfig(c);
 
  m_checkVerify->setChecked( c->readBoolEntry( "verify_data", false ) );

  m_writerSelectionWidget->loadConfig( c );

  m_editImagePath->setURL( c->readPathEntry( "last written image" ) );

  QString imageType = c->readEntry( "image type", "auto" );
  int x = 0;
  if( imageType == "iso9660" )
    x = d->imageTypeSelectionMapRev[IMAGE_ISO];
  else if( imageType == "cue-bin" )
    x = d->imageTypeSelectionMapRev[IMAGE_CUE_BIN];
  else if( imageType == "cdrecord-clone" )
    x = d->imageTypeSelectionMapRev[IMAGE_CDRECORD_CLONE];
  else if( imageType == "cdrdao-toc" )
    x = d->imageTypeSelectionMapRev[IMAGE_CDRDAO_TOC];

  m_comboImageType->setCurrentItem( x );

  slotToggleAll();
}


void K3bCdImageWritingDialog::slotSaveUserDefaults()
{
  KConfig* c = k3bcore->config();
  c->setGroup( "image writing" );

  m_writingModeWidget->saveConfig( c ),
  c->writeEntry( "simulate", m_checkDummy->isChecked() );
  c->writeEntry( "multisession", m_checkNoFix->isChecked() );
  m_dataModeWidget->saveConfig(c);
  
  c->writeEntry( "verify_data", m_checkVerify->isChecked() );

  m_writerSelectionWidget->saveConfig( c );

  c->writePathEntry( "last written image", m_editImagePath->url() );

  QString imageType;
  if( m_comboImageType->currentItem() == 0 )
    imageType = "auto";
  else {
    switch( d->imageTypeSelectionMap[m_comboImageType->currentItem()] ) {
    case IMAGE_ISO:
      imageType = "iso9660";
      break;
    case IMAGE_CUE_BIN:
      imageType = "cue-bin";
      break;
    case IMAGE_CDRECORD_CLONE:
      imageType = "cdrecord-clone";
      break;
    case IMAGE_CDRDAO_TOC:
      imageType = "cdrdao-toc";
      break;
    }
  }
  c->writeEntry( "image type", imageType );
}

void K3bCdImageWritingDialog::slotLoadK3bDefaults()
{
  m_writerSelectionWidget->loadDefaults();
  m_writingModeWidget->setWritingMode( K3b::WRITING_MODE_AUTO );
  m_checkDummy->setChecked( false );
  m_checkVerify->setChecked( false );
  m_checkNoFix->setChecked( false );
  m_dataModeWidget->setDataMode( K3b::DATA_MODE_AUTO );
  m_comboImageType->setCurrentItem(0);

  slotToggleAll();
}


int K3bCdImageWritingDialog::currentImageType()
{
  if( m_comboImageType->currentItem() == 0 )
    return d->foundImageType;
  else
    return d->imageTypeSelectionMap[m_comboImageType->currentItem()];    
}

#include "k3bcdimagewritingdialog.moc"
