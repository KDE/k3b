/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2010 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bthemeoptiontab.h"
#include "k3bapplication.h"
#include "k3bthememanager.h"
#include "k3bthememodel.h"

#include <kio/global.h>
#include <kio/netaccess.h>
#include <kio/deletejob.h>
#include <KConfig>
#include <KGlobalSettings>
#include <KLocale>
#include <KMessageBox>
#include <KStandardDirs>
#include <KTar>
#include <KUrlRequester>
#include <KUrlRequesterDialog>

#include <QFile>
#include <QFileInfo>
#include <QItemSelectionModel>
#include <QLabel>


K3b::ThemeOptionTab::ThemeOptionTab( QWidget* parent )
    : QWidget( parent ),
      m_themeModel( new ThemeModel( k3bappcore->themeManager(), this ) )
{
    setupUi( this );

    m_centerPreviewLabel->setAutoFillBackground( true );
    m_leftPreviewLabel->setAutoFillBackground( true );
    m_rightPreviewLabel->setAutoFillBackground( true );

    m_viewTheme->setModel( m_themeModel );

    connect( m_viewTheme->selectionModel(), SIGNAL(currentChanged(const QModelIndex&,const QModelIndex&)),
             this, SLOT(selectionChanged()) );
    connect( KGlobalSettings::self(), SIGNAL(appearanceChanged()),
             this, SLOT(selectionChanged()) );
    connect( m_buttonInstallTheme, SIGNAL(clicked()),
             this, SLOT(slotInstallTheme()) );
    connect( m_buttonRemoveTheme, SIGNAL(clicked()),
             this, SLOT(slotRemoveTheme()) );
}


K3b::ThemeOptionTab::~ThemeOptionTab()
{
}


void K3b::ThemeOptionTab::readSettings()
{
    m_themeModel->reload();
    
    QModelIndex index = m_themeModel->indexForTheme( k3bappcore->themeManager()->currentTheme() );
    m_viewTheme->setCurrentIndex( index );
}


bool K3b::ThemeOptionTab::saveSettings()
{
    QModelIndex index = m_viewTheme->currentIndex();
    if( Theme* theme = m_themeModel->themeForIndex( index ) ) {
        k3bappcore->themeManager()->setCurrentTheme( theme );
    }
    return true;
}


void K3b::ThemeOptionTab::selectionChanged()
{
    QModelIndex index = m_viewTheme->currentIndex();
    if( Theme* theme = m_themeModel->themeForIndex( index ) ) {
        m_centerPreviewLabel->setText( i18n("K3b - The CD/DVD Kreator") );

        QPalette pal( palette() );
        pal.setColor( backgroundRole(), theme->backgroundColor() );
        pal.setColor( foregroundRole(), theme->backgroundColor() );
        m_centerPreviewLabel->setPalette( pal );
        m_leftPreviewLabel->setPalette( pal );
        m_rightPreviewLabel->setPalette( pal );

        m_leftPreviewLabel->setPixmap( theme->pixmap( K3b::Theme::PROJECT_LEFT ) );
        m_rightPreviewLabel->setPixmap( theme->pixmap( K3b::Theme::PROJECT_RIGHT ) );

        m_buttonRemoveTheme->setEnabled( theme->local() );
    }
}


void K3b::ThemeOptionTab::slotInstallTheme()
{
    KUrl themeURL = KUrlRequesterDialog::getUrl( QString(), this,
                                                 i18n("Drag or Type Theme URL") );

    if( themeURL.url().isEmpty() )
        return;

    QString themeTmpFile;
    // themeTmpFile contains the name of the downloaded file

    if( !KIO::NetAccess::download( themeURL, themeTmpFile, this ) ) {
        QString sorryText;
        QString tmpArg = themeURL.prettyUrl();
        if (themeURL.isLocalFile())
            sorryText = i18n("Unable to find the icon theme archive %1.",tmpArg);
        else
            sorryText = i18n("Unable to download the icon theme archive.\n"
                             "Please check that address %1 is correct.",tmpArg);
        KMessageBox::sorry( this, sorryText );
        return;
    }

    // check if the archive contains a dir with a k3b.theme file
    QString themeName;
    KTar archive( themeTmpFile );
    archive.open(QIODevice::ReadOnly);
    const KArchiveDirectory* themeDir = archive.directory();
    QStringList entries = themeDir->entries();
    bool validThemeArchive = false;
    if( entries.count() > 0 ) {
        if( themeDir->entry(entries.first())->isDirectory() ) {
            const KArchiveDirectory* subDir = dynamic_cast<const KArchiveDirectory*>( themeDir->entry(entries.first()) );
            themeName = subDir->name();
            if( subDir && subDir->entry( "k3b.theme" ) ) {
                validThemeArchive = true;

                // check for all nessessary pixmaps (this is a little evil hacking)
                for( int i = 0; i <= K3b::Theme::WELCOME_BG; ++i ) {
                    if( !subDir->entry( K3b::Theme::filenameForPixmapType( (K3b::Theme::PixmapType)i ) ) ) {
                        validThemeArchive = false;
                        break;
                    }
                }
            }
        }
    }

    if( !validThemeArchive ) {
        KMessageBox::error( this, i18n("The file is not a valid K3b theme archive.") );
    }
    else {
        QString themeBasePath = KStandardDirs::locateLocal( "data", "k3b/pics/" );

        // check if there already is a theme by that name
        if( !QFile::exists( themeBasePath + '/' + themeName ) ||
            KMessageBox::warningYesNo( this,
                                       i18n("A theme with the name '%1' already exists. Do you want to "
                                            "overwrite it?", themeName),
                                       i18n("Theme exists"),
                                       KStandardGuiItem::overwrite(),
                                       KStandardGuiItem::cancel() ) == KMessageBox::Yes ) {
            // install the theme
            archive.directory()->copyTo( themeBasePath );
        }
    }

    archive.close();
    KIO::NetAccess::removeTempFile(themeTmpFile);

    readSettings();
}


void K3b::ThemeOptionTab::slotRemoveTheme()
{
    QModelIndex index = m_viewTheme->currentIndex();
    if( Theme* theme = m_themeModel->themeForIndex( index ) ) {
        QString question=i18n("<qt>Are you sure you want to remove the "
                              "<strong>%1</strong> theme?<br>"
                              "<br>"
                              "This will delete the files installed by this theme.</qt>", theme->name() );

        if( KMessageBox::warningContinueCancel( this, question, i18n("Delete"), KStandardGuiItem::del() ) != KMessageBox::Continue )
            return;

        m_themeModel->removeRow( index.row() );

        // reread the themes (this will also set the default theme in case we delete the
        // selected one)
        readSettings();
    }
}

#include "k3bthemeoptiontab.moc"
