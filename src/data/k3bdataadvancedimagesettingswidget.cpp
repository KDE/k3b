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

#include "k3bdataadvancedimagesettingswidget.h"

#include "../k3bisooptions.h"

#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qheader.h>
#include <qwhatsthis.h>
#include <qpoint.h>
#include <qpainter.h>
#include <qpalette.h>
#include <qvalidator.h>
#include <qregexp.h>

#include <klistview.h>
#include <kcombobox.h>
#include <klocale.h>
#include <kdebug.h>


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



class K3bDataAdvancedImageSettingsWidget::PrivateIsoWhatsThis : public QWhatsThis
{
public:
  PrivateIsoWhatsThis( K3bDataAdvancedImageSettingsWidget* w ) 
    : QWhatsThis( w->m_viewIsoSettings->viewport() ) {
    this->w = w;
  }

  QString text( const QPoint& p ) {
    QListViewItem* i = w->m_viewIsoSettings->selectedItem(); // dies funktioniert nur bei rechtsklick
    QListViewItem* i2 = w->m_viewIsoSettings->itemAt( p ); // dies funktioniert nur bei action whatsthis

    if( i2 != 0 )
      kdDebug() << "at p " << i2->text(0) << endl;

    if( i == w->m_checkAllowUntranslatedFilenames )
      return i18n( "Force all options below" );
    else if( i == w->m_radioIsoLevel1 || 
	     i == w->m_radioIsoLevel2 || 
	     i == w->m_radioIsoLevel3 ||
	     i == w->m_isoLevelController )
      return i18n( "<p>Set the ISO-9660 conformance level.\n"
		   "<ul>\n"
		   "<li>Level 1: Files may only consist of one section and filenames are restricted "
		   "to 8.3 characters.</li>\n"
		   "<li>Level 2: Files may only consist of one section.</li>\n"
		   "<li>Level 3: No restrictions.</li>\n"
		   "</ul>\n"
		   "<p>With all ISO-9660 levels, all filenames are restricted to upper case letters, "
		   "numbers and the underscore (_). The maximum filename length is 31 characters, the "
		   "directory nesting level is restricted to 8 and the maximum path length is limited "
		   "to 255 characters. (These restrictions may be violated with the additional ISO-9660 K3b offers)." );
    else
      return i18n("Set special Iso9660 Filesystem preferences.");
  }
  
private:
  K3bDataAdvancedImageSettingsWidget* w;
};



class K3bDataAdvancedImageSettingsWidget::PrivateCheckViewItem : public QCheckListItem
{
public:
  PrivateCheckViewItem( QListView* parent, const QString& text, Type tt = Controller )
    : QCheckListItem( parent, text, tt ),
      m_bEnabled(true) {
  }

  PrivateCheckViewItem( QListViewItem* parent, const QString& text, Type tt = Controller )
    : QCheckListItem( parent, text, tt ), 
      m_bEnabled(true) {
  }

  void setOn( bool b ) {
    if( m_bEnabled )
      QCheckListItem::setOn(b);

    // enable or disable all children
    QListViewItem* item = firstChild();
    while( item ) {
      PrivateCheckViewItem* pi = (PrivateCheckViewItem*)item;
      if( pi )
	pi->setEnabled(!b);
      item = item->nextSibling();
    }
  }

  void setEnabled( bool b ) {
    if( b != m_bEnabled ) {
      m_bEnabled = b;
      repaint();
    }
  }

  void paintCell( QPainter* p, const QColorGroup& cg, int column, int width, int align ) {
    QColorGroup ncg(cg);
    if( !m_bEnabled )
      ncg.setColor( QColorGroup::Text, gray );

    QCheckListItem::paintCell( p, ncg, column, width, align );
  }


private:
  bool m_bEnabled;
};


K3bDataAdvancedImageSettingsWidget::K3bDataAdvancedImageSettingsWidget( QWidget* parent, const char* name )
  : base_K3bAdvancedDataImageSettings( parent, name )
{
  m_viewIsoSettings->header()->hide();
  
  // create WhatsThis for the isoSettings view
  (void)new PrivateIsoWhatsThis( this );

  // create all the view items
  m_checkAllowUntranslatedFilenames = new PrivateCheckViewItem( m_viewIsoSettings, 
								i18n( "Allow untranslated filenames" ), 
								QCheckListItem::CheckBox );
  m_checkAllowMaxLengthFilenames = new PrivateCheckViewItem( m_checkAllowUntranslatedFilenames, 
							     i18n( "Allow max length filenames (37 characters)" ),
							     QCheckListItem::CheckBox );
  m_checkAllowFullAscii = new PrivateCheckViewItem( m_checkAllowUntranslatedFilenames, 
						    i18n( "Allow full ASCII charset" ),
						    QCheckListItem::CheckBox );
  m_checkAllowOther = new PrivateCheckViewItem( m_checkAllowUntranslatedFilenames, 
						i18n( "Allow ~ and #" ),
						QCheckListItem::CheckBox );
  m_checkAllowLowercaseCharacters = new PrivateCheckViewItem( m_checkAllowUntranslatedFilenames, 
							      i18n( "Allow lowercase characters" ),
							      QCheckListItem::CheckBox );
  m_checkAllowMultiDot = new PrivateCheckViewItem( m_checkAllowUntranslatedFilenames, 
						   i18n( "Allow multiple dots" ),
						   QCheckListItem::CheckBox );
  m_checkAllow31CharFilenames = new PrivateCheckViewItem( m_checkAllowUntranslatedFilenames, 
							  i18n( "Allow 31 character filenames" ),
							  QCheckListItem::CheckBox );
  m_checkAllowBeginningPeriod = new PrivateCheckViewItem( m_checkAllowUntranslatedFilenames, 
							  i18n( "Allow leading period" ),
							  QCheckListItem::CheckBox );
  m_checkOmitVersionNumbers = new PrivateCheckViewItem( m_checkAllowUntranslatedFilenames, 
							i18n( "Omit version numbers" ),
							QCheckListItem::CheckBox );
  m_checkOmitTrailingPeriod = new PrivateCheckViewItem( m_checkAllowUntranslatedFilenames, 
							i18n( "Omit trailing period" ),
							QCheckListItem::CheckBox );

  m_checkAllowUntranslatedFilenames->setOpen(true);


  m_checkCreateTransTbl = new QCheckListItem( m_viewIsoSettings, 
					      i18n( "Create TRANS.TBL files" ),
					      QCheckListItem::CheckBox );
  m_checkHideTransTbl = new QCheckListItem( m_viewIsoSettings, 
					    i18n( "Hide TRANS.TBL files in Joliet" ),
					    QCheckListItem::CheckBox );
  m_checkFollowSymbolicLinks = new QCheckListItem( m_viewIsoSettings, 
						   i18n( "Follow symbolic links" ),
						   QCheckListItem::CheckBox );


  m_isoLevelController = new QCheckListItem( m_viewIsoSettings,
					     i18n("ISO Level") );

  m_radioIsoLevel1 = new QCheckListItem( m_isoLevelController, 
					 i18n("Level 1"),
					 QCheckListItem::RadioButton );
  m_radioIsoLevel2 = new QCheckListItem( m_isoLevelController, 
					 i18n("Level 2"),
					 QCheckListItem::RadioButton );
  m_radioIsoLevel3 = new QCheckListItem( m_isoLevelController, 
					 i18n("Level 3"),
					 QCheckListItem::RadioButton );

  m_isoLevelController->setOpen(true);



  m_comboInputCharset->setValidator( new QRegExpValidator( QRegExp("[\\w_-]*"), this ) );

  // fill charset combo
  for( int i = 0; mkisofsCharacterSets[i]; i++ ) {
    m_comboInputCharset->insertItem( QString( mkisofsCharacterSets[i] ) );
  }
}


K3bDataAdvancedImageSettingsWidget::~K3bDataAdvancedImageSettingsWidget()
{
}


void K3bDataAdvancedImageSettingsWidget::load( const K3bIsoOptions& o )
{
  switch( o.ISOLevel() ) {
  case 1:
    m_radioIsoLevel1->setOn(true);
    break;
  case 2:
    m_radioIsoLevel2->setOn(true);
    break;
  case 3:
    m_radioIsoLevel3->setOn(true);
    break;
  }

  m_checkForceInputCharset->setChecked( o.forceInputCharset() );
  m_comboInputCharset->setEditText( o.inputCharset() );

  m_checkCreateTransTbl->setOn( o.createTRANS_TBL() );
  m_checkHideTransTbl->setOn( o.hideTRANS_TBL() );
  m_checkAllowUntranslatedFilenames->setOn( o.ISOuntranslatedFilenames() );
  m_checkAllow31CharFilenames->setOn( o.ISOallow31charFilenames() );
  m_checkAllowMaxLengthFilenames->setOn( o.ISOmaxFilenameLength() );
  m_checkAllowBeginningPeriod->setOn( o.ISOallowPeriodAtBegin() );
  m_checkAllowFullAscii->setOn( o.ISOrelaxedFilenames() );
  m_checkOmitVersionNumbers->setOn( o.ISOomitVersionNumbers() );
  m_checkOmitTrailingPeriod->setOn( o.ISOomitTrailingPeriod() );
  m_checkAllowOther->setOn( o.ISOnoIsoTranslate() );
  m_checkAllowMultiDot->setOn( o.ISOallowMultiDot() );
  m_checkAllowLowercaseCharacters->setOn( o.ISOallowLowercase() );
  m_checkFollowSymbolicLinks->setOn( o.followSymbolicLinks() );
}


void K3bDataAdvancedImageSettingsWidget::save( K3bIsoOptions& o )
{
  // save iso-level
  if( m_radioIsoLevel3->isOn() )
    o.setISOLevel( 3 );
  else if( m_radioIsoLevel2->isOn() )
    o.setISOLevel( 2 );
  else
    o.setISOLevel( 1 );
	
  o.setForceInputCharset( m_checkForceInputCharset->isChecked() );
  o.setInputCharset( m_comboInputCharset->currentText() );

  o.setCreateTRANS_TBL( m_checkCreateTransTbl->isOn() );
  o.setHideTRANS_TBL( m_checkHideTransTbl->isOn() );
  o.setISOuntranslatedFilenames( m_checkAllowUntranslatedFilenames->isOn() );
  o.setISOallow31charFilenames( m_checkAllow31CharFilenames->isOn() );
  o.setISOmaxFilenameLength( m_checkAllowMaxLengthFilenames->isOn() );
  o.setISOallowPeriodAtBegin( m_checkAllowBeginningPeriod->isOn() );
  o.setISOrelaxedFilenames( m_checkAllowFullAscii->isOn() );
  o.setISOomitVersionNumbers( m_checkOmitVersionNumbers->isOn() );
  o.setISOomitTrailingPeriod( m_checkOmitTrailingPeriod->isOn() );
  o.setISOnoIsoTranslate( m_checkAllowOther->isOn() );
  o.setISOallowMultiDot( m_checkAllowMultiDot->isOn() );
  o.setISOallowLowercase( m_checkAllowLowercaseCharacters->isOn() );
  o.setFollowSymbolicLinks( m_checkFollowSymbolicLinks->isOn() );
}


#include "k3bdataadvancedimagesettingswidget.moc"
