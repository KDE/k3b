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


#include "k3bisoimagewritingdialog.h"
#include "k3biso9660imagewritingjob.h"

#include <k3bdevicemanager.h>
#include <k3bdevice.h>
#include <k3bwriterselectionwidget.h>
#include <k3bburnprogressdialog.h>
#include <kcutlabel.h>
#include <k3bstdguiitems.h>
#include <tools/k3bmd5job.h>
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
#include <qfile.h>
#include <qfileinfo.h>
#include <qpushbutton.h>
#include <qtabwidget.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qspinbox.h>


class K3bIsoImageWritingDialog::Private
{
public:
  Private()
    : md5SumItem(0) {
  }

  K3bListViewItem* md5SumItem;
  QString lastCheckedFile;
  bool isIsoImage;
};


K3bIsoImageWritingDialog::K3bIsoImageWritingDialog( QWidget* parent, const char* name, bool modal )
  : K3bInteractionDialog( parent, name, 
			  i18n("Burn Iso9660 Image"), 
			  i18n("to DVD"),
			  START_BUTTON|CANCEL_BUTTON,
			  START_BUTTON,
			  modal )
{
  d = new Private();

  setupGui();

  m_writerSelectionWidget->setSupportedWritingApps( K3b::GROWISOFS );
  m_writingModeWidget->setSupportedModes( K3b::DAO|K3b::WRITING_MODE_INCR_SEQ|K3b::WRITING_MODE_RES_OVWR );

  m_job = 0;
  m_md5Job = new K3bMd5Job( this );
  connect( m_md5Job, SIGNAL(finished(bool)),
	   this, SLOT(slotMd5JobFinished(bool)) );
  connect( m_md5Job, SIGNAL(percent(int)),
	   this, SLOT(slotMd5JobPercent(int)) );


  slotLoadUserDefaults();

  updateImageSize( m_editImagePath->url() );

  connect( m_writerSelectionWidget, SIGNAL(writerChanged()),
	   this, SLOT(slotWriterChanged()) );
  connect( m_writerSelectionWidget, SIGNAL(writingAppChanged(int)),
	   this, SLOT(slotWriterChanged()) );
  connect( m_writingModeWidget, SIGNAL(writingModeChanged(int)),
	   this, SLOT(slotWriterChanged()) );
  connect( m_editImagePath, SIGNAL(textChanged(const QString&)), 
	   this, SLOT(updateImageSize(const QString&)) );
  connect( m_checkDummy, SIGNAL(toggled(bool)),
	   this, SLOT(slotWriterChanged()) );
}


K3bIsoImageWritingDialog::~K3bIsoImageWritingDialog()
{
  delete d;
  delete m_job;
}


void K3bIsoImageWritingDialog::setupGui()
{
  QWidget* frame = mainWidget();

  // image
  // -----------------------------------------------------------------------
  QGroupBox* groupImageUrl = new QGroupBox( 1, Qt::Horizontal, i18n("Image to Burn"), frame );
  m_editImagePath = new KURLRequester( groupImageUrl );
  m_editImagePath->setCaption( i18n("Choose Image File") );
  m_editImagePath->setFilter( i18n("*.iso *.ISO|ISO9660 Image Files") + "\n"
			      + i18n("*|All Files") );

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

  m_writerSelectionWidget = new K3bWriterSelectionWidget( true, optionTab );

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


  QGridLayout* grid = new QGridLayout( frame );
  grid->setSpacing( spacingHint() );
  grid->setMargin( 0 );

  grid->addWidget( groupImageUrl, 0, 0 );
  grid->addWidget( m_infoView, 1, 0 );
  grid->addWidget( optionTabbed, 2, 0 );

  grid->setRowStretch( 1, 1 );
}


void K3bIsoImageWritingDialog::slotStartClicked()
{
  m_md5Job->cancel();

  // save the path
  KConfig* c = k3bcore->config();
  c->setGroup( "DVD image writing" );
  if( c->readPathEntry( "last written image" ).isEmpty() )
    c->writePathEntry( "last written image", m_editImagePath->url() );

  // create the job
  if( m_job == 0 )
    m_job = new K3bIso9660ImageWritingJob();

  m_job->setBurnDevice( m_writerSelectionWidget->writerDevice() );
  m_job->setSpeed( m_writerSelectionWidget->writerSpeed() );
  m_job->setSimulate( m_checkDummy->isChecked() );
  m_job->setWritingMode( m_writingModeWidget->writingMode() );
  m_job->setVerifyData( m_checkVerify->isChecked() );
  m_job->setCopies( m_checkDummy->isChecked() ? 1 : m_spinCopies->value() );
  m_job->setImagePath( m_editImagePath->url() );

  // HACK (needed since if the medium is forced the stupid K3bIso9660ImageWritingJob defaults to cd writing)
  m_job->setWritingApp( K3b::GROWISOFS );

  // create a progresswidget
  K3bBurnProgressDialog dlg( kapp->mainWidget(), "burnProgress", true );

  hide();

  dlg.startJob(m_job);

  show();
}


void K3bIsoImageWritingDialog::updateImageSize( const QString& path )
{
  m_md5Job->cancel();
  m_infoView->clear();
  d->md5SumItem = 0;

  d->isIsoImage = false;

  QFileInfo info( path );
  if( info.isFile() ) {

    KIO::filesize_t imageSize = K3b::filesize( path );

    // ------------------------------------------------
    // Test for iso9660 image
    // ------------------------------------------------
    K3bIso9660 isoF( path );
    if( isoF.open() ) {

      d->isIsoImage = true;

      K3bListViewItem* isoRootItem = new K3bListViewItem( m_infoView, m_infoView->lastItem(),
							  i18n("Iso9660 image") );
      isoRootItem->setForegroundColor( 0, Qt::gray );
      isoRootItem->setPixmap( 0, SmallIcon( "cdimage") );

      K3bListViewItem* item = new K3bListViewItem( isoRootItem, m_infoView->lastItem(),
						   i18n("Filesize:"), KIO::convertSize( imageSize ) );
      item->setForegroundColor( 0, Qt::gray );

      item = new K3bListViewItem( isoRootItem, 
				  m_infoView->lastItem(),
				  i18n("System Id:"), 
				  isoF.primaryDescriptor().systemId.isEmpty()
				  ? QString("-") 
				  : isoF.primaryDescriptor().systemId );
      item->setForegroundColor( 0, Qt::gray );

      item = new K3bListViewItem( isoRootItem, 
				  m_infoView->lastItem(),
				  i18n("Volume Id:"), 
				  isoF.primaryDescriptor().volumeId.isEmpty() 
				  ? QString("-") 
				  : isoF.primaryDescriptor().volumeId );
      item->setForegroundColor( 0, Qt::gray );

      item = new K3bListViewItem( isoRootItem, 
				  m_infoView->lastItem(),
				  i18n("Volume Set Id:"), 
				  isoF.primaryDescriptor().volumeSetId.isEmpty()
				  ? QString("-")
				  : isoF.primaryDescriptor().volumeSetId );
      item->setForegroundColor( 0, Qt::gray );

      item = new K3bListViewItem( isoRootItem, 
				  m_infoView->lastItem(),
				  i18n("Publisher Id:"), 
				  isoF.primaryDescriptor().publisherId.isEmpty() 
				  ? QString("-") 
				  : isoF.primaryDescriptor().publisherId );
      item->setForegroundColor( 0, Qt::gray );

      item = new K3bListViewItem( isoRootItem, 
				  m_infoView->lastItem(),
				  i18n("Preparer Id:"), 
				  isoF.primaryDescriptor().preparerId.isEmpty() 
				  ? QString("-") : isoF.primaryDescriptor().preparerId );
      item->setForegroundColor( 0, Qt::gray );

      item = new K3bListViewItem( isoRootItem, 
				  m_infoView->lastItem(),
				  i18n("Application Id:"), 
				  isoF.primaryDescriptor().applicationId.isEmpty()
				  ? QString("-") 
				  : isoF.primaryDescriptor().applicationId );
      item->setForegroundColor( 0, Qt::gray );

      isoRootItem->setOpen( true );

      isoF.close();
    }
    else {
      K3bListViewItem* item = new K3bListViewItem( m_infoView, m_infoView->lastItem(),
				  i18n("Not an Iso9660 image") );
      item->setForegroundColor( 0, Qt::red );
      item->setPixmap( 0, SmallIcon( "stop") );
    }

    calculateMd5Sum( path );
  }

  slotWriterChanged();
}


void K3bIsoImageWritingDialog::slotWriterChanged()
{
  if( m_writerSelectionWidget->writerDevice() ) {
    m_buttonStart->setEnabled( d->isIsoImage );

    if( m_checkDummy->isChecked() ) {
      m_checkVerify->setEnabled( false );
      m_checkVerify->setChecked( false );
    }
    else
      m_checkVerify->setEnabled( true );
    m_spinCopies->setEnabled( !m_checkDummy->isChecked() );
  }
  else {
    m_buttonStart->setEnabled( false );
  }
}


void K3bIsoImageWritingDialog::setImage( const KURL& url )
{
  m_editImagePath->setURL( url.path() );
}


void K3bIsoImageWritingDialog::calculateMd5Sum( const QString& file )
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
    m_md5Job->setFile( file );
    m_md5Job->start();
  }
  else
    slotMd5JobFinished( true );
}


void K3bIsoImageWritingDialog::slotMd5JobPercent( int p )
{
  d->md5SumItem->setProgress( 1, p );
}


void K3bIsoImageWritingDialog::slotMd5JobFinished( bool success )
{
  if( success ) {
    d->md5SumItem->setPixmap( 0, SmallIcon( "ok") );
    d->md5SumItem->setText( 1, m_md5Job->hexDigest() );
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


void K3bIsoImageWritingDialog::slotMd5SumCompare()
{
  bool ok;
  QString md5sumToCompare = KLineEditDlg::getText( i18n("MD5 Sum check"),
						   i18n("Please insert the MD5 Sum to compare:"),
						   QString::null,
						   &ok,
						   this );
  if( ok ) {
    if( md5sumToCompare.utf8() == m_md5Job->hexDigest() )
      KMessageBox::information( this, i18n("The MD5 Sum of %1 equals the specified.").arg(m_editImagePath->url()),
				i18n("MD5 Sums Equal") );
    else
      KMessageBox::sorry( this, i18n("The MD5 Sum of %1 differs from the specified.").arg(m_editImagePath->url()),
			  i18n("MD5 Sums Differ") );
  }
}


void K3bIsoImageWritingDialog::slotLoadUserDefaults()
{
  KConfig* c = k3bcore->config();
  c->setGroup( "DVD image writing" );

  m_writingModeWidget->loadConfig( c );
  m_checkDummy->setChecked( c->readBoolEntry("simulate", false ) );
  m_checkVerify->setChecked( c->readBoolEntry( "verify_data", false ) );

  m_writerSelectionWidget->loadConfig( c );

  m_editImagePath->setURL( c->readPathEntry( "last written image" ) );
}


void K3bIsoImageWritingDialog::slotSaveUserDefaults()
{
  KConfig* c = k3bcore->config();
  c->setGroup( "DVD image writing" );

  m_writingModeWidget->saveConfig( c ),
  c->writeEntry( "simulate", m_checkDummy->isChecked() );
  c->writeEntry( "verify_data", m_checkVerify->isChecked() );

  m_writerSelectionWidget->saveConfig( c );

  c->writePathEntry( "last written image", m_editImagePath->url() );
}

void K3bIsoImageWritingDialog::slotLoadK3bDefaults()
{
  m_writerSelectionWidget->loadDefaults();
  m_writingModeWidget->setWritingMode( K3b::WRITING_MODE_AUTO );
  m_checkDummy->setChecked( false );
  m_checkVerify->setChecked( false );
}

#include "k3bisoimagewritingdialog.moc"
