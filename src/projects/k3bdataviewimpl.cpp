/*
    SPDX-FileCopyrightText: 2003-2008 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2009 Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
    SPDX-FileCopyrightText: 2009-2011 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bdataviewimpl.h"
#include "k3bbootimagedialog.h"
#include "k3bdatadoc.h"
#include "k3bdatamultisessionimportdialog.h"
#include "k3bdataprojectdelegate.h"
#include "k3bdataprojectmodel.h"
#include "k3bdataprojectsortproxymodel.h"
#include "k3bdatapropertiesdialog.h"
#include "k3bdataurladdingdialog.h"
#include "k3bdiritem.h"
#include "k3bview.h"
#include "k3bviewcolumnadjuster.h"
#include "k3bvolumenamewidget.h"

#include <KLocalizedString>
#include <KFileItemDelegate>
#include <KRun>
#include <KActionCollection>

#include <QSortFilterProxyModel>
#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QInputDialog>
#include <QShortcut>
#include <QWidgetAction>


namespace {

class VolumeNameWidgetAction : public QWidgetAction
{
public:
    VolumeNameWidgetAction( K3b::DataDoc* doc, QObject* parent )
    :
        QWidgetAction( parent ),
        m_doc( doc )
    {
    }

protected:
    QWidget* createWidget( QWidget* parent ) override
    {
        return new K3b::VolumeNameWidget( m_doc, parent );
    }

private:
    K3b::DataDoc* m_doc;
};

} // namespace


K3b::DataViewImpl::DataViewImpl( View* view, DataDoc* doc, KActionCollection* actionCollection )
:
    QObject( view ),
    m_view( view ),
    m_doc( doc ),
    m_model( new DataProjectModel( doc, view ) ),
    m_sortModel( new DataProjectSortProxyModel( this ) ),
    m_fileView( new QTreeView( view ) )
{
    connect( m_doc, SIGNAL(importedSessionChanged(int)), this, SLOT(slotImportedSessionChanged(int)) );
    connect( m_model, SIGNAL(addUrlsRequested(QList<QUrl>,K3b::DirItem*)), SLOT(slotAddUrlsRequested(QList<QUrl>,K3b::DirItem*)) );
    connect( m_model, SIGNAL(moveItemsRequested(QList<K3b::DataItem*>,K3b::DirItem*)), SLOT(slotMoveItemsRequested(QList<K3b::DataItem*>,K3b::DirItem*)) );

    m_sortModel->setSourceModel( m_model );

    m_fileView->setItemDelegate( new DataProjectDelegate( this ) );
    m_fileView->setModel( m_sortModel );
    m_fileView->setAcceptDrops( true );
    m_fileView->setDragEnabled( true );
    m_fileView->setDragDropMode( QTreeView::DragDrop );
    m_fileView->setItemsExpandable( false );
    m_fileView->setRootIsDecorated( false );
    m_fileView->setSelectionMode( QTreeView::ExtendedSelection );
    m_fileView->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
    m_fileView->setContextMenuPolicy( Qt::ActionsContextMenu );
    m_fileView->setSortingEnabled( true );
    m_fileView->sortByColumn( DataProjectModel::FilenameColumn, Qt::AscendingOrder );
    m_fileView->setMouseTracking( true );
    m_fileView->setAllColumnsShowFocus( true );
    connect( m_fileView, SIGNAL(doubleClicked(QModelIndex)),
             this, SLOT(slotItemActivated(QModelIndex)) );
    connect( m_fileView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
             this, SLOT(slotSelectionChanged()) );

    m_columnAdjuster = new ViewColumnAdjuster( this );
    m_columnAdjuster->setView( m_fileView );
    m_columnAdjuster->addFixedColumn( DataProjectModel::TypeColumn );
    m_columnAdjuster->setColumnMargin( DataProjectModel::TypeColumn, 10 );
    m_columnAdjuster->addFixedColumn( DataProjectModel::SizeColumn );
    m_columnAdjuster->setColumnMargin( DataProjectModel::SizeColumn, 10 );

    m_actionNewDir = new QAction( QIcon::fromTheme( "folder-new" ), i18n("New Folder..."), m_fileView );
    m_actionNewDir->setShortcut( Qt::CTRL + Qt::Key_N );
    m_actionNewDir->setShortcutContext( Qt::WidgetShortcut );
    actionCollection->addAction( "new_dir", m_actionNewDir );
    connect( m_actionNewDir, SIGNAL(triggered(bool)), this, SLOT(slotNewDir()) );

    m_actionRemove = new QAction( QIcon::fromTheme( "edit-delete" ), i18n("Remove"), m_fileView );
    m_actionRemove->setShortcut( Qt::Key_Delete );
    m_actionRemove->setShortcutContext( Qt::WidgetShortcut );
    actionCollection->addAction( "remove", m_actionRemove );
    connect( m_actionRemove, SIGNAL(triggered(bool)), this, SLOT(slotRemove()) );

    m_actionRename = new QAction( QIcon::fromTheme( "edit-rename" ), i18n("Rename"), m_fileView );
    m_actionRename->setShortcut( Qt::Key_F2 );
    m_actionRename->setShortcutContext( Qt::WidgetShortcut );
    actionCollection->addAction( "rename", m_actionRename );
    connect( m_actionRename, SIGNAL(triggered(bool)), this, SLOT(slotRename()) );

    m_actionParentDir = new QAction( QIcon::fromTheme( "go-up" ), i18n("Parent Folder"), m_fileView );
    m_actionParentDir->setShortcut( Qt::Key_Backspace );
    m_actionParentDir->setShortcutContext( Qt::WidgetShortcut );
    actionCollection->addAction( "parent_dir", m_actionParentDir );

    m_actionProperties = new QAction( QIcon::fromTheme( "document-properties" ), i18n("Properties"), m_fileView );
    m_actionProperties->setShortcut( Qt::ALT + Qt::Key_Return );
    m_actionProperties->setShortcutContext( Qt::WidgetShortcut );
    actionCollection->addAction( "properties", m_actionProperties );
    connect( m_actionProperties, SIGNAL(triggered(bool)), this, SLOT(slotProperties()) );

    m_actionOpen = new QAction( QIcon::fromTheme( "document-open" ), i18n("Open"), m_view );
    actionCollection->addAction( "open", m_actionOpen );
    connect( m_actionOpen, SIGNAL(triggered(bool)), this, SLOT(slotOpen()) );

    m_actionImportSession = new QAction( QIcon::fromTheme( "document-import" ), i18n("&Import Session..."), m_view );
    m_actionImportSession->setToolTip( i18n("Import a previously burned session into the current project") );
    actionCollection->addAction( "project_data_import_session", m_actionImportSession );
    connect( m_actionImportSession, SIGNAL(triggered(bool)), this, SLOT(slotImportSession()) );

    m_actionClearSession = new QAction( QIcon::fromTheme( "edit-clear" ), i18n("&Clear Imported Session"), m_view );
    m_actionClearSession->setToolTip( i18n("Remove the imported items from a previous session") );
    m_actionClearSession->setEnabled( m_doc->importedSession() > -1 );
    actionCollection->addAction( "project_data_clear_imported_session", m_actionClearSession );
    connect( m_actionClearSession, SIGNAL(triggered(bool)), this, SLOT(slotClearImportedSession()) );

    m_actionEditBootImages = new QAction( QIcon::fromTheme( "document-properties" ), i18n("&Edit Boot Images..."), m_view );
    m_actionEditBootImages->setToolTip( i18n("Modify the bootable settings of the current project") );
    actionCollection->addAction( "project_data_edit_boot_images", m_actionEditBootImages );
    connect( m_actionEditBootImages, SIGNAL(triggered(bool)), this, SLOT(slotEditBootImages()) );

    QWidgetAction* volumeNameWidgetAction = new VolumeNameWidgetAction( m_doc, this );
    actionCollection->addAction( "project_volume_name", volumeNameWidgetAction );

    QShortcut* enterShortcut = new QShortcut( QKeySequence( Qt::Key_Return ), m_fileView );
    enterShortcut->setContext( Qt::WidgetShortcut );
    connect( enterShortcut, SIGNAL(activated()), this, SLOT(slotEnterPressed()) );

    // Create data context menu
    QAction* separator = new QAction( this );
    separator->setSeparator( true );
    m_fileView->addAction( m_actionParentDir );
    m_fileView->addAction( separator );
    m_fileView->addAction( m_actionRename );
    m_fileView->addAction( m_actionRemove );
    m_fileView->addAction( m_actionNewDir );
    m_fileView->addAction( separator );
    m_fileView->addAction( m_actionOpen );
    m_fileView->addAction( separator );
    m_fileView->addAction( m_actionProperties );
    m_fileView->addAction( separator );
    m_fileView->addAction( actionCollection->action("project_burn") );
}


void K3b::DataViewImpl::addUrls( const QModelIndex& parent, const QList<QUrl>& urls )
{
    DirItem *item = dynamic_cast<DirItem*>( m_model->itemForIndex( parent ) );
    if (!item)
        item = m_doc->root();

    DataUrlAddingDialog::addUrls( urls, item, m_view );
}


void K3b::DataViewImpl::slotCurrentRootChanged( const QModelIndex& newRoot )
{
    // make the file view show only the child nodes of the currently selected
    // directory from dir view
    m_fileView->setRootIndex( m_sortModel->mapFromSource( newRoot ) );
    m_columnAdjuster->adjustColumns();
    m_actionParentDir->setEnabled( newRoot.isValid() && m_model->indexForItem( m_doc->root() ) != newRoot );
}


void K3b::DataViewImpl::slotNewDir()
{
    const QModelIndex parent = m_sortModel->mapToSource( m_fileView->rootIndex() );
    DirItem* parentDir = 0;

    if (parent.isValid())
        parentDir = dynamic_cast<DirItem*>( m_model->itemForIndex( parent ) );

    if (!parentDir)
        parentDir = m_doc->root();

    QString name;
    bool ok;

    name = QInputDialog::getText( m_view,
                                  i18n("New Folder"),
                                  i18n("Please insert the name for the new folder:"),
                                  QLineEdit::Normal,
                                  i18n("New Folder"),
                                  &ok );

    while( ok && DataDoc::nameAlreadyInDir( name, parentDir ) ) {
        name = QInputDialog::getText( m_view,
                                      i18n("New Folder"),
                                      i18n("A file with that name already exists. "
                                           "Please insert the name for the new folder:"),
                                      QLineEdit::Normal,
                                      i18n("New Folder"),
                                      &ok );
    }

    if( !ok )
        return;

    m_doc->addEmptyDir( name, parentDir );
}


void K3b::DataViewImpl::slotRemove()
{
    // Remove items directly from sort model to avoid unnecessary mapping of indexes
    const QItemSelection selection = m_fileView->selectionModel()->selection();
    const QModelIndex parentDirectory = m_fileView->rootIndex();
    for( int i = selection.size() - 1; i >= 0; --i ) {
        m_sortModel->removeRows( selection.at(i).top(), selection.at(i).height(), parentDirectory );
    }
}


void K3b::DataViewImpl::slotRename()
{
    const QModelIndex current = m_fileView->currentIndex();
    if( current.isValid() ) {
        m_fileView->edit( current );
    }
}


void K3b::DataViewImpl::slotProperties()
{
    const QModelIndexList indices = m_fileView->selectionModel()->selectedRows();
    if ( indices.isEmpty() )
    {
        // show project properties
        m_view->slotProperties();
    }
    else
    {
        QList<DataItem*> items;

        foreach(const QModelIndex& index, indices) {
            items.append( m_model->itemForIndex( m_sortModel->mapToSource( index ) ) );
        }

        DataPropertiesDialog dlg( items, m_view );
        dlg.exec();
    }
}


void K3b::DataViewImpl::slotOpen()
{
    const QModelIndex current = m_sortModel->mapToSource( m_fileView->currentIndex() );
    if( !current.isValid() )
        return;

    DataItem* item = m_model->itemForIndex( current );

    if( !item->isFile() ) {
        QUrl url = QUrl::fromLocalFile( item->localPath() );
        if( !KRun::isExecutableFile( url,
                                    item->mimeType().name() ) ) {
            KRun::runUrl( url,
                        item->mimeType().name(),
                        m_view,
                        KRun::RunFlags());
        }
        else {
            KRun::displayOpenWithDialog( QList<QUrl>() << url, m_view );
        }
    }
}


void K3b::DataViewImpl::slotSelectionChanged()
{
    const QModelIndexList indexes = m_fileView->selectionModel()->selectedRows();

    bool open = true, rename = true, remove = true;

    // we can only rename one item at a time
    // also, we can only create a new dir over a single directory
    if (indexes.count() > 1)
    {
        rename = false;
        open = false;
    }
    else if (indexes.count() == 1)
    {
        QModelIndex index = indexes.first();
        rename = (index.flags() & Qt::ItemIsEditable);
        open = (index.data(DataProjectModel::ItemTypeRole).toInt() == DataProjectModel::FileItemType);
    }
    else // selectedIndex.count() == 0
    {
        remove = false;
        rename = false;
        open = false;
    }

    // check if all selected items can be removed
    foreach(const QModelIndex &index, indexes)
    {
        if (!(index.data(DataProjectModel::CustomFlagsRole).toInt() & DataProjectModel::ItemIsRemovable))
        {
            remove = false;
            break;
        }
    }

    m_actionRename->setEnabled( rename );
    m_actionRemove->setEnabled( remove );
    m_actionOpen->setEnabled( open );
}


void K3b::DataViewImpl::slotItemActivated( const QModelIndex& index )
{
    if( index.isValid() ) {
        const int type = index.data( DataProjectModel::ItemTypeRole ).toInt();
        if( type == DataProjectModel::DirItemType ) {
            emit setCurrentRoot( m_sortModel->mapToSource( index ) );
        }
        else if( type == DataProjectModel::FileItemType ) {
            m_fileView->edit( index );
        }
    }
}


void K3b::DataViewImpl::slotEnterPressed()
{
    slotItemActivated( m_fileView->currentIndex() );
}


void K3b::DataViewImpl::slotImportSession()
{
    K3b::DataMultisessionImportDialog::importSession( m_doc, m_view );
}


void K3b::DataViewImpl::slotClearImportedSession()
{
    m_doc->clearImportedSession();
}


void K3b::DataViewImpl::slotEditBootImages()
{
    BootImageDialog dlg( m_doc );
    dlg.setWindowTitle( i18n("Edit Boot Images") );
    dlg.exec();
}


void K3b::DataViewImpl::slotImportedSessionChanged( int importedSession )
{
    m_actionClearSession->setEnabled( importedSession > -1 );
}


void K3b::DataViewImpl::slotAddUrlsRequested( QList<QUrl> urls, K3b::DirItem* targetDir )
{
    DataUrlAddingDialog::addUrls( urls, targetDir, m_view );
}


void K3b::DataViewImpl::slotMoveItemsRequested( QList<K3b::DataItem*> items, K3b::DirItem* targetDir )
{
    DataUrlAddingDialog::moveItems( items, targetDir, m_view );
}


