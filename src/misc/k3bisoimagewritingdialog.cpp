/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
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
#include <k3bmd5job.h>
#include <k3bglobals.h>
#include <k3bwritingmodewidget.h>
#include <k3bcore.h>
#include <k3blistview.h>
#include <k3biso9660.h>
#include <k3bapplication.h>
#include <k3bmediacache.h>

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
#include <kinputdialog.h>
#include <kurldrag.h>
#include <klineedit.h>

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
#include <qevent.h>
#include <qpopupmenu.h>
#include <qclipboard.h>


class K3bIsoImageWritingDialog::Private
{
public:
  Private()
    : md5SumItem(0),
      haveMd5Sum( false ),
      imageForced( false ) {
  }

  K3bListViewItem* md5SumItem;
  bool haveMd5Sum;
  QString lastCheckedFile;
  bool isIsoImage;

  bool imageForced;
};


K3bIsoImageWritingDialog::K3bIsoImageWritingDialog( QWidget* parent, const char* name, bool modal )
  : K3bInteractionDialog( parent, name,
			  i18n("Burn Iso9660 Image"),
			  i18n("to DVD"),
			  START_BUTTON|CANCEL_BUTTON,
			  START_BUTTON,
			  "DVD image writing",
			  modal )
{
  d = new Private();

  setAcceptDrops(true);
  setupGui();

  m_writerSelectionWidget->setWantedMediumType( K3bDevice::MEDIA_WRITABLE_DVD );
  m_writerSelectionWidget->setWantedMediumState( K3bDevice::STATE_EMPTY );
  m_writerSelectionWidget->setSupportedWritingApps( K3b::GROWISOFS );
  m_writingModeWidget->setSupportedModes( K3b::DAO|K3b::WRITING_MODE_INCR_SEQ|K3b::WRITING_MODE_RES_OVWR );

  m_md5Job = new K3bMd5Job( 0, this );
  connect( m_md5Job, SIGNAL(finished(bool)),
	   this, SLOT(slotMd5JobFinished(bool)) );
  connect( m_md5Job, SIGNAL(percent(int)),
	   this, SLOT(slotMd5JobPercent(int)) );

  updateImageSize( imagePath() );

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
}


void K3bIsoImageWritingDialog::init()
{
  if( !d->imageForced ) {
    // when opening the dialog first the default settings are loaded and afterwards we set the
    // last written image because that's what most users want
    KConfig* c = k3bcore->config();
    c->setGroup( configGroup() );
    QString image = c->readPathEntry( "last written image" );
    if( QFile::exists( image ) )
      m_editImagePath->setURL( image );
  }
}


void K3bIsoImageWritingDialog::setupGui()
{
  QWidget* frame = mainWidget();

  // image
  // -----------------------------------------------------------------------
  QGroupBox* groupImageUrl = new QGroupBox( 1, Qt::Horizontal, i18n("Image to Burn"), frame );
  m_editImagePath = new KURLRequester( groupImageUrl );
  m_editImagePath->setMode( KFile::File|KFile::ExistingOnly );
  m_editImagePath->setCaption( i18n("Choose Image File") );
  m_editImagePath->setFilter( i18n("*.iso *.ISO|ISO9660 Image Files") + "\n"
			      + i18n("*|All Files") );

  connect( m_editImagePath->lineEdit(), SIGNAL( textChanged ( const QString & ) ), this,  SLOT( slotWriterChanged() ) );

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

  connect( m_infoView, SIGNAL(contextMenu(KListView*, QListViewItem*, const QPoint&)),
	   this, SLOT(slotContextMenu(KListView*, QListViewItem*, const QPoint&)) );

  m_writerSelectionWidget = new K3bWriterSelectionWidget( frame );

  // options
  // -----------------------------------------------------------------------
  QTabWidget* optionTabbed = new QTabWidget( frame );

  QWidget* optionTab = new QWidget( optionTabbed );
  QGridLayout* optionTabLayout = new QGridLayout( optionTab );
  optionTabLayout->setAlignment( Qt::AlignTop );
  optionTabLayout->setSpacing( spacingHint() );
  optionTabLayout->setMargin( marginHint() );

  QGroupBox* writingModeGroup = new QGroupBox( 1, Vertical, i18n("Writing Mode"), optionTab );
  writingModeGroup->setInsideMargin( marginHint() );
  m_writingModeWidget = new K3bWritingModeWidget( writingModeGroup );


  // copies --------
  QGroupBox* groupCopies = new QGroupBox( 2, Qt::Horizontal, i18n("Copies"), optionTab );
  groupCopies->setInsideSpacing( spacingHint() );
  groupCopies->setInsideMargin( marginHint() );
  QLabel* pixLabel = new QLabel( groupCopies );
  pixLabel->setPixmap( SmallIcon( "cdcopy", KIcon::SizeMedium ) );
  pixLabel->setScaledContents( false );
  m_spinCopies = new QSpinBox( groupCopies );
  m_spinCopies->setMinValue( 1 );
  m_spinCopies->setMaxValue( 999 );
  // -------- copies

  QGroupBox* optionGroup = new QGroupBox( 3, Vertical, i18n("Settings"), optionTab );
  optionGroup->setInsideMargin( marginHint() );
  optionGroup->setInsideSpacing( spacingHint() );
  m_checkDummy = K3bStdGuiItems::simulateCheckbox( optionGroup );
  m_checkVerify = K3bStdGuiItems::verifyCheckBox( optionGroup );


  optionTabLayout->addWidget( writingModeGroup, 0, 0 );
  optionTabLayout->addWidget( groupCopies, 1, 0 );
  optionTabLayout->addMultiCellWidget( optionGroup, 0, 1, 1, 1 );
  optionTabLayout->setRowStretch( 1, 1 );
  optionTabLayout->setColStretch( 1, 1 );

  optionTabbed->addTab( optionTab, i18n("Settings") );


  QGridLayout* grid = new QGridLayout( frame );
  grid->setSpacing( spacingHint() );
  grid->setMargin( 0 );

  grid->addWidget( groupImageUrl, 0, 0 );
  grid->addWidget( m_infoView, 1, 0 );
  grid->addWidget( m_writerSelectionWidget, 2, 0 );
  grid->addWidget( optionTabbed, 3, 0 );

  grid->setRowStretch( 1, 1 );
}


void K3bIsoImageWritingDialog::slotStartClicked()
{
  if( !d->isIsoImage ) {
    if( KMessageBox::warningContinueCancel( this,
				   i18n("The image you selected is not a valid ISO9660 image. "
					"Are you sure you want to burn it anyway? "
					"(There may exist other valid image types that are not detected by K3b but "
					"will work fine.)"), i18n("Burn") ) == KMessageBox::Cancel )
      return;
  }

  K3bIso9660 isoFs( imagePath() );
  if( isoFs.open() ) {
    if( K3b::imageFilesize( KURL::fromPathOrURL( imagePath() ) ) < (KIO::filesize_t)(isoFs.primaryDescriptor().volumeSpaceSize*2048) ) {
      if( KMessageBox::questionYesNo( this, 
				      i18n("<p>This image has an invalid file size."
					   "If it has been downloaded make sure the download is complete."
					   "<p>Only continue if you know what you are doing."),
				      i18n("Warning"),
				      i18n("Continue"),
				      i18n("Cancel") ) == KMessageBox::No )
	return;
    }
  }

  m_md5Job->cancel();

  // save the path
  KConfig* c = k3bcore->config();
  c->setGroup( configGroup() );
  if( c->readPathEntry( "last written image" ).isEmpty() )
    c->writePathEntry( "last written image", imagePath() );

  // create a progresswidget
  K3bBurnProgressDialog dlg( kapp->mainWidget(), "burnProgress", true );

  // create the job
  K3bIso9660ImageWritingJob* job = new K3bIso9660ImageWritingJob( &dlg );

  job->setBurnDevice( m_writerSelectionWidget->writerDevice() );
  job->setSpeed( m_writerSelectionWidget->writerSpeed() );
  job->setSimulate( m_checkDummy->isChecked() );
  job->setWritingMode( m_writingModeWidget->writingMode() );
  job->setVerifyData( m_checkVerify->isChecked() );
  job->setCopies( m_checkDummy->isChecked() ? 1 : m_spinCopies->value() );
  job->setImagePath( imagePath() );

  // HACK (needed since if the medium is forced the stupid K3bIso9660ImageWritingJob defaults to cd writing)
  job->setWritingApp( K3b::GROWISOFS );

  if( !exitLoopOnHide() )
    hide();

  dlg.startJob( job );

  delete job;

  if( KConfigGroup( k3bcore->config(), "General Options" ).readBoolEntry( "keep action dialogs open", false ) &&
      !exitLoopOnHide() )
    show();
  else
    close();
}


void K3bIsoImageWritingDialog::updateImageSize( const QString& path )
{
  m_md5Job->cancel();
  m_infoView->clear();
  d->md5SumItem = 0;
  d->haveMd5Sum = false;
  d->isIsoImage = false;

  QFileInfo info( path );
  if( info.isFile() ) {

    KIO::filesize_t imageSize = K3b::filesize( KURL::fromPathOrURL(path) );

    // ------------------------------------------------
    // Test for iso9660 image
    // ------------------------------------------------
    K3bIso9660 isoF( path );
    if( isoF.open() ) {

      d->isIsoImage = true;

      K3bListViewItem* isoRootItem = new K3bListViewItem( m_infoView, m_infoView->lastItem(),
							  i18n("Iso9660 image") );
      isoRootItem->setForegroundColor( 0, palette().disabled().foreground() );
      isoRootItem->setPixmap( 0, SmallIcon( "cdimage") );

      K3bListViewItem* item = new K3bListViewItem( isoRootItem, m_infoView->lastItem(),
						   i18n("Filesize:"), KIO::convertSize( imageSize ) );
      item->setForegroundColor( 0, palette().disabled().foreground() );

      item = new K3bListViewItem( isoRootItem,
				  m_infoView->lastItem(),
				  i18n("System Id:"),
				  isoF.primaryDescriptor().systemId.isEmpty()
				  ? QString("-")
				  : isoF.primaryDescriptor().systemId );
      item->setForegroundColor( 0, palette().disabled().foreground() );

      item = new K3bListViewItem( isoRootItem,
				  m_infoView->lastItem(),
				  i18n("Volume Id:"),
				  isoF.primaryDescriptor().volumeId.isEmpty()
				  ? QString("-")
				  : isoF.primaryDescriptor().volumeId );
      item->setForegroundColor( 0, palette().disabled().foreground() );

      item = new K3bListViewItem( isoRootItem,
				  m_infoView->lastItem(),
				  i18n("Volume Set Id:"),
				  isoF.primaryDescriptor().volumeSetId.isEmpty()
				  ? QString("-")
				  : isoF.primaryDescriptor().volumeSetId );
      item->setForegroundColor( 0, palette().disabled().foreground() );

      item = new K3bListViewItem( isoRootItem,
				  m_infoView->lastItem(),
				  i18n("Publisher Id:"),
				  isoF.primaryDescriptor().publisherId.isEmpty()
				  ? QString("-")
				  : isoF.primaryDescriptor().publisherId );
      item->setForegroundColor( 0, palette().disabled().foreground() );

      item = new K3bListViewItem( isoRootItem,
				  m_infoView->lastItem(),
				  i18n("Preparer Id:"),
				  isoF.primaryDescriptor().preparerId.isEmpty()
				  ? QString("-") : isoF.primaryDescriptor().preparerId );
      item->setForegroundColor( 0, palette().disabled().foreground() );

      item = new K3bListViewItem( isoRootItem,
				  m_infoView->lastItem(),
				  i18n("Application Id:"),
				  isoF.primaryDescriptor().applicationId.isEmpty()
				  ? QString("-")
				  : isoF.primaryDescriptor().applicationId );
      item->setForegroundColor( 0, palette().disabled().foreground() );

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
  K3bDevice::Device* dev = m_writerSelectionWidget->writerDevice();
  if( dev ) {
    K3bMedium medium = k3bappcore->mediaCache()->medium( dev );
    if( medium.diskInfo().mediaType() & K3bDevice::MEDIA_DVD_PLUS_ALL ) {
      // no simulation support for DVD+R(W) media
      m_checkDummy->setChecked(false);
      m_checkDummy->setEnabled(false);
    }
    else {
      m_checkDummy->setDisabled( false );
    }

    m_writingModeWidget->determineSupportedModesFromMedium( dev );

    if( m_checkDummy->isChecked() ) {
      m_checkVerify->setEnabled( false );
      m_checkVerify->setChecked( false );
    }
    else
      m_checkVerify->setEnabled( true );

    m_spinCopies->setEnabled( !m_checkDummy->isChecked() );
  }

  setButtonEnabled( START_BUTTON, 
		    dev && !m_editImagePath->lineEdit()->text().isEmpty() );
}


void K3bIsoImageWritingDialog::setImage( const KURL& url )
{
  d->imageForced = true;
#if KDE_IS_VERSION(3,4,0)
  m_editImagePath->setKURL( url );
#else
  m_editImagePath->setURL( url.path() );
#endif
}


void K3bIsoImageWritingDialog::calculateMd5Sum( const QString& file )
{
  d->haveMd5Sum = false;

  if( !d->md5SumItem )
    d->md5SumItem = new K3bListViewItem( m_infoView, m_infoView->firstChild() );

  d->md5SumItem->setText( 0, i18n("Md5 Sum:") );
  d->md5SumItem->setForegroundColor( 0, palette().disabled().foreground() );
  d->md5SumItem->setProgress( 1, 0 );
  d->md5SumItem->setPixmap( 0, SmallIcon( "exec") );

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
    d->md5SumItem->setText( 1, m_md5Job->hexDigest() );
    d->haveMd5Sum = true;
  }
  else {
    d->md5SumItem->setForegroundColor( 1, Qt::red );
    if( m_md5Job->hasBeenCanceled() )
      d->md5SumItem->setText( 1, i18n("Calculation cancelled") );
    else
      d->md5SumItem->setText( 1, i18n("Calculation failed") );
    d->md5SumItem->setPixmap( 0, SmallIcon( "stop") );
    d->lastCheckedFile.truncate(0);
  }

  d->md5SumItem->setDisplayProgressBar( 1, false );
}


void K3bIsoImageWritingDialog::slotContextMenu( KListView*, QListViewItem*, const QPoint& pos )
{
  if( !d->haveMd5Sum )
    return;

  QPopupMenu popup;
  int copyItem = popup.insertItem( i18n("Copy checksum to clipboard") );
  int compareItem = popup.insertItem( i18n("Compare checksum...") );

  int r = popup.exec( pos );

  if( r == compareItem ) {
    bool ok;
    QString md5sumToCompare = KInputDialog::getText( i18n("MD5 Sum Check"),
						     i18n("Please insert the MD5 Sum to compare:"),
						     QString::null,
						     &ok,
						     this );
    if( ok ) {
      if( md5sumToCompare.lower().utf8() == m_md5Job->hexDigest().lower() )
	KMessageBox::information( this, i18n("The MD5 Sum of %1 equals the specified.").arg(imagePath()),
				  i18n("MD5 Sums Equal") );
      else
	KMessageBox::sorry( this, i18n("The MD5 Sum of %1 differs from the specified.").arg(imagePath()),
			    i18n("MD5 Sums Differ") );
    }
  }
  else if( r == copyItem ) {
    QApplication::clipboard()->setText( m_md5Job->hexDigest().lower(), QClipboard::Clipboard );
  }
}


void K3bIsoImageWritingDialog::loadUserDefaults( KConfigBase* c )
{
  m_writingModeWidget->loadConfig( c );
  m_checkDummy->setChecked( c->readBoolEntry("simulate", false ) );
  m_checkVerify->setChecked( c->readBoolEntry( "verify_data", false ) );
  m_spinCopies->setValue( c->readNumEntry( "copies", 1 ) );

  m_writerSelectionWidget->loadConfig( c );

  if( !d->imageForced ) {
    QString image = c->readPathEntry( "image path", c->readPathEntry( "last written image" ) );
    if( QFile::exists( image ) )
      m_editImagePath->setURL( image );
  }
}


void K3bIsoImageWritingDialog::saveUserDefaults( KConfigBase* c )
{
  m_writingModeWidget->saveConfig( c ),
  c->writeEntry( "simulate", m_checkDummy->isChecked() );
  c->writeEntry( "verify_data", m_checkVerify->isChecked() );
  c->writeEntry( "copies", m_spinCopies->value() );

  m_writerSelectionWidget->saveConfig( c );

  c->writePathEntry( "image path", imagePath() );
}


void K3bIsoImageWritingDialog::loadK3bDefaults()
{
  m_writerSelectionWidget->loadDefaults();
  m_writingModeWidget->setWritingMode( K3b::WRITING_MODE_AUTO );
  m_checkDummy->setChecked( false );
  m_checkVerify->setChecked( false );
  m_spinCopies->setValue( 1 );
}


QString K3bIsoImageWritingDialog::imagePath() const
{
  return K3b::convertToLocalUrl( KURL::fromPathOrURL( m_editImagePath->url() ) ).path();
}


void K3bIsoImageWritingDialog::dragEnterEvent( QDragEnterEvent* e )
{
  e->accept( KURLDrag::canDecode(e) );
}


void K3bIsoImageWritingDialog::dropEvent( QDropEvent* e )
{
  KURL::List urls;
  KURLDrag::decode( e, urls );
#if KDE_IS_VERSION(3,4,0)
  m_editImagePath->setKURL( urls.first() );
#else
  m_editImagePath->setURL( urls.first().path() );
#endif
}

#include "k3bisoimagewritingdialog.moc"
