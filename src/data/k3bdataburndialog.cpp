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
#include <k3bstdguiitems.h>

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
  prepareGui();

  QFrame* f2 = new QFrame( this );
  QFrame* f3 = new QFrame( this );
  QFrame* f4 = new QFrame( this );

  QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
  m_optionGroupLayout->addItem( spacer );

  setupVolumeInfoTab( f2 );
  setupSettingsTab( f3 );
  setupAdvancedTab( f4 );

  addPage( f2, i18n("Volume Desc") );
  addPage( f3, i18n("Settings") );
  addPage( f4, i18n("Advanced") );

  readSettings();

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
}

K3bDataBurnDialog::~K3bDataBurnDialog(){
}


void K3bDataBurnDialog::saveSettings()
{
  doc()->setDao( m_checkDao->isChecked() );
  doc()->setDummy( m_checkSimulate->isChecked() );
  doc()->setOnTheFly( m_checkOnTheFly->isChecked() );
  doc()->setBurnproof( m_checkBurnproof->isChecked() );
  ((K3bDataDoc*)doc())->setOnlyCreateImage( m_checkOnlyCreateImage->isChecked() );
  ((K3bDataDoc*)doc())->setDeleteImage( m_checkRemoveBufferFiles->isChecked() );
			
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
  m_checkSimulate->setChecked( doc()->dummy() );
  m_checkOnTheFly->setChecked( doc()->onTheFly() );
  m_checkBurnproof->setChecked( doc()->burnproof() );
  m_checkOnlyCreateImage->setChecked( ((K3bDataDoc*)doc())->onlyCreateImage() );
  m_checkRemoveBufferFiles->setChecked( ((K3bDataDoc*)doc())->deleteImage() );
	
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

  toggleAllOptions();
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


void K3bDataBurnDialog::slotOk()
{
  if( !m_checkOnTheFly->isChecked() ) {
    QFileInfo fi( m_tempDirSelectionWidget->tempPath() );
    if( fi.isDir() )
      m_tempDirSelectionWidget->setTempPath( fi.filePath() + "/image.iso" );
    
    if( QFile::exists( m_tempDirSelectionWidget->tempPath() ) ) {
      if( KMessageBox::questionYesNo( this, 
				      i18n("Do you want to overwrite %1").arg(m_tempDirSelectionWidget->tempPath()), 
				      i18n("File exists...") ) 
	  != KMessageBox::Yes )
	return;
    }
  }
    
  K3bProjectBurnDialog::slotOk();
}


void K3bDataBurnDialog::loadDefaults()
{
  m_checkSimulate->setChecked( false );
  m_checkDao->setChecked( true );
  m_checkOnTheFly->setChecked( true );
  m_checkBurnproof->setChecked( true );

  m_checkRemoveBufferFiles->setChecked( true );
  m_checkOnlyCreateImage->setChecked( false );

  m_imageSettingsWidget->load( K3bIsoOptions::defaults() );
  m_advancedImageSettingsWidget->load( K3bIsoOptions::defaults() );
  m_volumeDescWidget->load( K3bIsoOptions::defaults() );

  toggleAllOptions();
}


void K3bDataBurnDialog::loadUserDefaults()
{
  KConfig* c = kapp->config();
  c->setGroup( "default data settings" );

  m_checkSimulate->setChecked( c->readBoolEntry( "dummy_mode", false ) );
  m_checkDao->setChecked( c->readBoolEntry( "dao", true ) );
  m_checkOnTheFly->setChecked( c->readBoolEntry( "on_the_fly", true ) );
  m_checkBurnproof->setChecked( c->readBoolEntry( "burnproof", true ) );
  m_checkRemoveBufferFiles->setChecked( c->readBoolEntry( "remove_image", true ) );
  m_checkOnlyCreateImage->setChecked( c->readBoolEntry( "only_create_image", false ) );


  K3bIsoOptions o = K3bIsoOptions::load( c );
  m_imageSettingsWidget->load( o );
  m_advancedImageSettingsWidget->load( o );
  m_volumeDescWidget->load( o );

  toggleAllOptions();
}


void K3bDataBurnDialog::saveUserDefaults()
{
  KConfig* c = kapp->config();

  c->setGroup( "default data settings" );

  c->writeEntry( "dummy_mode", m_checkSimulate->isChecked() );
  c->writeEntry( "dao", m_checkDao->isChecked() );
  c->writeEntry( "on_the_fly", m_checkOnTheFly->isChecked() );
  c->writeEntry( "burnproof", m_checkBurnproof->isChecked() );
  c->writeEntry( "remove_image", m_checkRemoveBufferFiles->isChecked() );
  c->writeEntry( "only_create_image", m_checkOnlyCreateImage->isChecked() );


  K3bIsoOptions o;
  m_imageSettingsWidget->save( o );
  m_advancedImageSettingsWidget->save( o );
  m_volumeDescWidget->save( o );
  o.save( c );

  if( m_tempDirSelectionWidget->isEnabled() ) {
    m_tempDirSelectionWidget->saveConfig();
  }
}

#include "k3bdataburndialog.moc"
