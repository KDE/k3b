/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bdataimagesettingswidget.h"
#include "k3bdataadvancedimagesettingsdialog.h"
#include "k3bdatavolumedescdialog.h"
#include "k3bisooptions.h"

#include <KLocalizedString>
#include <KMessageBox>

#include <QDebug>
#include <QCheckBox>
#include <QComboBox>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>

// indices for the filesystems combobox
static const int FS_LINUX_ONLY = 0;
static const int FS_LINUX_AND_WIN = 1;
static const int FS_UDF = 2;
static const int FS_DOS_COMP = 3;
static const int FS_CUSTOM = 4;
static const int FS_MAX = 5;

static const char* s_fsPresetNames[] = {
    I18N_NOOP("Linux/Unix only"),
    I18N_NOOP("Linux/Unix + Windows"),
    I18N_NOOP("Very large files (UDF)"),
    I18N_NOOP("DOS Compatibility"),
    I18N_NOOP("Custom")
};

static bool s_fsPresetsInitialized = false;
static K3b::IsoOptions s_fsPresets[FS_CUSTOM];

// indices for the whitespace treatment combobox
static const int WS_NO_CHANGE = 0;
static const int WS_STRIP = 1;
static const int WS_EXTENDED_STRIP = 2;
static const int WS_REPLACE = 3;

// indices for the symlink handling combobox
static const int SYM_NO_CHANGE = 0;
static const int SYM_DISCARD_BROKEN = 1;
static const int SYM_DISCARD_ALL = 2;
static const int SYM_FOLLOW = 3;


//
// returns true if the part of the options that is presented in the advanced custom
// settings dialog is equal. used to determine if some preset is used.
//
static bool compareAdvancedOptions( const K3b::IsoOptions& o1, const K3b::IsoOptions& o2 )
{
    return ( o1.createRockRidge() == o2.createRockRidge() &&
             o1.createJoliet() == o2.createJoliet() &&
             o1.createUdf() == o2.createUdf() &&
             o1.ISOallowLowercase() == o2.ISOallowLowercase() &&
             o1.ISOallowPeriodAtBegin() == o2.ISOallowPeriodAtBegin() &&
             o1.ISOallow31charFilenames() == o2.ISOallow31charFilenames() &&
             o1.ISOomitVersionNumbers() == o2.ISOomitVersionNumbers() &&
             o1.ISOomitTrailingPeriod() == o2.ISOomitTrailingPeriod() &&
             o1.ISOmaxFilenameLength() == o2.ISOmaxFilenameLength() &&
             o1.ISOrelaxedFilenames() == o2.ISOrelaxedFilenames() &&
             o1.ISOnoIsoTranslate() == o2.ISOnoIsoTranslate() &&
             o1.ISOallowMultiDot() == o2.ISOallowMultiDot() &&
             o1.ISOuntranslatedFilenames() == o2.ISOuntranslatedFilenames() &&
             o1.createTRANS_TBL() == o2.createTRANS_TBL() &&
             o1.hideTRANS_TBL() == o2.hideTRANS_TBL() &&
             o1.jolietLong() == o2.jolietLong() &&
             o1.ISOLevel() == o2.ISOLevel() &&
             o1.preserveFilePermissions() == o2.preserveFilePermissions() &&
             o1.doNotCacheInodes() == o2.doNotCacheInodes() );
}


static void initializePresets()
{
    QString vid = i18nc( "This is the default volume identifier of a data project created by K3b. "
                         "The string should not be longer than 16 characters to avoid warnings regarding "
                         "Joiliet extensions which induce this restriction.",
                         "K3b data project" );

    for ( int i = 0; i < FS_CUSTOM; ++i ) {
        s_fsPresets[i].setVolumeID( vid );
    }

    // Linux-only
    s_fsPresets[FS_LINUX_ONLY].setCreateJoliet( false );
    s_fsPresets[FS_LINUX_ONLY].setISOallow31charFilenames( true );

    // Linux + Windows
    s_fsPresets[FS_LINUX_AND_WIN].setCreateJoliet( true );
    s_fsPresets[FS_LINUX_AND_WIN].setJolietLong( true );
    s_fsPresets[FS_LINUX_AND_WIN].setISOallow31charFilenames( true );

    // UDF
    s_fsPresets[FS_UDF].setCreateJoliet( false );
    s_fsPresets[FS_UDF].setCreateUdf( true );
    s_fsPresets[FS_UDF].setISOallow31charFilenames( true );

    // DOS comp
    s_fsPresets[FS_DOS_COMP].setCreateJoliet( false );
    s_fsPresets[FS_DOS_COMP].setCreateRockRidge( false );
    s_fsPresets[FS_DOS_COMP].setISOallow31charFilenames( false );
    s_fsPresets[FS_DOS_COMP].setISOLevel( 1 );

    s_fsPresetsInitialized = true;
}



K3b::DataImageSettingsWidget::DataImageSettingsWidget( QWidget* parent )
    : QWidget( parent ),
      m_fileSystemOptionsShown(true)
{
    setupUi( this );

    m_customFsDlg = new DataAdvancedImageSettingsDialog( this );
    m_volDescDlg = new DataVolumeDescDialog( this );

    connect( m_buttonCustomFilesystems, SIGNAL(clicked()),
             this, SLOT(slotCustomFilesystems()) );
    connect( m_buttonMoreVolDescFields, SIGNAL(clicked()),
             this, SLOT(slotMoreVolDescFields()) );
    connect( m_comboSpaceHandling, SIGNAL(activated(int)),
             this, SLOT(slotSpaceHandlingChanged(int)) );

    for( int i = 0; i < FS_MAX; ++i )
        m_comboFilesystems->addItem( i18n( s_fsPresetNames[i] ) );

    if( !s_fsPresetsInitialized )
        initializePresets();

    m_comboFilesystems->setWhatsThis(
        i18n("<p><b>File System Presets</b>"
             "<p>K3b provides the following file system Presets which allow for a quick selection "
             "of the most frequently used settings.")
        + "<p><b>" + i18n(s_fsPresetNames[0]) + "</b><br>"
        + i18n("The file system is optimized for usage on Linux/Unix systems. This mainly means that "
               "it uses the Rock Ridge extensions to provide long filenames, symbolic links, and POSIX "
               "compatible file permissions.")
        + "<p><b>" + i18n(s_fsPresetNames[1]) + "</b><br>"
        + i18n("In addition to the settings for Linux/Unix the file system contains a Joliet tree which "
               "allows for long file names on Windows which does not support the Rock Ridge extensions. "
               "Be aware that the file name length is restricted to 103 characters.")
        + "<p><b>" + i18n(s_fsPresetNames[2]) + "</b><br>"
        + i18n("The file system has additional UDF entries attached to it. This raises the maximal file "
               "size to 4 GB. Be aware that the UDF support in K3b is limited.")
        + "<p><b>" + i18n(s_fsPresetNames[3]) + "</b><br>"
        + i18n("The file system is optimized for compatibility with old systems. That means file names "
               "are restricted to 8.3 characters and no symbolic links or file permissions are supported.") );
}


K3b::DataImageSettingsWidget::~DataImageSettingsWidget()
{
}


void K3b::DataImageSettingsWidget::showFileSystemOptions( bool b )
{
    m_groupFileSystem->setVisible(b);
    m_groupSymlinks->setVisible(b);
    m_groupWhitespace->setVisible(b);

    m_fileSystemOptionsShown = b;
}


void K3b::DataImageSettingsWidget::slotSpaceHandlingChanged( int i )
{
    m_editReplace->setEnabled( i == WS_REPLACE );
}


void K3b::DataImageSettingsWidget::slotCustomFilesystems()
{
    // load settings in custom window
    if( m_comboFilesystems->currentIndex() != FS_CUSTOM ) {
        m_customFsDlg->load( s_fsPresets[m_comboFilesystems->currentIndex()] );
    }

    // store the current settings in case the user cancels the changes
    K3b::IsoOptions o;
    m_customFsDlg->save( o );

    if( m_customFsDlg->exec() == QDialog::Accepted ) {
        slotFilesystemsChanged();
    }
    else {
        // reload the old settings discarding any changes
        m_customFsDlg->load( o );
    }
}


void K3b::DataImageSettingsWidget::slotFilesystemsChanged()
{
    if( !m_fileSystemOptionsShown )
        return;

    // new custom entry
    QStringList s;
    if( m_customFsDlg->m_checkRockRidge->isChecked() )
        s += i18n("Rock Ridge");
    if( m_customFsDlg->m_checkJoliet->isChecked() )
        s += i18n("Joliet");
    if( m_customFsDlg->m_checkUdf->isChecked() )
        s += i18n("UDF");
    if( s.isEmpty() )
        m_comboFilesystems->setItemText( FS_CUSTOM,i18n("Custom (ISO 9660 only)") );
    else
        m_comboFilesystems->setItemText( FS_CUSTOM,i18n("Custom (%1)", s.join(", ") ) );

    // see if any of the presets is loaded
    m_comboFilesystems->setCurrentIndex( FS_CUSTOM );
    K3b::IsoOptions o;
    m_customFsDlg->save( o );
    for( int i = 0; i < FS_CUSTOM; ++i ) {
        if( compareAdvancedOptions( o, s_fsPresets[i] ) ) {
            qDebug() << "(K3b::DataImageSettingsWidget) found preset settings: " << s_fsPresetNames[i];
            m_comboFilesystems->setCurrentIndex( i );
            break;
        }
    }

    if( m_comboFilesystems->currentIndex() == FS_CUSTOM ) {
        if( !m_customFsDlg->m_checkRockRidge->isChecked() ) {
            KMessageBox::information( this,
                                      i18n("<p>Be aware that it is not recommended to disable the Rock Ridge "
                                           "Extensions. There is no disadvantage in enabling Rock Ridge (except "
                                           "for a very small space overhead) but a lot of advantages."
                                           "<p>Without Rock Ridge Extensions symbolic links are not supported "
                                           "and will always be followed as if the \"Follow Symbolic Links\" option "
                                           "was enabled."),
                                      i18n("Rock Ridge Extensions Disabled"),
                                      "warning_about_rock_ridge" );
        }

        if( !m_customFsDlg->m_checkJoliet->isChecked() )
            KMessageBox::information( this,
                                      i18n("<p>Be aware that without the Joliet extensions Windows "
                                           "systems will not be able to display long filenames. You "
                                           "will only see the ISO 9660 filenames."
                                           "<p>If you do not intend to use the CD/DVD on a Windows "
                                           "system it is safe to disable Joliet."),
                                      i18n("Joliet Extensions Disabled"),
                                      "warning_about_joliet" );
    }
}


void K3b::DataImageSettingsWidget::slotMoreVolDescFields()
{
    // update dlg to current state
    m_volDescDlg->m_editVolumeName->setText( m_editVolumeName->text() );

    // remember old settings
    K3b::IsoOptions o;
    m_volDescDlg->save( o );

    m_volDescDlg->m_editVolumeName->setFocus( Qt::PopupFocusReason );

    // exec dlg
    if( m_volDescDlg->exec() == QDialog::Accepted ) {
        // accept new entries
        m_volDescDlg->save( o );
        m_editVolumeName->setText( o.volumeID() );
    }
    else {
        // restore old settings
        m_volDescDlg->load( o );
    }
}


void K3b::DataImageSettingsWidget::load( const K3b::IsoOptions& o )
{
    m_customFsDlg->load( o );
    m_volDescDlg->load( o );

    slotFilesystemsChanged();

    if( o.discardBrokenSymlinks() )
        m_comboSymlinkHandling->setCurrentIndex( SYM_DISCARD_BROKEN );
    else if( o.discardSymlinks() )
        m_comboSymlinkHandling->setCurrentIndex( SYM_DISCARD_ALL );
    else if( o.followSymbolicLinks() )
        m_comboSymlinkHandling->setCurrentIndex( SYM_FOLLOW );
    else
        m_comboSymlinkHandling->setCurrentIndex( SYM_NO_CHANGE );

    switch( o.whiteSpaceTreatment() ) {
    case K3b::IsoOptions::strip:
        m_comboSpaceHandling->setCurrentIndex( WS_STRIP );
        break;
    case K3b::IsoOptions::extended:
        m_comboSpaceHandling->setCurrentIndex( WS_EXTENDED_STRIP );
        break;
    case K3b::IsoOptions::replace:
        m_comboSpaceHandling->setCurrentIndex( WS_REPLACE );
        break;
    default:
        m_comboSpaceHandling->setCurrentIndex( WS_NO_CHANGE );
    }
    slotSpaceHandlingChanged( m_comboSpaceHandling->currentIndex() );

    m_editReplace->setText( o.whiteSpaceTreatmentReplaceString() );

    m_editVolumeName->setText( o.volumeID() );
}


void K3b::DataImageSettingsWidget::save( K3b::IsoOptions& o )
{
    if( m_comboFilesystems->currentIndex() != FS_CUSTOM )
        m_customFsDlg->load( s_fsPresets[m_comboFilesystems->currentIndex()] );
    m_customFsDlg->save( o );

    m_volDescDlg->save( o );

    o.setDiscardSymlinks( m_comboSymlinkHandling->currentIndex() == SYM_DISCARD_ALL );
    o.setDiscardBrokenSymlinks( m_comboSymlinkHandling->currentIndex() == SYM_DISCARD_BROKEN );
    o.setFollowSymbolicLinks( m_comboSymlinkHandling->currentIndex() == SYM_FOLLOW );

    switch( m_comboSpaceHandling->currentIndex() ) {
    case WS_STRIP:
        o.setWhiteSpaceTreatment( K3b::IsoOptions::strip );
        break;
    case WS_EXTENDED_STRIP:
        o.setWhiteSpaceTreatment( K3b::IsoOptions::extended );
        break;
    case WS_REPLACE:
        o.setWhiteSpaceTreatment( K3b::IsoOptions::replace );
        break;
    default:
        o.setWhiteSpaceTreatment( K3b::IsoOptions::noChange );
    }
    o.setWhiteSpaceTreatmentReplaceString( m_editReplace->text() );

    o.setVolumeID( m_editVolumeName->text() );
}


