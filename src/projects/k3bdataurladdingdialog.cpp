/*
 *
 * Copyright (C) 2005-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C)      2010 Michal Malek <michalm@jabster.pl>
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

#include "k3bdataurladdingdialog.h"
#include "k3bencodingconverter.h"

#include "k3bdatadoc.h"
#include "k3bdiritem.h"
#include "k3bcore.h"
#include "k3bfileitem.h"
#include "k3bmultichoicedialog.h"
#include "k3bvalidators.h"
#include "k3bglobals.h"
#include "k3bisooptions.h"
#include "k3b.h"
#include "k3bapplication.h"
#include "k3biso9660.h"
#include "k3bdirsizejob.h"
#include "k3binteractiondialog.h"
#include "k3bthread.h"
#include "k3bsignalwaiter.h"
#include "k3bexternalbinmanager.h"

#include <KConfig>
#include <KSqueezedTextLabel>
#include <KIconLoader>
#include <KLocalizedString>
#include <KMessageBox>
#include <KStandardGuiItem>

#include <QDir>
#include <QFileInfo>
#include <QList>
#include <QUrl>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QProgressBar>
#include <QLabel>
#include <QLayout>
#include <QInputDialog>

#include <unistd.h>


K3b::DataUrlAddingDialog::DataUrlAddingDialog( const QList<QUrl>& urls, DirItem* dir, QWidget* parent )
    : DataUrlAddingDialog( dir, parent )
{
    m_urls = urls;
    for( QList<QUrl>::ConstIterator it = urls.begin(); it != urls.end(); ++it )
        m_urlQueue.append( qMakePair( K3b::convertToLocalUrl(*it), dir ) );
}


K3b::DataUrlAddingDialog::DataUrlAddingDialog( const QList<DataItem*>& items, DirItem* dir, bool copy, QWidget* parent )
    : DataUrlAddingDialog( dir, parent )
{
    m_infoLabel->setText( i18n("Moving files to project \"%1\"...", dir->getDoc()->URL().fileName()) );
    m_copyItems = copy;

    for( QList<K3b::DataItem*>::const_iterator it = items.begin(); it != items.end(); ++it ) {
        m_items.append( qMakePair( *it, dir ) );
        ++m_totalFiles;
        if( (*it)->isDir() ) {
            m_totalFiles += static_cast<K3b::DirItem*>( *it )->numFiles();
            m_totalFiles += static_cast<K3b::DirItem*>( *it )->numDirs();
        }
    }
}


K3b::DataUrlAddingDialog::DataUrlAddingDialog( DirItem* dir, QWidget* parent )
    : QDialog( parent),
      m_doc( dir->getDoc() ),
      m_bExistingItemsReplaceAll(false),
      m_bExistingItemsIgnoreAll(false),
      m_bFolderLinksFollowAll(false),
      m_bFolderLinksAddAll(false),
      m_iAddHiddenFiles(0),
      m_iAddSystemFiles(0),
      m_bCanceled(false),
      m_copyItems(false),
      m_totalFiles(0),
      m_filesHandled(0),
      m_lastProgress(0)
{
    m_encodingConverter = new K3b::EncodingConverter();

    setWindowTitle(i18n("Adding files to project '%1'",m_doc->URL().fileName()));
    setAttribute( Qt::WA_DeleteOnClose );
    QGridLayout* grid = new QGridLayout( this );
    grid->setContentsMargins( 0, 0, 0, 0 );

    m_counterLabel = new QLabel( this );
    m_infoLabel = new KSqueezedTextLabel( i18n("Adding files to project '%1'..."
                                               ,m_doc->URL().fileName()), this );
    m_progressWidget = new QProgressBar( this );

    QDialogButtonBox* buttonBox = new QDialogButtonBox( QDialogButtonBox::Cancel, this );
    connect( buttonBox, SIGNAL(rejected()), SLOT(reject()) );

    grid->addWidget( m_counterLabel, 0, 1 );
    grid->addWidget( m_infoLabel, 0, 0 );
    grid->addWidget( m_progressWidget, 1, 0, 1, 2 );
    grid->addWidget( buttonBox, 2, 0, 1, 2 );

    m_dirSizeJob = new K3b::DirSizeJob( this );
    connect( m_dirSizeJob, SIGNAL(finished(bool)),
             this, SLOT(slotDirSizeDone(bool)) );

    // try to start with a reasonable size
    resize( (int)( fontMetrics().width( windowTitle() ) * 1.5 ), sizeHint().height() );
}


K3b::DataUrlAddingDialog::~DataUrlAddingDialog()
{
    // make sure the dir size job is finished
    m_dirSizeJob->cancel();
    K3b::SignalWaiter::waitForJob( m_dirSizeJob );

    QString message = resultMessage();
    if( !message.isEmpty() )
        KMessageBox::detailedSorry( parentWidget(), i18n("Problems while adding files to the project."), message );

    delete m_encodingConverter;
}


void K3b::DataUrlAddingDialog::addUrls( const QList<QUrl>& urls,
                                     K3b::DirItem* dir,
                                     QWidget* parent )
{
    if( !urls.isEmpty() ) {
        auto *dlg = new DataUrlAddingDialog( urls, dir, parent );
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        QMetaObject::invokeMethod( dlg, "slotStartAddUrls", Qt::QueuedConnection );
    }
}


void K3b::DataUrlAddingDialog::moveItems( const QList<K3b::DataItem*>& items,
                                       K3b::DirItem* dir,
                                       QWidget* parent )
{
    if( !items.isEmpty() ) {
        auto *dlg = new DataUrlAddingDialog( items, dir, false, parent );
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        QMetaObject::invokeMethod( dlg, "slotStartCopyMoveItems", Qt::QueuedConnection );
    }
}


void K3b::DataUrlAddingDialog::copyItems( const QList<K3b::DataItem*>& items,
                                       K3b::DirItem* dir,
                                       QWidget* parent )
{
    if( !items.isEmpty() ) {
        auto *dlg = new DataUrlAddingDialog( items, dir, true, parent );
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        QMetaObject::invokeMethod( dlg, "slotStartCopyMoveItems", Qt::QueuedConnection );
    }
}


void K3b::DataUrlAddingDialog::slotStartAddUrls()
{
    //
    // A common mistake by beginners is to try to burn an iso image
    // with a data project. Let's warn them
    //
    if( m_urls.count() == 1 ) {
        K3b::Iso9660 isoF( m_urls.first().toLocalFile() );
        if( isoF.open() ) {
            if( KMessageBox::warningYesNo( parentWidget(),
                                           i18n("<p>The file you are about to add to the project is an ISO 9660 image. As such "
                                                "it can be burned to a medium directly since it already contains a file "
                                                "system.<br>"
                                                "Are you sure you want to add this file to the project?"),
                                           i18n("Adding image file to project"),
                                           KGuiItem(i18n("Add the file to the project"),"list-add"),
                                           KGuiItem(i18n("Burn the image directly"),"tools-media-optical-burn") ) == KMessageBox::No ) {
                k3bappcore->k3bMainWindow()->slotWriteImage( m_urls.first() );
                reject();
                return;
            }
        }
    }

    slotAddUrls();
    if( !m_urlQueue.isEmpty() ) {
        m_dirSizeJob->setUrls( m_urls );
        m_dirSizeJob->setFollowSymlinks( m_doc->isoOptions().followSymbolicLinks() );
        m_dirSizeJob->start();
        exec();
    }
}


void K3b::DataUrlAddingDialog::slotStartCopyMoveItems()
{
    slotCopyMoveItems();
    if( !m_items.isEmpty() ) {
        m_progressWidget->setMaximum( m_totalFiles );
        exec();
    }
}


void K3b::DataUrlAddingDialog::slotAddUrls()
{
    if( m_bCanceled )
        return;

    // add next url
    QUrl url = m_urlQueue.first().first;
    K3b::DirItem* dir = m_urlQueue.first().second;
    m_urlQueue.erase( m_urlQueue.begin() );
    //
    // HINT:
    // we only use QFileInfo::absoluteFilePath() and QFileInfo::isHidden()
    // both do not cause QFileInfo to stat, thus no speed improvement
    // can come from removing QFileInfo usage here.
    //
    QFileInfo info(url.toLocalFile());
    QString absoluteFilePath( info.absoluteFilePath() );
    QString resolved( absoluteFilePath );

    bool valid = true;
    k3b_struct_stat statBuf, resolvedStatBuf;
    bool isSymLink = false;
    bool isDir = false;
    bool isFile = false;

    ++m_filesHandled;

    m_infoLabel->setText( url.toLocalFile() );
    if( m_totalFiles == 0 )
        m_counterLabel->setText( QString("(%1)").arg(m_filesHandled) );
    else
        m_counterLabel->setText( QString("(%1/%2)").arg(m_filesHandled).arg(m_totalFiles) );

    //
    // 1. Check if we want and can add the url
    //

    if( !url.isLocalFile() ) {
        valid = false;
        m_nonLocalFiles.append( url.toLocalFile() );
    }

    else if( k3b_lstat( QFile::encodeName(absoluteFilePath), &statBuf ) != 0 ) {
        valid = false;
        m_notFoundFiles.append( url.toLocalFile() );
    }

    else if( !m_encodingConverter->encodedLocally( QFile::encodeName( url.toLocalFile() ) ) ) {
        valid = false;
        m_invalidFilenameEncodingFiles.append( url.toLocalFile() );
    }

    else {
        isSymLink = S_ISLNK(statBuf.st_mode);
        isFile = S_ISREG(statBuf.st_mode);
        isDir = S_ISDIR(statBuf.st_mode);

        // symlinks are always readable and can always be added to a project
        // but we need to know if the symlink points to a directory
        if( isSymLink ) {
            resolved = K3b::resolveLink( absoluteFilePath );
            k3b_stat( QFile::encodeName(resolved), &resolvedStatBuf );
            isDir = S_ISDIR(resolvedStatBuf.st_mode);
        }

        else {
            if( ::access( QFile::encodeName( absoluteFilePath ), R_OK ) != 0 ) {
                valid = false;
                m_unreadableFiles.append( url.toLocalFile() );
            }
            else if( isFile && (unsigned long long)statBuf.st_size >= 0xFFFFFFFFULL ) {
                const K3b::ExternalBin *mkisofsBin = k3bcore->externalBinManager()->binObject( "mkisofs" );
                if ( !mkisofsBin || !mkisofsBin->hasFeature( "no-4gb-limit" ) ) {
                    valid = false;
                    m_tooBigFiles.append( url.toLocalFile() );
                }
            }
        }

        // FIXME: if we do not add hidden dirs the progress gets messed up!

        //
        // check for hidden and system files
        //
        if( valid ) {
            if( info.isHidden() && !addHiddenFiles() )
                valid = false;
            if( S_ISCHR(statBuf.st_mode) ||
                S_ISBLK(statBuf.st_mode) ||
                S_ISFIFO(statBuf.st_mode) ||
                S_ISSOCK(statBuf.st_mode) )
                if( !addSystemFiles() )
                    valid = false;
            if( isSymLink )
                if( S_ISCHR(resolvedStatBuf.st_mode) ||
                    S_ISBLK(resolvedStatBuf.st_mode) ||
                    S_ISFIFO(resolvedStatBuf.st_mode) ||
                    S_ISSOCK(resolvedStatBuf.st_mode) )
                    if( !addSystemFiles() )
                        valid = false;
        }
    }


    //
    // 2. Handle the url
    //

    QString newName = url.fileName();

    // filenames cannot end in backslashes (mkisofs problem. See comments in k3bisoimager.cpp (escapeGraftPoint()))
    bool bsAtEnd = false;
    while (!newName.isEmpty() && newName[newName.length() - 1] == '\\') {
        newName.truncate( newName.length()-1 );
        bsAtEnd = true;
    }
    if( bsAtEnd )
        m_mkisofsLimitationRenamedFiles.append( url.toLocalFile() + " -> " + newName );

    // backup dummy name
    if( newName.isEmpty() )
        newName = '1';

    K3b::DirItem* newDirItem = 0;

    //
    // The source is valid. Now check if the project already contains a file with that name
    // and if so handle it properly
    //
    if( valid ) {
        if( K3b::DataItem* oldItem = dir->find( newName ) ) {
            //
            // reuse an existing dir
            //
            if( oldItem->isDir() && isDir )
                newDirItem = dynamic_cast<K3b::DirItem*>(oldItem);

            //
            // we cannot replace files in the old session with dirs and vice versa (I think)
            // files are handled in K3b::FileItem constructor and dirs handled above
            //
            else if( oldItem->isFromOldSession() &&
                     isDir != oldItem->isDir() ) {
                if( !getNewName( newName, dir, newName ) )
                    valid = false;
            }

            else if( m_bExistingItemsIgnoreAll )
                valid = false;

            else if( oldItem->localPath() == resolved ) {
                //
                // Just ignore if the same file is added again
                //
                valid = false;
            }

            else if( m_bExistingItemsReplaceAll ) {
                // if we replace an item from an old session the K3b::FileItem constructor takes care
                // of replacing the item
                if( !oldItem->isFromOldSession() )
                    delete oldItem;
            }

            //
            // Let the user choose
            //
            else {
                switch( K3b::MultiChoiceDialog::choose( i18n("File already exists"),
                                                      i18n("<p>File <em>%1</em> already exists in "
                                                           "project folder <em>%2</em>.",
                                                           newName,
                                                           QString( '/' + dir->k3bPath()) ),
                                                      QMessageBox::Warning,
                                                      this,
                                                      6,
                                                      KGuiItem( i18n("Replace"),
                                                                QString(),
                                                                i18n("Replace the existing file") ),
                                                      KGuiItem( i18n("Replace All"),
                                                                QString(),
                                                                i18n("Always replace existing files") ),
                                                      KGuiItem( i18n("Ignore"),
                                                                QString(),
                                                                i18n("Keep the existing file") ),
                                                      KGuiItem( i18n("Ignore All"),
                                                                QString(),
                                                                i18n("Always keep the existing file") ),
                                                      KGuiItem( i18n("Rename"),
                                                                QString(),
                                                                i18n("Rename the new file") ),
                                                      KStandardGuiItem::cancel() ) ) {
                case 2: // replace all
                    m_bExistingItemsReplaceAll = true;
                    // fallthrough
                case 1: // replace
                    // if we replace an item from an old session the K3b::FileItem constructor takes care
                    // of replacing the item
                    if( !oldItem->isFromOldSession() )
                        delete oldItem;
                    break;
                case 4: // ignore all
                    m_bExistingItemsIgnoreAll = true;
                    // fallthrough
                case 3: // ignore
                    valid = false;
                    break;
                case 5: // rename
                    if( !getNewName( newName, dir, newName ) )
                        valid = false;
                    break;
                case 6: // cancel
                    reject();
                    return;
                }
            }
        }
    }


    //
    // One more thing to warn the user about: We cannot follow links to folders since that
    // would change the doc. So we simply ask the user what to do with a link to a folder
    //
    if( valid ) {
        // let's see if this link starts a loop
        // that means if it points to some folder above this one
        // if so we cannot follow it anyway
        if( isDir && isSymLink && !absoluteFilePath.startsWith( resolved ) ) {
            bool followLink = dir->getDoc()->isoOptions().followSymbolicLinks() || m_bFolderLinksFollowAll;
            if( !followLink && !m_bFolderLinksAddAll ) {
                switch( K3b::MultiChoiceDialog::choose( i18n("Adding link to folder"),
                                                      i18n("<p>'%1' is a symbolic link to folder '%2'."
                                                           "<p>If you intend to make K3b follow symbolic links you should consider letting K3b do this now "
                                                           "since K3b will not be able to do so afterwards because symbolic links to folders inside a "
                                                           "K3b project cannot be resolved."
                                                           "<p><b>If you do not intend to enable the option <em>follow symbolic links</em> you may safely "
                                                           "ignore this warning and choose to add the link to the project.</b>",
                                                           absoluteFilePath,
                                                           resolved ),
                                                      QMessageBox::Warning,
                                                      this,
                                                      5,
                                                      KGuiItem(i18n("Follow link now")),
                                                      KGuiItem(i18n("Always follow links")),
                                                      KGuiItem(i18n("Add link to project")),
                                                      KGuiItem(i18n("Always add links")),
                                                      KStandardGuiItem::cancel())  ) {
                case 2:
                    m_bFolderLinksFollowAll = true;
                    // fall-through
                case 1:
                    followLink = true;
                    break;
                case 4:
                    m_bFolderLinksAddAll = true;
                    // fall-through
                case 3:
                    followLink = false;
                    break;
                case 5:
                    reject();
                    return;
                }
            }

            if( followLink ) {
                absoluteFilePath = resolved;
                isSymLink = false;

                // count the files in the followed dir
                if( m_dirSizeJob->active() )
                    m_dirSizeQueue.append( QUrl::fromLocalFile(absoluteFilePath) );
                else {
                    m_progressWidget->setMaximum( 0 );
                    m_dirSizeJob->setUrls( QList<QUrl>() << QUrl::fromLocalFile(absoluteFilePath) );
                    m_dirSizeJob->start();
                }
            }
        }
    }


    //
    // Project valid also (we overwrite or renamed)
    // now create the new item
    //
    if( valid ) {
        //
        // Set the volume id from the first added url
        // only if the doc was not changed yet
        //
        if( m_urls.count() == 1 &&
            !dir->getDoc()->isModified() &&
            !dir->getDoc()->isSaved() ) {
            dir->getDoc()->setVolumeID( K3b::removeFilenameExtension( newName ) );
        }

        if( isDir && !isSymLink ) {
            if( !newDirItem ) { // maybe we reuse an already existing dir
                newDirItem = new K3b::DirItem( newName );
                newDirItem->setLocalPath( url.toLocalFile() ); // HACK: see k3bdiritem.h
                dir->addDataItem( newDirItem );
            }

            QDir newDir( absoluteFilePath );
            foreach( const QString& dir, newDir.entryList( QDir::AllEntries|QDir::Hidden|QDir::System|QDir::NoDotAndDotDot ) ) {
                m_urlQueue.append( qMakePair( QUrl::fromLocalFile(absoluteFilePath + '/' + dir ), newDirItem ) );
            }
        }
        else {
            m_newItems[ dir ].append( new K3b::FileItem( &statBuf, &resolvedStatBuf, url.toLocalFile(), *dir->getDoc(), newName ) );
        }
    }

    if( m_urlQueue.isEmpty() ) {
        Q_FOREACH( DirItem* dir, m_newItems.keys() ) {
            dir->addDataItems( m_newItems[ dir ] );
        }
        m_dirSizeJob->cancel();
        m_progressWidget->setMaximum( 100 );
        accept();
    }
    else {
        updateProgress();
        QMetaObject::invokeMethod( this, "slotAddUrls", Qt::QueuedConnection );
    }
}


void K3b::DataUrlAddingDialog::slotCopyMoveItems()
{
    if( m_bCanceled )
        return;

    //
    // Pop first item from the item list
    //
    K3b::DataItem* item = m_items.first().first;
    K3b::DirItem* dir = m_items.first().second;
    m_items.erase( m_items.begin() );

    ++m_filesHandled;
    m_infoLabel->setText( item->k3bPath() );
    if( m_totalFiles == 0 )
        m_counterLabel->setText( QString("(%1)").arg(m_filesHandled) );
    else
        m_counterLabel->setText( QString("(%1/%2)").arg(m_filesHandled).arg(m_totalFiles) );


    if( dir == item->parent() ) {
        qDebug() << "(K3b::DataUrlAddingDialog) trying to move an item into its own parent dir.";
    }
    else if( dir == item ) {
        qDebug() << "(K3b::DataUrlAddingDialog) trying to move an item into itselft.";
    }
    else {
        //
        // Let's see if an item with that name alredy exists
        //
        if( K3b::DataItem* oldItem = dir->find( item->k3bName() ) ) {
            //
            // reuse an existing dir: move all child items into the old dir
            //
            if( oldItem->isDir() && item->isDir() ) {
                QList<K3b::DataItem*> const& cl = dynamic_cast<K3b::DirItem*>( item )->children();
                for( QList<K3b::DataItem*>::const_iterator it = cl.constBegin();
                     it != cl.constEnd(); ++it )
                    m_items.append( qMakePair( *it, dynamic_cast<K3b::DirItem*>( oldItem ) ) );

                // FIXME: we need to remove the old dir item
            }

            //
            // we cannot replace files in the old session with dirs and vice versa (I think)
            // files are handled in K3b::FileItem constructor and dirs handled above
            //
            else if( oldItem->isFromOldSession() &&
                     item->isDir() != oldItem->isDir() ) {
                QString newName;
                if( getNewName( newName, dir, newName ) ) {
                    if( m_copyItems )
                        item = item->copy();
                    item->setK3bName( newName );
                    dir->addDataItem( item );
                }
            }

            else if( m_bExistingItemsReplaceAll ) {
                //
                // if we replace an item from an old session K3b::DirItem::addDataItem takes care
                // of replacing the item
                //
                if( !oldItem->isFromOldSession() )
                    delete oldItem;
                if( m_copyItems )
                    item = item->copy();
                dir->addDataItem( item );
            }

            else if( !m_bExistingItemsIgnoreAll ) {
                switch( K3b::MultiChoiceDialog::choose( i18n("File already exists"),
                                                      i18n("<p>File <em>%1</em> already exists in "
                                                           "project folder <em>%2</em>.",
                                                      item->k3bName(),
                                                      '/' + dir->k3bPath() ),
                                                      QMessageBox::Warning,
                                                      this,
                                                      6,
                                                      KGuiItem( i18n("Replace"),
                                                                QString(),
                                                                i18n("Replace the existing file") ),
                                                      KGuiItem( i18n("Replace All"),
                                                                QString(),
                                                                i18n("Always replace existing files") ),
                                                      KGuiItem( i18n("Ignore"),
                                                                QString(),
                                                                i18n("Keep the existing file") ),
                                                      KGuiItem( i18n("Ignore All"),
                                                                QString(),
                                                                i18n("Always keep the existing file") ),
                                                      KGuiItem( i18n("Rename"),
                                                                QString(),
                                                                i18n("Rename the new file") ),
                                                      KStandardGuiItem::cancel() ) ) {
                case 2: // replace all
                    m_bExistingItemsReplaceAll = true;
                    // fallthrough
                case 1: // replace
                    //
                    // if we replace an item from an old session K3b::DirItem::addDataItem takes care
                    // of replacing the item
                    //
                    if( !oldItem->isFromOldSession() )
                        delete oldItem;
                    if( m_copyItems )
                        item = item->copy();
                    dir->addDataItem( item );
                    break;
                case 4: // ignore all
                    m_bExistingItemsIgnoreAll = true;
                    // fallthrough
                case 3: // ignore
                    // do nothing
                    break;
                case 5: {// rename
                    QString newName;
                    if( getNewName( newName, dir, newName ) ) {
                        if( m_copyItems )
                            item = item->copy();
                        item->setK3bName( newName );
                        dir->addDataItem( item );
                    }
                    break;
                }
                case 6: // cancel
                    reject();
                    return;
                }
            }
        }

        //
        // No old item with the same name
        //
        else {
            if( m_copyItems )
                item = item->copy();
            dir->addDataItem( item );
        }
    }

    if( m_items.isEmpty() ) {
        m_dirSizeJob->cancel();
        accept();
    }
    else {
        updateProgress();
        QMetaObject::invokeMethod( this, "slotCopyMoveItems", Qt::QueuedConnection );
    }
}


void K3b::DataUrlAddingDialog::reject()
{
    m_bCanceled = true;
    m_dirSizeJob->cancel();
    QDialog::reject();
}


void K3b::DataUrlAddingDialog::slotDirSizeDone( bool success )
{
    if( success ) {
        m_totalFiles += m_dirSizeJob->totalFiles() + m_dirSizeJob->totalDirs();
        if( m_dirSizeQueue.isEmpty() ) {
            m_progressWidget->setValue( 100 );
            updateProgress();
        }
        else {
            m_dirSizeJob->setUrls( QList<QUrl>() << m_dirSizeQueue.back() );
            m_dirSizeQueue.pop_back();
            m_dirSizeJob->start();
        }
    }
}


void K3b::DataUrlAddingDialog::updateProgress()
{
    if( m_totalFiles > 0 ) {
        unsigned int p = 100*m_filesHandled/m_totalFiles;
        if( p > m_lastProgress ) {
            m_lastProgress = p;
            m_progressWidget->setValue( p );
        }
    }
    else {
        // make sure the progress bar shows something
        m_progressWidget->setValue( m_filesHandled );
    }
}


bool K3b::DataUrlAddingDialog::getNewName( const QString& oldName, K3b::DirItem* dir, QString& newName )
{
    bool ok = true;
    newName = oldName;
    QValidator* validator = K3b::Validators::iso9660Validator( false, this );
    int pos;
    do {
        newName = QInputDialog::getText( this,
                                         i18n("Enter New Filename"),
                                         i18n("A file with that name already exists. Please enter a new name:"),
                                         QLineEdit::Normal,
                                         newName, &ok );

    } while( ok && validator->validate( newName, pos ) == QValidator::Acceptable && dir->find( newName ) );

    delete validator;

    return ok;
}


bool K3b::DataUrlAddingDialog::addHiddenFiles()
{
    if( m_iAddHiddenFiles == 0 ) {
        // FIXME: the isVisible() stuff makes the static addUrls method not return (same below)
        if( KMessageBox::questionYesNo( /*isVisible() ? */this/* : parentWidget()*/,
                                        i18n("Do you also want to add hidden files?"),
                                        i18n("Hidden Files"), KGuiItem(i18n("Add")), KGuiItem(i18n("Do Not Add")) ) == KMessageBox::Yes )
            m_iAddHiddenFiles = 1;
        else
            m_iAddHiddenFiles = -1;
    }

    return ( m_iAddHiddenFiles == 1 );
}


bool K3b::DataUrlAddingDialog::addSystemFiles()
{
    if( m_iAddSystemFiles == 0 ) {
        if( KMessageBox::questionYesNo( /*isVisible() ? */this/* : parentWidget()*/,
                                        i18n("Do you also want to add system files "
                                             "(FIFOs, sockets, device files, and broken symlinks)?"),
                                        i18n("System Files"), KGuiItem(i18n("Add")), KGuiItem(i18n("Do Not Add")) ) == KMessageBox::Yes )
            m_iAddSystemFiles = 1;
        else
            m_iAddSystemFiles = -1;
    }

    return ( m_iAddSystemFiles == 1 );
}


QString K3b::DataUrlAddingDialog::resultMessage() const
{
    QString message;
    if( !m_unreadableFiles.isEmpty() )
        message += QString("<p><b>%1:</b><br>%2")
                   .arg( i18n("Insufficient permissions to read the following files") )
                   .arg( m_unreadableFiles.join( "<br>" ) );
    if( !m_notFoundFiles.isEmpty() )
        message += QString("<p><b>%1:</b><br>%2")
                   .arg( i18n("Unable to find the following files") )
                   .arg( m_notFoundFiles.join( "<br>" ) );
    if( !m_nonLocalFiles.isEmpty() )
        message += QString("<p><b>%1:</b><br>%2")
                   .arg( i18n("No non-local files supported") )
                   .arg( m_unreadableFiles.join( "<br>" ) );
    if( !m_tooBigFiles.isEmpty() )
        message += QString("<p><b>%1:</b><br>%2")
                   .arg( i18n("To burn files bigger than %1 please use %2",KIO::convertSize(0xFFFFFFFF),
                              QString("mkisofs >= 2.01.01a33 / genisoimage >= 1.1.4") ) )
                   .arg( m_tooBigFiles.join( "<br>" ) );
    if( !m_mkisofsLimitationRenamedFiles.isEmpty() )
        message += QString("<p><b>%1:</b><br>%2")
                   .arg( i18n("Some filenames had to be modified due to limitations in mkisofs") )
                   .arg( m_mkisofsLimitationRenamedFiles.join( "<br>" ) );
    if( !m_invalidFilenameEncodingFiles.isEmpty() )
        message += QString("<p><b>%1:</b><br>%2")
                   .arg( i18n("The following filenames have an invalid encoding. You may fix this "
                              "with the convmv tool") )
                   .arg( m_invalidFilenameEncodingFiles.join( "<br>" ) );

    return message;
}


