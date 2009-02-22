/*
 *
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

#include "k3bdataadvancedimagesettingswidget.h"

#include "k3bisooptions.h"

#include <qcheckbox.h>
#include <qradiobutton.h>
#include <q3buttongroup.h>
#include <q3header.h>

#include <qpoint.h>
#include <qpainter.h>
#include <qpalette.h>
#include <qvalidator.h>
#include <qregexp.h>

#include <k3listview.h>
#include <kcombobox.h>
#include <klocale.h>
#include <kdebug.h>
#include <Q3WhatsThis>


class K3b::DataAdvancedImageSettingsWidget::PrivateIsoWhatsThis : public Q3WhatsThis
{
public:
    PrivateIsoWhatsThis( K3b::DataAdvancedImageSettingsWidget* w )
        : Q3WhatsThis( w->m_viewIsoSettings->viewport() ) {
        this->w = w;
    }

    QString text( const QPoint& p ) {
        Q3ListViewItem* i = w->m_viewIsoSettings->selectedItem(); // dies funktioniert nur bei rechtsklick
        Q3ListViewItem* i2 = w->m_viewIsoSettings->itemAt( p ); // dies funktioniert nur bei action whatsthis

        if( i2 != 0 )
            kDebug() << "at p " << i2->text(0);

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
            return i18n("Set special ISO9660 Filesystem preferences.");
    }

private:
    K3b::DataAdvancedImageSettingsWidget* w;
};



class K3b::DataAdvancedImageSettingsWidget::PrivateCheckViewItem : public Q3CheckListItem
{
public:
    PrivateCheckViewItem( Q3ListView* parent, const QString& text, Type tt = Controller )
        : Q3CheckListItem( parent, text, tt ) {
    }

    PrivateCheckViewItem( Q3ListViewItem* parent, const QString& text, Type tt = Controller )
        : Q3CheckListItem( parent, text, tt ) {
    }

protected:
    void stateChange( bool on ) {
        // enable or disable all children
        Q3ListViewItem* item = firstChild();
        while( item ) {
            if( PrivateCheckViewItem* pi = dynamic_cast<PrivateCheckViewItem*>(item) )
                pi->setEnabled( !on );
            item = item->nextSibling();
        }
    }
};


K3b::DataAdvancedImageSettingsWidget::DataAdvancedImageSettingsWidget( QWidget* parent )
    : QWidget( parent )
{
    setupUi( this );

    m_viewIsoSettings->header()->hide();
    m_viewIsoSettings->setSorting( -1 );

    // create WhatsThis for the isoSettings view
    (void)new PrivateIsoWhatsThis( this );

    // create all the view items
    Q3CheckListItem* iso9660Root = new Q3CheckListItem( m_viewIsoSettings,
                                                        i18n("IS09660 Settings"),
                                                        Q3CheckListItem::Controller );
    Q3CheckListItem* rrRoot = new Q3CheckListItem( m_viewIsoSettings,
                                                   iso9660Root,
                                                   i18n("Rock Ridge Settings"),
                                                   Q3CheckListItem::Controller );
    Q3CheckListItem* jolietRoot = new Q3CheckListItem( m_viewIsoSettings,
                                                       rrRoot,
                                                       i18n("Joliet Settings"),
                                                       Q3CheckListItem::Controller );
    Q3CheckListItem* miscRoot = new Q3CheckListItem( m_viewIsoSettings,
                                                     jolietRoot,
                                                     i18n("Misc Settings"),
                                                     Q3CheckListItem::Controller );

    // ISO9660 settings
    m_checkAllowUntranslatedFilenames = new PrivateCheckViewItem( iso9660Root,
                                                                  i18n( "Allow untranslated ISO9660 filenames" ),
                                                                  Q3CheckListItem::CheckBox );
    m_checkAllowMaxLengthFilenames = new PrivateCheckViewItem( m_checkAllowUntranslatedFilenames,
                                                               i18n( "Allow max length ISO9660 filenames (37 characters)" ),
                                                               Q3CheckListItem::CheckBox );
    m_checkAllowFullAscii = new PrivateCheckViewItem( m_checkAllowUntranslatedFilenames,
                                                      i18n( "Allow full ASCII charset for ISO9660 filenames" ),
                                                      Q3CheckListItem::CheckBox );
    m_checkAllowOther = new PrivateCheckViewItem( m_checkAllowUntranslatedFilenames,
                                                  i18n( "Allow ~ and # in ISO9660 filenames" ),
                                                  Q3CheckListItem::CheckBox );
    m_checkAllowLowercaseCharacters = new PrivateCheckViewItem( m_checkAllowUntranslatedFilenames,
                                                                i18n( "Allow lowercase characters in ISO9660 filenames" ),
                                                                Q3CheckListItem::CheckBox );
    m_checkAllowMultiDot = new PrivateCheckViewItem( m_checkAllowUntranslatedFilenames,
                                                     i18n( "Allow multiple dots in ISO9660 filenames" ),
                                                     Q3CheckListItem::CheckBox );
    m_checkAllow31CharFilenames = new PrivateCheckViewItem( m_checkAllowUntranslatedFilenames,
                                                            i18n( "Allow 31 character ISO9660 filenames" ),
                                                            Q3CheckListItem::CheckBox );
    m_checkAllowBeginningPeriod = new PrivateCheckViewItem( m_checkAllowUntranslatedFilenames,
                                                            i18n( "Allow leading period in ISO9660 filenames" ),
                                                            Q3CheckListItem::CheckBox );
    m_checkOmitVersionNumbers = new PrivateCheckViewItem( m_checkAllowUntranslatedFilenames,
                                                          i18n( "Omit version numbers in ISO9660 filenames" ),
                                                          Q3CheckListItem::CheckBox );
    m_checkOmitTrailingPeriod = new PrivateCheckViewItem( m_checkAllowUntranslatedFilenames,
                                                          i18n( "Omit trailing period in ISO9660 filenames" ),
                                                          Q3CheckListItem::CheckBox );

    m_checkAllowUntranslatedFilenames->setOpen(true);
    m_isoLevelController = new Q3CheckListItem( iso9660Root,
                                                m_checkAllowUntranslatedFilenames,
                                                i18n("ISO Level") );

    m_radioIsoLevel3 = new Q3CheckListItem( m_isoLevelController,
                                            i18n("Level %1",3),
                                            Q3CheckListItem::RadioButton );
    m_radioIsoLevel2 = new Q3CheckListItem( m_isoLevelController,
                                            i18n("Level %1",2),
                                            Q3CheckListItem::RadioButton );
    m_radioIsoLevel1 = new Q3CheckListItem( m_isoLevelController,
                                            i18n("Level %1",1),
                                            Q3CheckListItem::RadioButton );

    m_isoLevelController->setOpen(true);

    // Joliet Settings
    m_checkJolietLong = new Q3CheckListItem( jolietRoot,
                                             i18n("Allow 103 character Joliet filenames"),
                                             Q3CheckListItem::CheckBox );

    // Rock Ridge Settings
    m_checkCreateTransTbl = new Q3CheckListItem( rrRoot,
                                                 i18n( "Create TRANS.TBL files" ),
                                                 Q3CheckListItem::CheckBox );
    m_checkHideTransTbl = new Q3CheckListItem( rrRoot, m_checkCreateTransTbl,
                                               i18n( "Hide TRANS.TBL files in Joliet" ),
                                               Q3CheckListItem::CheckBox );

    // Misc Settings
//   m_checkFollowSymbolicLinks = new QCheckListItem( m_viewIsoSettings,
// 						   i18n( "Follow symbolic links" ),
// 						   QCheckListItem::CheckBox );

    m_checkDoNotCacheInodes = new Q3CheckListItem( miscRoot,
                                                   i18n("Do not cache inodes" ),
                                                   Q3CheckListItem::CheckBox );

    m_checkDoNotImportSession = new Q3CheckListItem( miscRoot,
                                                     i18n("Do not import previous session" ),
                                                     Q3CheckListItem::CheckBox );

    iso9660Root->setOpen( true );
    jolietRoot->setOpen( true );
    rrRoot->setOpen( true );
    miscRoot->setOpen( true );

    connect( m_checkJoliet, SIGNAL(toggled(bool)), this, SLOT(slotJolietToggled(bool)) );
}


K3b::DataAdvancedImageSettingsWidget::~DataAdvancedImageSettingsWidget()
{
}


void K3b::DataAdvancedImageSettingsWidget::load( const K3b::IsoOptions& o )
{
    m_checkRockRidge->setChecked( o.createRockRidge() );
    m_checkJoliet->setChecked( o.createJoliet() );
    m_checkUdf->setChecked( o.createUdf() );

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

    m_checkPreservePermissions->setChecked( o.preserveFilePermissions() );

    // RR settings
    m_checkCreateTransTbl->setOn( o.createTRANS_TBL() );
    m_checkHideTransTbl->setOn( o.hideTRANS_TBL() );

    // iso9660 settings
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

    // joliet settings
    m_checkJolietLong->setOn( o.jolietLong() );

    // misc (FIXME: should not be here)
    m_checkDoNotCacheInodes->setOn( o.doNotCacheInodes() );
    m_checkDoNotImportSession->setOn( o.doNotImportSession() );

    slotJolietToggled( m_checkJoliet->isChecked() );
}


void K3b::DataAdvancedImageSettingsWidget::save( K3b::IsoOptions& o )
{
    o.setCreateRockRidge( m_checkRockRidge->isChecked() );
    o.setCreateJoliet( m_checkJoliet->isChecked() );
    o.setCreateUdf( m_checkUdf->isChecked() );

    // save iso-level
    if( m_radioIsoLevel3->isOn() )
        o.setISOLevel( 3 );
    else if( m_radioIsoLevel2->isOn() )
        o.setISOLevel( 2 );
    else
        o.setISOLevel( 1 );

    o.setPreserveFilePermissions( m_checkPreservePermissions->isChecked() );

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
    //  o.setFollowSymbolicLinks( m_checkFollowSymbolicLinks->isOn() );
    o.setJolietLong( m_checkJolietLong->isOn() );
    o.setDoNotCacheInodes( m_checkDoNotCacheInodes->isOn() );
    o.setDoNotImportSession( m_checkDoNotImportSession->isOn() );
}


void K3b::DataAdvancedImageSettingsWidget::slotJolietToggled( bool on )
{
    m_checkJolietLong->setEnabled( on );
}

#include "k3bdataadvancedimagesettingswidget.moc"
