#include "k3bmixedburndialog.h"
#include "k3bmixeddoc.h"

#include "../data/k3bdataimagesettingswidget.h"
#include "../data/k3bdataadvancedimagesettingswidget.h"
#include "../data/k3bdatavolumedescwidget.h"
#include "../data/k3bdatadoc.h"
#include "../device/k3bdevice.h"
#include "../k3bwriterselectionwidget.h"
#include "../k3btempdirselectionwidget.h"
#include "../k3bisooptions.h"


#include <qtabwidget.h>
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
#include <qvbox.h>

#include <klocale.h>
#include <kconfig.h>
#include <kapp.h>
#include <kdebug.h>


K3bMixedBurnDialog::K3bMixedBurnDialog( K3bMixedDoc* doc, QWidget *parent, const char *name, bool modal )
  : K3bProjectBurnDialog( doc, parent, name, modal ), m_doc(doc)
{
  // create the main tab
  QTabWidget* mainTab = new QTabWidget( k3bMainWidget() );

  // create burn tab
  QWidget* burnTab = new QWidget( mainTab );
  mainTab->addTab( burnTab, i18n("Burning") );

  // create volume descriptor tab
  QVBox* box = new QVBox( mainTab );
  box->setMargin( marginHint() );
  m_volumeDescWidget = new K3bDataVolumeDescWidget( box );
  mainTab->addTab( box, i18n("Volume Desc") );

  // create image settings tab
  box = new QVBox( mainTab );
  box->setMargin( marginHint() );
  m_imageSettingsWidget = new K3bDataImageSettingsWidget( box );
  mainTab->addTab( box, i18n("Data Settings") );

  // create advanced image settings tab
  box = new QVBox( mainTab );
  box->setMargin( marginHint() );
  m_advancedImageSettingsWidget = new K3bDataAdvancedImageSettingsWidget( box );
  mainTab->addTab( box, i18n("Advanced") );
}


void K3bMixedBurnDialog::slotWriterChanged()
{
  if( K3bDevice* dev = m_writerSelectionWidget->writerDevice() )
    m_checkBurnProof->setEnabled( dev->burnproof() );
}


void K3bMixedBurnDialog::saveSettings()
{
//   m_doc->setDao( m_checkDao->isChecked() );
//   m_doc->setDummy( m_checkDummy->isChecked() );
//   m_doc->setOnTheFly( m_checkOnTheFly->isChecked() );
//   m_doc->setBurnproof( m_checkBurnProof->isChecked() );
//   m_doc->setOnlyCreateImage( m_checkOnlyCreateImage->isChecked() );
//   m_doc->setDeleteImage( m_checkDeleteImage->isChecked() );
			
  // -- saving current speed --------------------------------------
//   m_doc->setSpeed( m_writerSelectionWidget->writerSpeed() );
	
//   // -- saving current device --------------------------------------
//   m_doc->setBurner( m_writerSelectionWidget->writerDevice() );


  // save iso image settings
  m_imageSettingsWidget->save( m_doc->dataDoc()->isoOptions() );
  m_advancedImageSettingsWidget->save( m_doc->dataDoc()->isoOptions() );
  m_volumeDescWidget->save( m_doc->dataDoc()->isoOptions() );
	

  // save image file path
  //  ((K3bDataDoc*)doc())->setIsoImage( m_tempDirSelectionWidget->tempPath() );  
}


void K3bMixedBurnDialog::readSettings()
{
//   m_checkDao->setChecked( doc()->dao() );
//   m_checkDummy->setChecked( doc()->dummy() );
//   m_checkOnTheFly->setChecked( doc()->onTheFly() );
//   m_checkBurnProof->setChecked( doc()->burnproof() );
//   m_checkOnlyCreateImage->setChecked( m_doc->onlyCreateImage() );
//   m_checkDeleteImage->setChecked( m_doc->deleteImage() );
	

//   if( !((K3bDataDoc*)doc())->isoImage().isEmpty() )
//     m_tempDirSelectionWidget->setTempPath( ((K3bDataDoc*)doc())->isoImage() );

  m_imageSettingsWidget->load( m_doc->dataDoc()->isoOptions() );
  m_advancedImageSettingsWidget->load( m_doc->dataDoc()->isoOptions() );
  m_volumeDescWidget->load( m_doc->dataDoc()->isoOptions() );

  K3bProjectBurnDialog::readSettings();
}


void K3bMixedBurnDialog::slotOnlyCreateImageToggled( bool on )
{
  //  m_checkDeleteImage->setChecked( !on );
}


void K3bMixedBurnDialog::loadDefaults()
{
//   m_checkDummy->setChecked( false );
//   m_checkDao->setChecked( true );
//   m_checkOnTheFly->setChecked( true );
//   m_checkBurnProof->setChecked( true );

//   m_checkDeleteImage->setChecked( true );
//   m_checkOnlyCreateImage->setChecked( false );

  m_imageSettingsWidget->load( K3bIsoOptions::defaults() );
  m_advancedImageSettingsWidget->load( K3bIsoOptions::defaults() );
  m_volumeDescWidget->load( K3bIsoOptions::defaults() );
}


void K3bMixedBurnDialog::loadUserDefaults()
{
  KConfig* c = kapp->config();
  c->setGroup( "default mixed settings" );

//   m_checkDummy->setChecked( c->readBoolEntry( "dummy_mode", false ) );
//   m_checkDao->setChecked( c->readBoolEntry( "dao", true ) );
//   m_checkOnTheFly->setChecked( c->readBoolEntry( "on_the_fly", true ) );
//   m_checkBurnProof->setChecked( c->readBoolEntry( "burnproof", true ) );
//   m_checkDeleteImage->setChecked( c->readBoolEntry( "remove_image", true ) );
//   m_checkOnlyCreateImage->setChecked( c->readBoolEntry( "only_create_image", false ) );


  K3bIsoOptions o = K3bIsoOptions::load( c );
  m_imageSettingsWidget->load( o );
  m_advancedImageSettingsWidget->load( o );
  m_volumeDescWidget->load( o );
}


void K3bMixedBurnDialog::saveUserDefaults()
{
  KConfig* c = kapp->config();

  c->setGroup( "default mixed settings" );

//   c->writeEntry( "dummy_mode", m_checkDummy->isChecked() );
//   c->writeEntry( "dao", m_checkDao->isChecked() );
//   c->writeEntry( "on_the_fly", m_checkOnTheFly->isChecked() );
//   c->writeEntry( "burnproof", m_checkBurnProof->isChecked() );
//   c->writeEntry( "remove_image", m_checkDeleteImage->isChecked() );
//   c->writeEntry( "only_create_image", m_checkOnlyCreateImage->isChecked() );


  K3bIsoOptions o;
  m_imageSettingsWidget->save( o );
  m_advancedImageSettingsWidget->save( o );
  m_volumeDescWidget->save( o );
  o.save( c );


//   if( m_tempDirSelectionWidget->isEnabled() ) {
//     kapp->config()->setGroup( "General Options" );
//     QFileInfo fi( m_tempDirSelectionWidget->tempPath() );
//     QString path;
//     if( fi.isFile() )
//       path = fi.dirPath();
//     else
//       path = fi.filePath();

//     kapp->config()->writeEntry( "Temp Dir", path );
//   }
}


void K3bMixedBurnDialog::slotOk()
{
  K3bProjectBurnDialog::slotOk();
}


#include "k3bmixedburndialog.moc"

