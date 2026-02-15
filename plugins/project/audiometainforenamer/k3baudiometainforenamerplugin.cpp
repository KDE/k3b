/*
    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3baudiometainforenamerplugin.h"

#include <config-k3b.h>

// the k3b stuff we need
#include "k3bcore.h"
#include "k3bdatadoc.h"
#include "k3bdiritem.h"
#include "k3bfileitem.h"
#include "k3bmixeddoc.h"
#include "k3bplugin_i18n.h"

#include <KConfig>
#include <KMessageBox>

#include <QDebug>
#include <QFile>
#include <QMimeDatabase>
#include <QMimeType>
#include <QPair>
#include <QLatin1String>
#include <QHash>
#include <QString>
#include <QIcon>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QRadioButton>
#include <QToolTip>
#include <QTreeWidget>
#include <QVBoxLayout>

// Taglib includes
#include <tag.h>
#include <fileref.h>
#include <audioproperties.h>
#include <mpegfile.h>
#include <vorbisfile.h>
#include <oggflacfile.h>


K_PLUGIN_CLASS_WITH_JSON(K3bAudioMetainfoRenamerPlugin , "k3baudiometainforenamerplugin.json")


namespace {
    class K3bMimeTypeResolver : public TagLib::FileRef::FileTypeResolver
    {
    public:
        // to make gcc shut up
        virtual ~K3bMimeTypeResolver() {}

        TagLib::File* createFile( TagLib::FileName fileName, bool, TagLib::AudioProperties::ReadStyle ) const override
        {
            QMimeType mimetype = m_mimeDataBase.mimeTypeForFile( QFile::decodeName( fileName ) );
            if ( mimetype.isValid() ) {
                if ( mimetype.name() == QLatin1String( "audio/mpeg" ) )
                    return new TagLib::MPEG::File(fileName);
                else if ( mimetype.name() == QLatin1String( "application/ogg" ) )
                    return new TagLib::Ogg::Vorbis::File(fileName);
                else if ( mimetype.name() == QLatin1String( "application/x-flac" ) )
                    return new TagLib::Ogg::FLAC::File(fileName);
            }

            return nullptr;
        }

    private:
        QMimeDatabase m_mimeDataBase;
    };
}


class K3bAudioMetainfoRenamerPluginWidget::Private
{
public:
    K3b::DataDoc* doc;
    QString pattern;

    QComboBox* comboPattern;
    QTreeWidget* viewFiles;
    //  KProgressDialog* progressDialog;
    QPushButton* scanButton;

    QList< QPair<K3b::FileItem*, QTreeWidgetItem*> > renamableItems;
    QHash<K3b::DirItem*, QTreeWidgetItem*> dirItemHash;

//   long long scannedSize;
//   int progressCounter;
};


K3bAudioMetainfoRenamerPluginWidget::K3bAudioMetainfoRenamerPluginWidget( K3b::DataDoc* doc,
                                                                          QWidget* parent )
    : QWidget( parent )
{
    d = new Private();
    d->doc = doc;
    //  d->progressDialog = 0;

    // pattern group
    QGroupBox* patternGroup = new QGroupBox( i18n("Rename Pattern"), this );
    QHBoxLayout* patternGroupLayout = new QHBoxLayout( patternGroup );

    d->comboPattern = new QComboBox( patternGroup );
    d->comboPattern->setEditable( true );

    d->scanButton = new QPushButton( i18n("Scan"), patternGroup );
    patternGroupLayout->addWidget( d->comboPattern );
    patternGroupLayout->addWidget( d->scanButton );

    // the files view
    QGroupBox* filesGroup = new QGroupBox( i18n("Found Files"), this );
    QHBoxLayout* filesGroupLayout = new QHBoxLayout( filesGroup );

    d->viewFiles = new QTreeWidget( filesGroup );
    d->viewFiles->setHeaderLabels( QStringList() << i18n("New Name") << i18n("Old Name") );
    // FIXME: how about a class that installs an event filter to paint the no item text?
//    d->viewFiles->setNoItemText( i18n("Please click the Scan button to search for renameable files.") );

    filesGroupLayout->addWidget( d->viewFiles );

    // layout
    QVBoxLayout* box = new QVBoxLayout( this );
    box->setContentsMargins( 0, 0, 0, 0 );

    box->addWidget( patternGroup );
    box->addWidget( filesGroup );

    connect( d->scanButton, SIGNAL(clicked()), this, SLOT(slotScanClicked()) );

    d->scanButton->setToolTip( i18n("Scan for renamable files") );
    d->comboPattern->setWhatsThis( i18n("<qt>This specifies how the files should be renamed. "
                                        "Currently only the special strings <em>%a</em> (Artist), "
                                        "<em>%n</em> (Track number), and <em>%t</em> (Title) "
                                        "are supported.") );

    TagLib::FileRef::addFileTypeResolver( new K3bMimeTypeResolver() );
}


K3bAudioMetainfoRenamerPluginWidget::~K3bAudioMetainfoRenamerPluginWidget()
{
    delete d;
}


QString K3bAudioMetainfoRenamerPluginWidget::title() const
{
    return i18n("Rename Audio Files");
}


QString K3bAudioMetainfoRenamerPluginWidget::subTitle() const
{
    return i18n("Based on meta info");
}


void K3bAudioMetainfoRenamerPluginWidget::readSettings( const KConfigGroup& grp )
{
    d->comboPattern->setEditText( grp.readEntry( "rename pattern", "%a - %t" ) );
}


void K3bAudioMetainfoRenamerPluginWidget::saveSettings( KConfigGroup grp )
{
    grp.writeEntry( "rename pattern", d->comboPattern->currentText() );
}


void K3bAudioMetainfoRenamerPluginWidget::slotScanClicked()
{
    d->pattern = d->comboPattern->currentText();
    if( d->pattern.isEmpty() ) {
        KMessageBox::error( this, i18n("Please specify a valid pattern.") );
    }
    else {
//     if( d->progressDialog == 0 ) {
//       d->progressDialog = new KProgressDialog( this, "scanning_progress",
// 					       i18n("Scanning..."),
// 					       i18n("Scanning for renameable files."),
// 					       true );
//       d->progressDialog->setAllowCancel(false);
//     }

        K3b::DirItem* dir = d->doc->root();

        // clear old searches
        d->viewFiles->clear();
        d->renamableItems.clear();
        d->dirItemHash.clear();
//     d->scannedSize = 0;
//     d->progressCounter = 0;

        // create root item
        QTreeWidgetItem* rootItem = new QTreeWidgetItem( d->viewFiles, QStringList() << QLatin1String( "/" ) );
        rootItem->setIcon( 0, QIcon::fromTheme( "folder" ) );

        //  d->progressDialog->show();
        scanDir( dir, rootItem );
        //    d->progressDialog->close();

        rootItem->setExpanded(true);

        if( d->renamableItems.isEmpty() )
            KMessageBox::error( this, i18n("No renameable files found.") );
    }
}


void K3bAudioMetainfoRenamerPluginWidget::scanDir( K3b::DirItem* dir, QTreeWidgetItem* viewRoot )
{
    qDebug() << "(K3bAudioMetainfoRenamerPluginWidget) scanning dir " << dir->k3bName();

    d->dirItemHash.insert( dir, viewRoot );

    foreach( K3b::DataItem* item, dir->children() ) {
        if( item->isFile() ) {
            if( item->isRenameable() ) {
                QString newName = createNewName( static_cast<K3b::FileItem *>(item) );
                if( !newName.isEmpty() ) {
                    QTreeWidgetItem* fileViewItem = new QTreeWidgetItem( viewRoot, QStringList() << newName << item->k3bName() );
                    fileViewItem->setCheckState( 0, Qt::Checked );
                    fileViewItem->setIcon( 0, QIcon::fromTheme( item->mimeType().iconName() ) );
                    d->renamableItems.append( qMakePair( static_cast<K3b::FileItem *>(item), fileViewItem ) );
                }
            }

//       d->scannedSize += item->k3bSize();
//       d->progressCounter++;
//       if( d->progressCounter > 50 ) {
// 	d->progressCounter = 0;
// 	d->progressDialog->progressBar()->setProgress( 100*d->scannedSize/d->doc->root()->k3bSize() );
// 	qApp->processEvents();
//       }
        }
        else if( item->isDir() ) {
            // create dir item
            K3b::DirItem* dirItem = static_cast<K3b::DirItem *>( item );
            if ( !dirItem->children().isEmpty() ) {
                QTreeWidgetItem* dirViewItem = new QTreeWidgetItem( viewRoot, QStringList() << item->k3bName() );
                dirViewItem->setIcon( 0, QIcon::fromTheme( "folder" ) );
                scanDir( dirItem, dirViewItem );
                dirViewItem->setExpanded(true);
            }
        }
    }
}


void K3bAudioMetainfoRenamerPluginWidget::activate()
{
    if( d->renamableItems.isEmpty() ) {
        KMessageBox::error( this, i18n("Please click the Scan button to search for renameable files.") );
    }
    else {
        for( QList< QPair<K3b::FileItem*, QTreeWidgetItem*> >::iterator it = d->renamableItems.begin();
             it != d->renamableItems.end(); ++it ) {
            QPair<K3b::FileItem*, QTreeWidgetItem*>& item = *it;

            if( item.second->checkState(0) == Qt::Checked )
                item.first->setK3bName( item.second->text(0) );
        }

        d->viewFiles->clear();
        d->renamableItems.clear();

        KMessageBox::information( this, i18n("Done.") );
    }
}


QString K3bAudioMetainfoRenamerPluginWidget::createNewName( K3b::FileItem* item )
{
    TagLib::FileRef file( QFile::encodeName( item->localPath() ).data() );

    if ( !file.isNull() && file.tag() ) {
        QString artist, title, track;
        artist = TStringToQString( file.tag()->artist() );
        title = TStringToQString( file.tag()->title() );
        if ( file.tag()->track() > 0 )
            track = QString::number( file.tag()->track() );

        QString newName;
        for( int i = 0; i < d->pattern.length(); ++i ) {

            if( d->pattern[i] == '%' ) {
                ++i;

                if( i < d->pattern.length() ) {
                    if( d->pattern[i] == 'a' ) {
                        if( artist.isEmpty() )
                            return QString();
                        newName.append(artist);
                    }
                    else if( d->pattern[i] == 'n' ) {
                        if (track.isEmpty())
                            return QString();
                        newName.append(track);
                    }
                    else if( d->pattern[i] == 't' ) {
                        if( title.isEmpty() )
                            return QString();
                        newName.append(title);
                    }
                    else {
                        newName.append( "%" );
                        newName.append( d->pattern[i] );
                    }
                }
                else {  // end of pattern
                    newName.append( "%" );
                }
            }
            else {
                newName.append( d->pattern[i] );
            }
        }

        // remove white spaces from end and beginning
        newName = newName.trimmed();

        QString extension = item->k3bName().mid( item->k3bName().lastIndexOf('.') );

        if( !newName.isEmpty() ) {
            //
            // Check if files with that name exists and if so append number
            //
            if( existsOtherItemWithSameName( item, newName + extension ) ) {
                qDebug() << "(K3bAudioMetainfoRenamerPluginWidget) file with name "
                         << newName << extension << " already exists" << Qt::endl;
                int i = 1;
                while( existsOtherItemWithSameName( item, newName + QString( " (%1)").arg(i) + extension ) )
                    i++;
                newName.append( QString( " (%1)").arg(i) );
            }

            // append extension
            newName.append( extension );
        }

        return newName;
    }
    else
        return QString();
}


bool K3bAudioMetainfoRenamerPluginWidget::existsOtherItemWithSameName( K3b::FileItem* item, const QString& name )
{
    K3b::DirItem* dir = item->parent();
    K3b::DataItem* otherItem = dir->find( name );
    if( otherItem && otherItem != item )
        return true;

    QTreeWidgetItem* dirViewItem = d->dirItemHash[dir];
    for ( int i = 0; i < dirViewItem->childCount(); ++i ) {
        QTreeWidgetItem* current = dirViewItem->child( i );
        if( current->text(0) == name )
            return true;
    }

    return false;
}



K3bAudioMetainfoRenamerPlugin::K3bAudioMetainfoRenamerPlugin( QObject* parent, const QVariantList& )
    : K3b::ProjectPlugin( K3b::Doc::DataProject, true, parent )
{
    setText( i18n("Rename Audio Files") );
    setToolTip( i18n("Rename audio files based on their meta info.") );
    setIcon( QIcon::fromTheme( "edit-rename" ) );
}


K3bAudioMetainfoRenamerPlugin::~K3bAudioMetainfoRenamerPlugin()
{
}


K3b::ProjectPluginGUIBase* K3bAudioMetainfoRenamerPlugin::createGUI( K3b::Doc* doc, QWidget* parent )
{
    if( K3b::DataDoc* dataDoc = dynamic_cast<K3b::DataDoc*>( doc ) )
        return new K3bAudioMetainfoRenamerPluginWidget( dataDoc, parent );
    else if( K3b::MixedDoc* mixedDoc = dynamic_cast<K3b::MixedDoc*>( doc ) )
        return new K3bAudioMetainfoRenamerPluginWidget( mixedDoc->dataDoc(), parent );
    else
        return nullptr;
}

#include "k3baudiometainforenamerplugin.moc"

#include "moc_k3baudiometainforenamerplugin.cpp"
