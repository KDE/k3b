/***************************************************************************
                          k3bdataburndialog.cpp  -  description
                             -------------------
    begin                : Wed May 16 2001
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

#include "k3bdataburndialog.h"
#include "k3bdatadoc.h"
#include "../k3b.h"
#include "../device/k3bdevice.h"
#include "../k3bwriterselectionwidget.h"
#include "../k3btempdirselectionwidget.h"
#include "../k3bisooptions.h"
#include "../k3bjob.h"
#include "k3bdataimagesettingswidget.h"
#include "k3bdataadvancedimagesettingswidget.h"
#include "k3bdatavolumedescwidget.h"

#include <qcheckbox.h>
#include <qframe.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qtoolbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qpoint.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qfileinfo.h>
#include <qtabwidget.h>
#include <qvalidator.h>
#include <qregexp.h>

#include <kmessagebox.h>
#include <klineedit.h>
#include <klocale.h>
#include <kconfig.h>
#include <kstandarddirs.h>
#include <kfiledialog.h>
#include <kcombobox.h>



static const char * mkisofsCharacterSets[] = { "cp10081",
					       "cp10079",
					       "cp10029",
					       "cp10007",
					       "cp10006",
					       "cp10000",
					       "koi8-r",
					       "cp874",
					       "cp869",
					       "cp866",
					       "cp865",
					       "cp864",
					       "cp863",
					       "cp862",
					       "cp861",
					       "cp860",
					       "cp857",
					       "cp855",
					       "cp852",
					       "cp850",
					       "cp775",
					       "cp737",
					       "cp437",
					       "iso8859-15",
					       "iso8859-14",
					       "iso8859-9",
					       "iso8859-8",
					       "iso8859-7",
					       "iso8859-6",
					       "iso8859-5",
					       "iso8859-4",
					       "iso8859-3",
					       "iso8859-2",
					       "iso8859-1",
					       0 };  // terminating zero



K3bDataBurnDialog::K3bDataBurnDialog(K3bDataDoc* _doc, QWidget *parent, const char *name, bool modal )
  : K3bProjectBurnDialog( _doc, parent, name, modal )
{
  QTabWidget* tab = new QTabWidget( k3bMainWidget() );
  QFrame* f1 = new QFrame( tab );
  QFrame* f2 = new QFrame( tab );
  QFrame* f3 = new QFrame( tab );
  QFrame* f4 = new QFrame( tab );

  setupBurnTab( f1 );
  setupVolumeInfoTab( f2 );
  setupSettingsTab( f3 );
  setupAdvancedTab( f4 );

//   connect( m_imageSettingsWidget->m_checkDiscardAllLinks, SIGNAL(toggled(bool)), 
// 	   m_advancedImageSettingsWidget->m_checkFollowSymbolicLinks, SLOT(setDisabled(bool)) );
	
  tab->addTab( f1, i18n("Burning") );
  tab->addTab( f2, i18n("Volume Desc") );
  tab->addTab( f3, i18n("Settings") );
  tab->addTab( f4, i18n("Advanced") );

  readSettings();

  if( K3bDevice* dev = m_writerSelectionWidget->writerDevice() )
    m_checkBurnProof->setEnabled( dev->burnproof() );

  QFileInfo fi( m_tempDirSelectionWidget->tempPath() );
  QString path;
  if( fi.isFile() )
    path = fi.dirPath();
  else
    path = fi.filePath();
  if( path[path.length()-1] != '/' )
    path.append("/");
  path.append( _doc->isoOptions().volumeID() + ".iso" );
  m_tempDirSelectionWidget->setTempPath( path );

  m_tempDirSelectionWidget->setNeededSize( doc()->size() );

  slotWriterChanged();
}

K3bDataBurnDialog::~K3bDataBurnDialog(){
}


void K3bDataBurnDialog::saveSettings()
{
  doc()->setDao( m_checkDao->isChecked() );
  doc()->setDummy( m_checkDummy->isChecked() );
  doc()->setOnTheFly( m_checkOnTheFly->isChecked() );
  doc()->setBurnproof( m_checkBurnProof->isChecked() );
  ((K3bDataDoc*)doc())->setOnlyCreateImage( m_checkOnlyCreateImage->isChecked() );
  ((K3bDataDoc*)doc())->setDeleteImage( m_checkDeleteImage->isChecked() );
			
  // -- saving current speed --------------------------------------
  doc()->setSpeed( m_writerSelectionWidget->writerSpeed() );
	
  // -- saving current device --------------------------------------
  doc()->setBurner( m_writerSelectionWidget->writerDevice() );


  // save iso image settings
  m_imageSettingsWidget->save( ((K3bDataDoc*)doc())->isoOptions() );
  m_advancedImageSettingsWidget->save( ((K3bDataDoc*)doc())->isoOptions() );
  m_volumeDescWidget->save( ((K3bDataDoc*)doc())->isoOptions() );
	

  // save image file path
  ((K3bDataDoc*)doc())->setIsoImage( m_tempDirSelectionWidget->tempPath() );  

  // save multisession settings
  if( m_groupMultiSession->selected() == m_radioMultiSessionStart )
    ((K3bDataDoc*)doc())->setMultiSessionMode( K3bDataDoc::START );
  else if( m_groupMultiSession->selected() == m_radioMultiSessionContinue )
    ((K3bDataDoc*)doc())->setMultiSessionMode( K3bDataDoc::CONTINUE );
  else if( m_groupMultiSession->selected() == m_radioMultiSessionFinish )
    ((K3bDataDoc*)doc())->setMultiSessionMode( K3bDataDoc::FINISH );
  else
    ((K3bDataDoc*)doc())->setMultiSessionMode( K3bDataDoc::NONE );
}


void K3bDataBurnDialog::readSettings()
{
  m_checkDao->setChecked( doc()->dao() );
  m_checkDummy->setChecked( doc()->dummy() );
  m_checkOnTheFly->setChecked( doc()->onTheFly() );
  m_checkBurnProof->setChecked( doc()->burnproof() );
  m_checkOnlyCreateImage->setChecked( ((K3bDataDoc*)doc())->onlyCreateImage() );
  m_checkDeleteImage->setChecked( ((K3bDataDoc*)doc())->deleteImage() );
	
  // read multisession 
  switch( ((K3bDataDoc*)doc())->multiSessionMode() ) {
  case K3bDataDoc::START:
    m_radioMultiSessionStart->setChecked(true);
    break;
  case K3bDataDoc::CONTINUE:
    m_radioMultiSessionContinue->setChecked(true);
    break;
  case K3bDataDoc::FINISH:
    m_radioMultiSessionFinish->setChecked(true);
    break;
  default:
    m_radioMultiSessionNone->setChecked(true);
    break;
  }

  if( !((K3bDataDoc*)doc())->isoImage().isEmpty() )
    m_tempDirSelectionWidget->setTempPath( ((K3bDataDoc*)doc())->isoImage() );


  m_imageSettingsWidget->load( ((K3bDataDoc*)doc())->isoOptions() );
  m_advancedImageSettingsWidget->load( ((K3bDataDoc*)doc())->isoOptions() );
  m_volumeDescWidget->load( ((K3bDataDoc*)doc())->isoOptions() );

  K3bProjectBurnDialog::readSettings();

  slotWriterChanged();
}


void K3bDataBurnDialog::setupBurnTab( QFrame* frame )
{
  QGridLayout* frameLayout = new QGridLayout( frame );
  frameLayout->setSpacing( spacingHint() );
  frameLayout->setMargin( marginHint() );

  m_writerSelectionWidget = new K3bWriterSelectionWidget( frame );
  m_tempDirSelectionWidget = new K3bTempDirSelectionWidget( frame );
  m_tempDirSelectionWidget->setSelectionMode( K3bTempDirSelectionWidget::FILE );

  frameLayout->addMultiCellWidget( m_writerSelectionWidget, 0, 0, 0, 1 );
  frameLayout->addWidget( m_tempDirSelectionWidget, 1, 1 );

  m_groupOptions = new QGroupBox( frame, "m_groupOptions" );
  m_groupOptions->setTitle( i18n( "Options" ) );
  m_groupOptions->setColumnLayout(0, Qt::Vertical );
  m_groupOptions->layout()->setSpacing( 0 );
  m_groupOptions->layout()->setMargin( 0 );
  QVBoxLayout* m_groupOptionsLayout = new QVBoxLayout( m_groupOptions->layout() );
  m_groupOptionsLayout->setAlignment( Qt::AlignTop );
  m_groupOptionsLayout->setSpacing( spacingHint() );
  m_groupOptionsLayout->setMargin( marginHint() );

  m_checkDummy = new QCheckBox( m_groupOptions, "m_checkDummy" );
  m_checkDummy->setText( i18n( "Simulate writing" ) );
  m_groupOptionsLayout->addWidget( m_checkDummy );

  m_checkOnTheFly = new QCheckBox( m_groupOptions, "m_checkOnTheFly" );
  m_checkOnTheFly->setText( i18n( "Writing on the fly" ) );
  m_groupOptionsLayout->addWidget( m_checkOnTheFly );

  m_checkOnlyCreateImage = new QCheckBox( m_groupOptions, "m_checkOnlyCreateImage" );
  m_checkOnlyCreateImage->setText( i18n( "Only create image" ) );
  m_groupOptionsLayout->addWidget( m_checkOnlyCreateImage );

  m_checkDeleteImage = new QCheckBox( m_groupOptions, "m_checkDeleteImage" );
  m_checkDeleteImage->setText( i18n( "Delete image" ) );
  m_groupOptionsLayout->addWidget( m_checkDeleteImage );

  m_checkDao = new QCheckBox( m_groupOptions, "m_checkDao" );
  m_checkDao->setText( i18n( "Disk at once" ) );
  m_groupOptionsLayout->addWidget( m_checkDao );

  m_checkBurnProof = new QCheckBox( m_groupOptions, "m_checkBurnProof" );
  m_checkBurnProof->setText( i18n( "Use BURN-PROOF" ) );
  m_groupOptionsLayout->addWidget( m_checkBurnProof );

  frameLayout->addWidget( m_groupOptions, 1, 0 );

  // we do not need a tempdir or image settings when writing on-the-fly
  connect( m_checkOnTheFly, SIGNAL(toggled(bool)), m_checkDeleteImage, SLOT(setDisabled(bool)) );
  connect( m_checkOnTheFly, SIGNAL(toggled(bool)), m_tempDirSelectionWidget, SLOT(setDisabled(bool)) );
  connect( m_checkOnTheFly, SIGNAL(toggled(bool)), m_checkOnlyCreateImage, SLOT(setDisabled(bool)) );

  // we do not need writer settings when only creating the image
  connect( m_checkOnlyCreateImage, SIGNAL(toggled(bool)), m_writerSelectionWidget, SLOT(setDisabled(bool)) );
  connect( m_checkOnlyCreateImage, SIGNAL(toggled(bool)), m_checkOnTheFly, SLOT(setDisabled(bool)) );
  connect( m_checkOnlyCreateImage, SIGNAL(toggled(bool)), m_checkBurnProof, SLOT(setDisabled(bool)) );
  connect( m_checkOnlyCreateImage, SIGNAL(toggled(bool)), m_checkDao, SLOT(setDisabled(bool)) );
  connect( m_checkOnlyCreateImage, SIGNAL(toggled(bool)), m_checkDummy, SLOT(setDisabled(bool)) );
  connect( m_checkOnlyCreateImage, SIGNAL(toggled(bool)), this, SLOT(slotOnlyCreateImageToggled(bool)) );

  frameLayout->setRowStretch( 1, 1 );
  frameLayout->setColStretch( 1, 1 );


  connect( m_writerSelectionWidget, SIGNAL(writerChanged()), this, SLOT(slotWriterChanged()) );


  // ToolTips
  // -------------------------------------------------------------------------
  QToolTip::add( m_checkDummy, i18n("Only simulate the writing process") );
  QToolTip::add( m_checkOnTheFly, i18n("Write files directly to CD without creating an image") );
  QToolTip::add( m_checkOnlyCreateImage, i18n("Only create an ISO9660 image") );
  QToolTip::add( m_checkDeleteImage, i18n("Remove images from harddisk when finished") );
  QToolTip::add( m_checkDao, i18n("Write in disk at once mode") );
  QToolTip::add( m_checkBurnProof, i18n("Enable BURN-PROOF to avoid buffer underruns") );


  // What's This info
  // -------------------------------------------------------------------------
  QWhatsThis::add( m_checkDummy, i18n("<p>If this option is checked K3b will perform all writing steps with the "
				      "laser turned off."
				      "<p>This is useful, for example, to test a higher writing speed "
				      "or if your system is able to write on-the-fly.") );
  QWhatsThis::add( m_checkOnTheFly, i18n("<p>If this option is checked, K3b will not create an image first but write "
					 "the files directly to the CD."
					 "<p><b>Caution:</b> Although this should work on most systems, make sure "
					 "the data is sent to the writer fast enough.")
					 + i18n("<p>It is recommended to try a simulation first.") );
  QWhatsThis::add( m_checkOnlyCreateImage, i18n("<p>If this option is checked, K3b will only create an ISO9660 "
						"image and not do any actual writing."
						"<p>The image can later be written to a CD with most current CD writing "
						"programs (including K3b of course).") );
  QWhatsThis::add( m_checkDeleteImage, i18n("<p>If this option is checked, K3b will remove any created images after the "
					    "writing has finished."
					    "<p>Uncheck this if you want to keep the images.") );
  QWhatsThis::add( m_checkDao, i18n("<p>If this option is checked, K3b will write the CD in 'disk at once' mode as "
				    "compared to 'track at once' (TAO)."
				    "<p>It is always recommended to use DAO where possible."
				    "<p><b>Caution:</b> Track pregaps with a length other than 2 seconds are only supported "
				    "in DAO mode.") );
  QWhatsThis::add( m_checkBurnProof, i18n("<p>If this option is checked, K3b enables <em>BURN-PROOF</em>. This is "
					  "a feature of the CD writer which avoids buffer underruns.") );
}


void K3bDataBurnDialog::setupAdvancedTab( QFrame* frame )
{
  QGridLayout* frameLayout = new QGridLayout( frame );
  frameLayout->setSpacing( spacingHint() );
  frameLayout->setMargin( marginHint() );

  m_advancedImageSettingsWidget = new K3bDataAdvancedImageSettingsWidget( frame );
  m_advancedImageSettingsWidget->m_comboInputCharset->setValidator( new QRegExpValidator( QRegExp("[\\w_-]*"), this ) );

  frameLayout->addWidget( m_advancedImageSettingsWidget, 0, 0 );

  // fill charset combo
  for( int i = 0; mkisofsCharacterSets[i]; i++ ) {
    m_advancedImageSettingsWidget->m_comboInputCharset->insertItem( QString( mkisofsCharacterSets[i] ) );
  }
}


void K3bDataBurnDialog::setupVolumeInfoTab( QFrame* frame )
{
  QGridLayout* frameLayout = new QGridLayout( frame );
  frameLayout->setSpacing( spacingHint() );
  frameLayout->setMargin( marginHint() );

  m_volumeDescWidget = new K3bDataVolumeDescWidget( frame );

  frameLayout->addWidget( m_volumeDescWidget, 0, 0 );
}


void K3bDataBurnDialog::setupSettingsTab( QFrame* frame )
{
  QGridLayout* frameLayout = new QGridLayout( frame );
  frameLayout->setSpacing( spacingHint() );
  frameLayout->setMargin( marginHint() );

  m_imageSettingsWidget = new K3bDataImageSettingsWidget( frame );

  // Multisession
  // ////////////////////////////////////////////////////////////////////////
  m_groupMultiSession = new QButtonGroup( 0, Qt::Vertical, i18n("Multi-Session"), frame );
  m_groupMultiSession->layout()->setSpacing( 0 );
  m_groupMultiSession->layout()->setMargin( 0 );
  QGridLayout* m_groupMultiSessionLayout = new QGridLayout( m_groupMultiSession->layout() );
  m_groupMultiSessionLayout->setAlignment( Qt::AlignTop );
  m_groupMultiSessionLayout->setSpacing( spacingHint() );
  m_groupMultiSessionLayout->setMargin( marginHint() );

  m_radioMultiSessionNone = new QRadioButton( i18n("&No multi-session"), m_groupMultiSession );
  m_radioMultiSessionStart = new QRadioButton( i18n("&Start multi-session"), m_groupMultiSession );
  m_radioMultiSessionContinue = new QRadioButton( i18n("&Continue multi-session"), m_groupMultiSession );
  m_radioMultiSessionFinish = new QRadioButton( i18n("&Finish multi-session"), m_groupMultiSession );

  m_groupMultiSessionLayout->addWidget( m_radioMultiSessionNone, 0, 0 );
  m_groupMultiSessionLayout->addWidget( m_radioMultiSessionStart, 1, 0 );
  m_groupMultiSessionLayout->addWidget( m_radioMultiSessionContinue, 0, 1 );
  m_groupMultiSessionLayout->addWidget( m_radioMultiSessionFinish, 1, 1 );


  frameLayout->addWidget( m_imageSettingsWidget, 0, 0 );
  frameLayout->addWidget( m_groupMultiSession, 1, 0 );


  frameLayout->setRowStretch( 1, 1 );

  // ToolTips
  // -------------------------------------------------------------------------


  // What's This info
  // -------------------------------------------------------------------------
}


void K3bDataBurnDialog::slotWriterChanged()
{
  if( K3bDevice* dev = m_writerSelectionWidget->writerDevice() ) {
    m_checkBurnProof->setEnabled( dev->burnproof() );
    m_checkDao->setEnabled( dev->dao() );
    if( !dev->burnproof() )
      m_checkBurnProof->setChecked(false);
    if( !dev->dao() )
      m_checkDao->setChecked(false);
  }
}


void K3bDataBurnDialog::slotOk()
{
  // check if enough space in tempdir if not on-the-fly
  if( !m_checkOnTheFly->isChecked() && doc()->size()/1024 > m_tempDirSelectionWidget->freeTempSpace() ) {
    KMessageBox::sorry( this, i18n("Not enough space in temporary directory. Either change the directory or select on-the-fly burning.") );
    return;
  }
  else if( !m_checkOnTheFly->isChecked() ) {
    QFileInfo fi( m_tempDirSelectionWidget->tempPath() );
    if( fi.isDir() )
      m_tempDirSelectionWidget->setTempPath( fi.filePath() + "/image.iso" );

    if( QFile::exists( m_tempDirSelectionWidget->tempPath() ) ) {
      if( KMessageBox::questionYesNo( this, i18n("Do you want to overwrite %1").arg(m_tempDirSelectionWidget->tempPath()), i18n("File exists...") ) 
	  != KMessageBox::Yes )
	return;
    }
  }
    
  K3bProjectBurnDialog::slotOk();
}


void K3bDataBurnDialog::slotOnlyCreateImageToggled( bool on )
{
  m_checkDeleteImage->setChecked( !on );
}


void K3bDataBurnDialog::loadDefaults()
{
  m_checkDummy->setChecked( false );
  m_checkDao->setChecked( true );
  m_checkOnTheFly->setChecked( true );
  m_checkBurnProof->setChecked( true );

  m_checkDeleteImage->setChecked( true );
  m_checkOnlyCreateImage->setChecked( false );

  m_imageSettingsWidget->load( K3bIsoOptions::defaults() );
  m_advancedImageSettingsWidget->load( K3bIsoOptions::defaults() );
  m_volumeDescWidget->load( K3bIsoOptions::defaults() );

  slotWriterChanged();
}


void K3bDataBurnDialog::loadUserDefaults()
{
  KConfig* c = kapp->config();
  c->setGroup( "default data settings" );

  m_checkDummy->setChecked( c->readBoolEntry( "dummy_mode", false ) );
  m_checkDao->setChecked( c->readBoolEntry( "dao", true ) );
  m_checkOnTheFly->setChecked( c->readBoolEntry( "on_the_fly", true ) );
  m_checkBurnProof->setChecked( c->readBoolEntry( "burnproof", true ) );
  m_checkDeleteImage->setChecked( c->readBoolEntry( "remove_image", true ) );
  m_checkOnlyCreateImage->setChecked( c->readBoolEntry( "only_create_image", false ) );


  K3bIsoOptions o = K3bIsoOptions::load( c );
  m_imageSettingsWidget->load( o );
  m_advancedImageSettingsWidget->load( o );
  m_volumeDescWidget->load( o );

  slotWriterChanged();
}


void K3bDataBurnDialog::saveUserDefaults()
{
  KConfig* c = kapp->config();

  c->setGroup( "default data settings" );

  c->writeEntry( "dummy_mode", m_checkDummy->isChecked() );
  c->writeEntry( "dao", m_checkDao->isChecked() );
  c->writeEntry( "on_the_fly", m_checkOnTheFly->isChecked() );
  c->writeEntry( "burnproof", m_checkBurnProof->isChecked() );
  c->writeEntry( "remove_image", m_checkDeleteImage->isChecked() );
  c->writeEntry( "only_create_image", m_checkOnlyCreateImage->isChecked() );


  K3bIsoOptions o;
  m_imageSettingsWidget->save( o );
  m_advancedImageSettingsWidget->save( o );
  m_volumeDescWidget->save( o );
  o.save( c );


  if( m_tempDirSelectionWidget->isEnabled() ) {
    kapp->config()->setGroup( "General Options" );
    QFileInfo fi( m_tempDirSelectionWidget->tempPath() );
    QString path;
    if( fi.isFile() )
      path = fi.dirPath();
    else
      path = fi.filePath();

    kapp->config()->writeEntry( "Temp Dir", path );
  }
}


void K3bDataBurnDialog::prepareJob( K3bBurnJob* job )
{
  job->setWritingApp( m_writerSelectionWidget->writingApp() );
}

#include "k3bdataburndialog.moc"
