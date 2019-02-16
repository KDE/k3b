/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bmovixoptionswidget.h"
#include "k3bmovixdoc.h"
#include "k3bmovixprogram.h"

#include <KConfig>
#include <KComboBox>
#include <KLocalizedString>

#include <QDebug>
#include <QLocale>
#include <QMap>
#include <QStringList>
#include <QCheckBox>
#include <QLabel>
#include <QSpinBox>


class K3b::MovixOptionsWidget::LanguageSelectionHelper
{
public:
    LanguageSelectionHelper( QComboBox* box )
        : m_box(box) {
    }

    void insertLanguages( const QStringList& langs ) {
        m_box->clear();
        m_langMap.clear();

        for( QStringList::const_iterator it = langs.begin(); it != langs.end(); ++it ) {
            if( *it == i18n("default") )
                m_box->addItem( *it );
            else {
                m_langMap[m_box->count()] = *it;
                m_indexMap[*it] = m_box->count();
                m_box->addItem( QLocale( *it ).nativeLanguageName() );
            }
        }
    }

    QString selectedLanguage() const {
        if( m_box->currentIndex() == 0 )
            return i18n("default");
        else
            return m_langMap[m_box->currentIndex()];
    }

    void setLanguage( const QString& l ) {
        QMap<QString,int>::const_iterator it = m_indexMap.constFind(l);
        if( it == m_indexMap.constEnd() )
            m_box->setCurrentIndex( 0 );
        else
            m_box->setCurrentIndex( *it );
    }

private:
    QComboBox* m_box;
    QMap<int,QString> m_langMap;
    QMap<QString,int> m_indexMap;
};


K3b::MovixOptionsWidget::MovixOptionsWidget( QWidget* parent )
    : QWidget( parent )
{
    setupUi( this );
    m_keyboardLangHelper = new LanguageSelectionHelper( m_comboKeyboardLayout );
    m_helpLangHelper = new LanguageSelectionHelper( m_comboBootMessageLanguage );
}


K3b::MovixOptionsWidget::~MovixOptionsWidget()
{
    delete m_keyboardLangHelper;
    delete m_helpLangHelper;
}


void K3b::MovixOptionsWidget::init( const K3b::MovixBin* bin )
{
    m_labelAudioBackground->setVisible( bin->hasFeature( "newfiles" ) );
    m_comboAudioBackground->setVisible( bin->hasFeature( "newfiles" ) );
    m_labelKeyboardLayout->setVisible( bin->hasFeature( "newfiles" ) );
    m_comboKeyboardLayout->setVisible( bin->hasFeature( "newfiles" ) );

    qDebug() << bin->supportedSubtitleFonts();
    m_comboSubtitleFontset->addItems( bin->supportedSubtitleFonts() );
    m_helpLangHelper->insertLanguages( bin->supportedLanguages() );
    m_comboDefaultBootLabel->addItems( bin->supportedBootLabels() );
    m_keyboardLangHelper->insertLanguages( bin->supportedKbdLayouts() );
    m_comboAudioBackground->addItems( bin->supportedBackgrounds() );
}


void K3b::MovixOptionsWidget::readSettings( K3b::MovixDoc* doc )
{
    if ( doc->subtitleFontset().isEmpty() )
        m_comboSubtitleFontset->setCurrentIndex( 0 );
    else
        m_comboSubtitleFontset->setCurrentItem( doc->subtitleFontset(), false );

    if ( doc->defaultBootLabel().isEmpty() )
        m_comboDefaultBootLabel->setCurrentIndex( 0 );
    else
        m_comboDefaultBootLabel->setCurrentItem( doc->defaultBootLabel(), false );

    if ( doc->audioBackground().isEmpty() )
        m_comboAudioBackground->setCurrentIndex( 0 );
    else
        m_comboAudioBackground->setCurrentItem( doc->audioBackground(), false );

    m_spinLoop->setValue( doc->loopPlaylist() );
    m_editAdditionalMplayerOptions->setText( doc->additionalMPlayerOptions() );
    m_editUnwantedMplayerOptions->setText( doc->unwantedMPlayerOptions() );
    m_helpLangHelper->setLanguage( doc->bootMessageLanguage() );
    m_keyboardLangHelper->setLanguage( doc->keyboardLayout() );
    m_checkShutdown->setChecked( doc->shutdown() );
    m_checkReboot->setChecked( doc->reboot() );
    m_checkEject->setChecked( doc->ejectDisk() );
    m_checkRandomPlay->setChecked( doc->randomPlay() );
    m_checkNoDma->setChecked( doc->noDma() );
}


void K3b::MovixOptionsWidget::saveSettings( K3b::MovixDoc* doc )
{
    doc->setShutdown( m_checkShutdown->isChecked() );
    doc->setReboot( m_checkReboot->isChecked() );
    doc->setEjectDisk( m_checkEject->isChecked() );
    doc->setSubtitleFontset( m_comboSubtitleFontset->currentText() );
    doc->setBootMessageLanguage( m_helpLangHelper->selectedLanguage() );
    doc->setDefaultBootLabel( m_comboDefaultBootLabel->currentText() );
    doc->setKeyboardLayout( m_keyboardLangHelper->selectedLanguage() );
    doc->setAudioBackground( m_comboAudioBackground->currentText() );
    doc->setAdditionalMPlayerOptions( m_editAdditionalMplayerOptions->text() );
    doc->setUnwantedMPlayerOptions( m_editUnwantedMplayerOptions->text() );
    doc->setLoopPlaylist( m_spinLoop->value() );
    doc->setRandomPlay( m_checkRandomPlay->isChecked() );
    doc->setNoDma( m_checkNoDma->isChecked() );
}


void K3b::MovixOptionsWidget::loadConfig( const KConfigGroup & c )
{
    QString s = c.readEntry("subtitle_fontset");
    if( !s.isEmpty() && s != "none" && m_comboSubtitleFontset->contains(s) )
        m_comboSubtitleFontset->setCurrentItem( s, false );
    else
        m_comboSubtitleFontset->setCurrentIndex( 0 ); // none

    m_spinLoop->setValue( c.readEntry("loop", 1 ) );
    m_editAdditionalMplayerOptions->setText( c.readEntry( "additional_mplayer_options" ) );
    m_editUnwantedMplayerOptions->setText( c.readEntry( "unwanted_mplayer_options" ) );

    s = c.readEntry("boot_message_language");
    m_helpLangHelper->setLanguage( s == "default" ? QString() : s );

    s = c.readEntry( "default_boot_label" );
    if( !s.isEmpty() && s != "default" && m_comboDefaultBootLabel->contains(s) )
        m_comboDefaultBootLabel->setCurrentItem( s, false );
    else
        m_comboDefaultBootLabel->setCurrentIndex( 0 );  // default

    s = c.readEntry("audio_background");
    if( !s.isEmpty() && s != "default" && m_comboAudioBackground->contains(s) )
        m_comboAudioBackground->setCurrentItem( s, false );
    else
        m_comboAudioBackground->setCurrentIndex( 0 ); // default

    s = c.readEntry("keyboard_layout");
    m_keyboardLangHelper->setLanguage( s == "default" ? QString() : s );

    m_checkShutdown->setChecked( c.readEntry( "shutdown", false) );
    m_checkReboot->setChecked( c.readEntry( "reboot", false ) );
    m_checkEject->setChecked( c.readEntry( "eject", false ) );
    m_checkRandomPlay->setChecked( c.readEntry( "random_play", false ) );
    m_checkNoDma->setChecked( c.readEntry( "no_dma", false ) );
}


void K3b::MovixOptionsWidget::saveConfig( KConfigGroup c )
{
    if( m_comboSubtitleFontset->currentIndex() == 0 )
        c.writeEntry( "subtitle_fontset", "none" );
    else
        c.writeEntry( "subtitle_fontset", m_comboSubtitleFontset->currentText() );

    c.writeEntry( "loop", m_spinLoop->value() );
    c.writeEntry( "additional_mplayer_options", m_editAdditionalMplayerOptions->text() );
    c.writeEntry( "unwanted_mplayer_options", m_editUnwantedMplayerOptions->text() );

    if( m_comboBootMessageLanguage->currentIndex() == 0 )
        c.writeEntry( "boot_message_language", "default" );
    else
        c.writeEntry( "boot_message_language", m_helpLangHelper->selectedLanguage() );

    if( m_comboDefaultBootLabel->currentIndex() == 0 )
        c.writeEntry( "default_boot_label", "default" );
    else
        c.writeEntry( "default_boot_label", m_comboDefaultBootLabel->currentText() );

    if( m_comboAudioBackground->currentIndex() == 0 )
        c.writeEntry( "audio_background", "default" );
    else
        c.writeEntry( "audio_background", m_comboAudioBackground->currentText() );

    if( m_comboKeyboardLayout->currentIndex() == 0 )
        c.writeEntry( "keyboard_layout", "default" );
    else
        c.writeEntry( "keyboard_layout", m_keyboardLangHelper->selectedLanguage() );

    c.writeEntry( "shutdown", m_checkShutdown->isChecked() );
    c.writeEntry( "reboot", m_checkReboot->isChecked() );
    c.writeEntry( "eject", m_checkEject->isChecked() );
    c.writeEntry( "random_play", m_checkRandomPlay->isChecked() );
    c.writeEntry( "no_dma", m_checkNoDma->isChecked() );
}



