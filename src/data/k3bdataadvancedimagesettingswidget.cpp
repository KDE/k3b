#include "k3bdataadvancedimagesettingswidget.h"

#include "../k3bisooptions.h"

#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <kcombobox.h>


K3bDataAdvancedImageSettingsWidget::K3bDataAdvancedImageSettingsWidget( QWidget* parent, const char* name )
  : base_K3bAdvancedDataImageSettings( parent, name )
{
}


K3bDataAdvancedImageSettingsWidget::~K3bDataAdvancedImageSettingsWidget()
{
}


void K3bDataAdvancedImageSettingsWidget::load( const K3bIsoOptions& o )
{
  switch( o.ISOLevel() ) {
  case 1:
    m_radioIsoLevel1->setChecked(true);
    break;
  case 2:
    m_radioIsoLevel2->setChecked(true);
    break;
  case 3:
    m_radioIsoLevel3->setChecked(true);
    break;
  }

  m_checkForceInputCharset->setChecked( o.forceInputCharset() );
  m_comboInputCharset->setEditText( o.inputCharset() );

  m_checkCreateTransTbl->setChecked( o.createTRANS_TBL() );
  m_checkHideTransTbl->setChecked( o.hideTRANS_TBL() );
  m_checkAllowUntranslatedFilenames->setChecked( o.ISOuntranslatedFilenames() );
  m_checkAllow31CharFilenames->setChecked( o.ISOallow31charFilenames() );
  m_checkAllowMaxLengthFilenames->setChecked( o.ISOmaxFilenameLength() );
  m_checkAllowBeginningPeriod->setChecked( o.ISOallowPeriodAtBegin() );
  m_checkAllowFullAscii->setChecked( o.ISOrelaxedFilenames() );
  m_checkOmitVersionNumbers->setChecked( o.ISOomitVersionNumbers() );
  m_checkOmitTrailingPeriod->setChecked( o.ISOomitTrailingPeriod() );
  m_checkAllowOther->setChecked( o.ISOnoIsoTranslate() );
  m_checkAllowMultiDot->setChecked( o.ISOallowMultiDot() );
  m_checkAllowLowercaseCharacters->setChecked( o.ISOallowLowercase() );
  m_checkFollowSymbolicLinks->setChecked( o.followSymbolicLinks() );
}


void K3bDataAdvancedImageSettingsWidget::save( K3bIsoOptions& o )
{
  // save iso-level
  if( m_groupIsoLevel->selected() == m_radioIsoLevel3 )
    o.setISOLevel( 3 );
  else if( m_groupIsoLevel->selected() == m_radioIsoLevel2 )
    o.setISOLevel( 2 );
  else
    o.setISOLevel( 1 );
	
  o.setForceInputCharset( m_checkForceInputCharset->isChecked() );
  o.setInputCharset( m_comboInputCharset->currentText() );

  o.setCreateTRANS_TBL( m_checkCreateTransTbl->isChecked() );
  o.setHideTRANS_TBL( m_checkHideTransTbl->isChecked() );
  o.setISOuntranslatedFilenames( m_checkAllowUntranslatedFilenames->isChecked() );
  o.setISOallow31charFilenames( m_checkAllow31CharFilenames->isChecked() );
  o.setISOmaxFilenameLength( m_checkAllowMaxLengthFilenames->isChecked() );
  o.setISOallowPeriodAtBegin( m_checkAllowBeginningPeriod->isChecked() );
  o.setISOrelaxedFilenames( m_checkAllowFullAscii->isChecked() );
  o.setISOomitVersionNumbers( m_checkOmitVersionNumbers->isChecked() );
  o.setISOomitTrailingPeriod( m_checkOmitTrailingPeriod->isChecked() );
  o.setISOnoIsoTranslate( m_checkAllowOther->isChecked() );
  o.setISOallowMultiDot( m_checkAllowMultiDot->isChecked() );
  o.setISOallowLowercase( m_checkAllowLowercaseCharacters->isChecked() );
  o.setFollowSymbolicLinks( m_checkFollowSymbolicLinks->isChecked() );
}


#include "k3bdataadvancedimagesettingswidget.moc"
