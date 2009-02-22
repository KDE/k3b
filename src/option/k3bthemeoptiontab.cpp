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


#include "k3bthemeoptiontab.h"

#include "k3bthememanager.h"

#include <k3bapplication.h>
#include <klocale.h>
#include <kconfig.h>
#include <kmessagebox.h>
#include <kurlrequester.h>
#include <k3listview.h>
#include <kio/global.h>
#include <kio/netaccess.h>
#include <kio/deletejob.h>
#include <kstandarddirs.h>
#include <ktar.h>
#include <kurlrequesterdlg.h>
#include <KGlobalSettings>

#include <qlabel.h>
#include <qfile.h>
#include <qfileinfo.h>


class ThemeViewItem : public K3ListViewItem
{
public:
    ThemeViewItem( K3b::Theme* theme_, Q3ListView* parent, Q3ListViewItem* after )
        : K3ListViewItem( parent, after ),
          theme(theme_) {
        setText( 0, theme->name() );
        setText( 1, theme->author() );
        setText( 2, theme->version() );
        setText( 3, theme->comment() );
    }

    K3b::Theme* theme;
};

K3b::ThemeOptionTab::ThemeOptionTab( QWidget* parent )
    : QWidget( parent )
{
    setupUi( this );

    m_centerPreviewLabel->setAutoFillBackground( true );
    m_leftPreviewLabel->setAutoFillBackground( true );
    m_rightPreviewLabel->setAutoFillBackground( true );

    m_viewTheme->setShadeSortColumn( false );

    connect( m_viewTheme, SIGNAL(selectionChanged()),
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
    m_viewTheme->clear();

    k3bappcore->themeManager()->loadThemes();

    QList<K3b::Theme*> themes = k3bappcore->themeManager()->themes();
    for( QList<K3b::Theme*>::const_iterator it = themes.constBegin(); it != themes.constEnd(); ++it ) {
        K3b::Theme* theme = *it;
        ThemeViewItem* item = new ThemeViewItem( theme, m_viewTheme, m_viewTheme->lastItem() );
        if( theme == k3bappcore->themeManager()->currentTheme() )
            m_viewTheme->setSelected( item, true );
    }
}


bool K3b::ThemeOptionTab::saveSettings()
{
    ThemeViewItem* item = (ThemeViewItem*)m_viewTheme->selectedItem();
    if( item )
        k3bappcore->themeManager()->setCurrentTheme( item->theme );

    return true;
}


void K3b::ThemeOptionTab::selectionChanged()
{
    ThemeViewItem* item = (ThemeViewItem*)m_viewTheme->selectedItem();
    if( item ) {
        m_centerPreviewLabel->setText( i18n("K3b - The CD/DVD Kreator") );

        QPalette pal( palette() );
        pal.setColor( backgroundRole(), item->theme->backgroundColor() );
        pal.setColor( foregroundRole(), item->theme->backgroundColor() );
        m_centerPreviewLabel->setPalette( pal );
        m_leftPreviewLabel->setPalette( pal );
        m_rightPreviewLabel->setPalette( pal );

        m_leftPreviewLabel->setPixmap( item->theme->pixmap( K3b::Theme::PROJECT_LEFT ) );
        m_rightPreviewLabel->setPixmap( item->theme->pixmap( K3b::Theme::PROJECT_RIGHT ) );

        m_buttonRemoveTheme->setEnabled( item->theme->local() );
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
                                       KGuiItem( i18n("Overwrite") ),
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
    ThemeViewItem* item = (ThemeViewItem*)m_viewTheme->selectedItem();
    if( item ) {
        QString question=i18n("<qt>Are you sure you want to remove the "
                              "<strong>%1</strong> icon theme?<br>"
                              "<br>"
                              "This will delete the files installed by this theme.</qt>",item->text(0));

        if( KMessageBox::warningContinueCancel( this, question, i18n("Delete") ) != KMessageBox::Continue )
            return;

        K3b::Theme* theme = item->theme;
        delete item;
        QString path = theme->path();

        // delete k3b.theme file to avoid it to get loaded
        QFile::remove( path + "/k3b.theme" );

        // reread the themes (this will also set the default theme in case we delete the
        // selected one)
        readSettings();

        // delete the theme data itself
        KIO::del( path, KIO::HideProgressInfo );
    }
}

#include "k3bthemeoptiontab.moc"
