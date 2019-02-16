/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
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


#include "k3bdatapropertiesdialog.h"
#include "k3bdiritem.h"
#include "k3bfileitem.h"
#include "k3bvalidators.h"

#include <KLineEdit>
#include <KLocalizedString>
#include <KIconLoader>
#include <KIO/Global>
#include <KSqueezedTextLabel>

#include <QFileInfo>
#include <QUrl>
#include <QValidator>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QToolTip>


K3b::DataPropertiesDialog::DataPropertiesDialog( const QList<K3b::DataItem*>& dataItems, QWidget* parent )
    : QDialog( parent )
{
    setWindowTitle( i18n("File Properties") );

    m_dataItems = dataItems;

    m_labelIcon = new QLabel( this );
    if ( dataItems.count() == 1 ) {
        m_editName = new KLineEdit( this );
        m_editName->setValidator( K3b::Validators::iso9660Validator( false, this ) );
        m_labelType = new QLabel( this );
        m_labelLocalName = new KSqueezedTextLabel( this );
        m_labelLocalLocation = new KSqueezedTextLabel( this );
        m_labelLocalLinkTarget = new KSqueezedTextLabel( this );
        m_extraInfoLabel = new QLabel( this );
    }
    else {
        m_multiSelectionLabel = new QLabel( this );
    }

    m_labelLocation = new KSqueezedTextLabel( this );
    m_labelSize = new QLabel( this );
    m_labelBlocks = new QLabel( this );

    // layout
    // -----------------------------
    QGridLayout* grid = new QGridLayout( this );

    grid->addWidget( m_labelIcon, 0, 0 );
    if ( dataItems.count() == 1 ) {
        grid->addWidget( m_editName, 0, 2 );
    }
    else {
        grid->addWidget( m_multiSelectionLabel, 0, 2 );
    }
    int row = 1;

    m_spacerLine = new QFrame( this );
    m_spacerLine->setFrameStyle( QFrame::HLine | QFrame::Sunken );
    grid->addWidget( m_spacerLine, row, 0, 1, 3 );
    ++row;
    if ( dataItems.count() == 1 ) {
        grid->addWidget( new QLabel( i18n("Type:"), this ), row, 0 );
        grid->addWidget( m_labelType, row++, 2 );
        grid->addWidget( m_extraInfoLabel, row++, 2 );
    }
    grid->addWidget( new QLabel( i18n("Location:"), this ), row, 0 );
    grid->addWidget( m_labelLocation, row++, 2 );
    grid->addWidget( new QLabel( i18n("Size:"), this ), row, 0 );
    grid->addWidget( m_labelSize, row++, 2 );
    grid->addWidget( new QLabel( i18n("Used blocks:"), this ), row, 0 );
    grid->addWidget( m_labelBlocks, row++, 2 );

    m_spacerLine = new QFrame( this );
    m_spacerLine->setFrameStyle( QFrame::HLine | QFrame::Sunken );
    grid->addWidget( m_spacerLine, row, 0, 1, 3 );
    ++row;

    if ( dataItems.count() == 1 ) {
        m_labelLocalNameText = new QLabel( i18n("Local name:"), this );
        grid->addWidget( m_labelLocalNameText, row, 0 );
        grid->addWidget( m_labelLocalName, row++, 2 );
        m_labelLocalLocationText = new QLabel( i18n("Local location:"), this );
        grid->addWidget( m_labelLocalLocationText, row, 0 );
        grid->addWidget( m_labelLocalLocation, row++, 2 );
        m_labelLocalLinkTargetText = new QLabel( i18n("Local link target:"), this );
        grid->addWidget( m_labelLocalLinkTargetText, row, 0 );
        grid->addWidget( m_labelLocalLinkTarget, row++, 2 );
    }

    grid->addItem( new QSpacerItem( 50, 1, QSizePolicy::Fixed, QSizePolicy::Fixed ), 0, 1 );
    grid->setColumnStretch( 2, 1 );


    // OPTIONS
    // /////////////////////////////////////////////////
    QTabWidget* optionTab = new QTabWidget( this );
    m_spacerLine = new QFrame( this );
    m_spacerLine->setFrameStyle( QFrame::HLine | QFrame::Sunken );

    grid->addWidget( m_spacerLine, row++, 0, 1, 3 );
    grid->setRowStretch( row++, 1 );
    grid->addWidget( optionTab, row++, 0, 1, 3 );

    QWidget* hideBox = new QWidget( optionTab );
    QGridLayout* hideBoxGrid = new QGridLayout( hideBox );
    m_checkHideOnRockRidge = new QCheckBox( i18n("Hide on RockRidge"), hideBox );
    m_checkHideOnJoliet = new QCheckBox( i18n("Hide on Joliet"), hideBox );
    hideBoxGrid->addWidget( m_checkHideOnRockRidge, 0, 0 );
    hideBoxGrid->addWidget( m_checkHideOnJoliet, 1, 0 );
    hideBoxGrid->setRowStretch( 2, 1 );
//   grid->addMultiCellWidget( m_checkHideOnRockRidge, 10, 10, 0, 2 );
//   grid->addMultiCellWidget( m_checkHideOnJoliet, 11, 11, 0, 2 );

    QWidget* sortingBox = new QWidget( optionTab );
    QGridLayout* sortingBoxGrid = new QGridLayout( sortingBox );
    m_editSortWeight = new KLineEdit( sortingBox );
    m_editSortWeight->setValidator( new QIntValidator( -2147483647, 2147483647, m_editSortWeight ) );
    m_editSortWeight->setAlignment( Qt::AlignRight );
    sortingBoxGrid->addWidget( new QLabel( i18n("Sort weight:"), sortingBox ), 0, 0 );
    sortingBoxGrid->addWidget( m_editSortWeight, 0, 1 );
    sortingBoxGrid->setColumnStretch( 1, 1 );
    sortingBoxGrid->setRowStretch( 1, 1 );

    optionTab->addTab( hideBox, i18n("Settings") );
    optionTab->addTab( sortingBox, i18n("Advanced") );


    // load the data
    // ----------------------------
    if ( dataItems.count() == 1 ) {
        loadItemProperties( dataItems.first() );
    }
    else {
        loadListProperties( dataItems );
    }


    m_checkHideOnRockRidge->setToolTip( i18n("Hide this file in the RockRidge filesystem") );
    m_checkHideOnJoliet->setToolTip( i18n("Hide this file in the Joliet filesystem") );
    m_editSortWeight->setToolTip( i18n("Modify the physical sorting") );
    m_checkHideOnRockRidge->setWhatsThis( i18n("<p>If this option is checked, the file or folder "
                                               "(and its entire contents) will be hidden on the "
                                               "ISO 9660 and RockRidge filesystem.</p>"
                                               "<p>This is useful, for example, for having different README "
                                               "files for RockRidge and Joliet, which can be managed "
                                               "by hiding README.joliet on RockRidge and README.rr "
                                               "on the Joliet filesystem.</p>") );
    m_checkHideOnJoliet->setWhatsThis( i18n("<p>If this option is checked, the file or folder "
                                            "(and its entire contents) will be hidden on the "
                                            "Joliet filesystem.</p>"
                                            "<p>This is useful, for example, for having different README "
                                            "files for RockRidge and Joliet, which can be managed "
                                            "by hiding README.joliet on RockRidge and README.rr "
                                            "on the Joliet filesystem.</p>") );
    m_editSortWeight->setWhatsThis( i18n("<p>This value modifies the physical sort order of the files "
                                         "in the ISO 9660 filesystem. A higher weighting means that the "
                                         "file will be located closer to the beginning of the image "
                                         "(and the disk)."
                                         "<p>This option is useful in order to optimize the data layout "
                                         "on a medium."
                                         "<p><b>Caution:</b> This does not sort the order of the file "
                                         "names that appear in the ISO 9660 folder. "
                                         "It sorts the order in which the file data is "
                                         "written to the image.") );

    QDialogButtonBox* buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this );
    grid->addWidget( buttonBox, row++, 0, 1, 3 );
    connect( buttonBox, SIGNAL(accepted()), SLOT(accept()) );
    connect( buttonBox, SIGNAL(rejected()), SLOT(reject()) );
}


K3b::DataPropertiesDialog::~DataPropertiesDialog()
{
}


void K3b::DataPropertiesDialog::loadItemProperties( K3b::DataItem* dataItem )
{
    if( K3b::FileItem* fileItem = dynamic_cast<K3b::FileItem*>(dataItem) ) {
        QFileInfo fileInfo( fileItem->localPath() );
        qDebug() << fileItem->k3bPath() << fileItem->localPath();
        if( fileItem->isSymLink() ) {
            m_labelIcon->setPixmap( DesktopIcon( fileItem->mimeType().iconName(), KIconLoader::SizeLarge,
                                                 KIconLoader::DefaultState, QStringList() << "emblem-symbolic-link" ) );
            m_labelType->setText( i18n( "Link to %1", fileItem->mimeType().comment() ) );
            m_labelLocalLinkTarget->setText( fileItem->linkDest() );
        }
        else {
            m_labelIcon->setPixmap( DesktopIcon( fileItem->mimeType().iconName(), KIconLoader::SizeLarge ) );
            m_labelType->setText( fileItem->mimeType().comment() );
            m_labelLocalLinkTargetText->hide();
            m_labelLocalLinkTarget->hide();
        }
        m_labelLocalName->setText( fileInfo.fileName() );
        m_labelLocalLocation->setText( fileInfo.absolutePath() );
        m_labelSize->setText( KIO::convertSize(dataItem->size()) );
    }
    else if( K3b::DirItem* dirItem = dynamic_cast<K3b::DirItem*>(dataItem) ) {
        m_labelIcon->setPixmap( DesktopIcon( "folder", KIconLoader::SizeLarge ) );
        m_labelType->setText( i18n("Folder") );
        m_labelLocalNameText->hide();
        m_labelLocalName->hide();
        m_labelLocalLocationText->hide();
        m_labelLocalLocation->hide();
        m_labelLocalLinkTargetText->hide();
        m_labelLocalLinkTarget->hide();
        m_spacerLine->hide();
        m_labelSize->setText( KIO::convertSize(dataItem->size()) + "\n(" +
                              i18np("in one file", "in %1 files", dirItem->numFiles()) + ' ' +
                              i18np("and one folder", "and %1 folders", dirItem->numDirs()) + ')' );
    }
    else {
        m_labelIcon->setPixmap( DesktopIcon("unknown", KIconLoader::SizeLarge) );
        m_labelType->setText( i18n("Special file") );
        m_labelLocalNameText->hide();
        m_labelLocalName->hide();
        m_labelLocalLocationText->hide();
        m_labelLocalLocation->hide();
        m_labelLocalLinkTargetText->hide();
        m_labelLocalLinkTarget->hide();
        m_spacerLine->hide();
        m_labelSize->setText( KIO::convertSize(dataItem->size()) );
    }

    m_editName->setText( dataItem->k3bName() );
    m_labelBlocks->setText( QString::number(dataItem->blocks().lba()) );

    QString location = '/' + dataItem->k3bPath();
    if( location[location.length()-1] == '/' )
        location.truncate( location.length()-1 );
    location.truncate( location.lastIndexOf('/') );
    if( location.isEmpty() )
        location = '/';
    m_labelLocation->setText( location );
    m_extraInfoLabel->setText( QString( "(%1)" ).arg(dataItem->extraInfo()) );
    if( dataItem->extraInfo().isEmpty() )
        m_extraInfoLabel->hide();

    m_checkHideOnJoliet->setChecked( dataItem->hideOnJoliet() );
    m_checkHideOnRockRidge->setChecked( dataItem->hideOnRockRidge() );
    m_editSortWeight->setText( QString::number(dataItem->sortWeight()) );

    // if the parent is hidden the value cannot be changed (see K3b::DataItem::setHide...)
    if( dataItem->parent() ) {
        m_checkHideOnRockRidge->setDisabled( dataItem->parent()->hideOnRockRidge() );
        m_checkHideOnJoliet->setDisabled( dataItem->parent()->hideOnJoliet() );
    }

    if( !dataItem->isHideable() ) {
        m_checkHideOnJoliet->setDisabled(true);
        m_checkHideOnRockRidge->setDisabled(true);
        //    m_spacerLine->hide();
    }

    m_editName->setReadOnly( !dataItem->isRenameable() );
    m_editName->setFocus();
}


void K3b::DataPropertiesDialog::loadListProperties( const QList<K3b::DataItem*>& items )
{
    m_labelIcon->setPixmap( DesktopIcon( "document-multiple", KIconLoader::SizeLarge ) );

    int files = 0;
    int folders = 0;
    KIO::filesize_t size = 0;
    K3b::Msf blocks = 0;
    for ( QList<K3b::DataItem*>::iterator it = m_dataItems.begin();
          it != m_dataItems.end(); ++it ) {
        K3b::DataItem* item = *it;
        if ( item->isFile() )
            ++files;
        else if ( item->isDir() )
            ++folders;
        blocks += item->blocks();
        size += item->size();
    }
    QString s = i18np( "One Item", "%1 Items", items.count() );
    s += " - ";
    if ( files > 0 )
        s += i18np( "One File", "%1 Files", files );
    else
        s += i18n( "No Files" );
    s += " - ";
    if ( folders > 0 )
        s += i18np( "One Folder", "%1 Folders", folders );
    else
        s += i18n( "No Folders" );
    m_multiSelectionLabel->setText( s );

    m_labelSize->setText( KIO::convertSize(size) );
    m_labelBlocks->setText( QString::number(blocks.lba()) );

    // the location of all items are the same since it is not possible to
    // select items from different folders
    // FIXME: maybe better use QString::section?
    QString location = '/' + items.first()->k3bPath();
    if( location[location.length()-1] == '/' )
        location.truncate( location.length()-1 );
    location.truncate( location.lastIndexOf('/') );
    if( location.isEmpty() )
        location = '/';
    m_labelLocation->setText( location );


    m_checkHideOnJoliet->setChecked( items.first()->hideOnJoliet() );
    for ( QList<K3b::DataItem*>::iterator it = m_dataItems.begin();
          it != m_dataItems.end(); ++it ) {
        K3b::DataItem* item = *it;
        if ( m_checkHideOnJoliet->isChecked() != item->hideOnJoliet() ) {
            m_checkHideOnJoliet->setCheckState( Qt::PartiallyChecked );
            break;
        }
    }
    m_checkHideOnRockRidge->setChecked( items.first()->hideOnRockRidge() );
    for ( QList<K3b::DataItem*>::iterator it = m_dataItems.begin();
          it != m_dataItems.end(); ++it ) {
        K3b::DataItem* item = *it;
        if ( m_checkHideOnRockRidge->isChecked() != item->hideOnRockRidge() ) {
            m_checkHideOnRockRidge->setCheckState( Qt::PartiallyChecked );
            break;
        }
    }

    int weight = items.first()->sortWeight();
    for ( QList<K3b::DataItem*>::iterator it = m_dataItems.begin();
          it != m_dataItems.end(); ++it ) {
        K3b::DataItem* item = *it;
        if ( weight != item->sortWeight() ) {
            weight = 0;
            break;
        }
    }
    m_editSortWeight->setText( QString::number( weight ) );
}


void K3b::DataPropertiesDialog::accept()
{
    if ( m_dataItems.count() == 1 ) {
        m_dataItems.first()->setK3bName( m_editName->text() );
    }

    for ( QList<K3b::DataItem*>::iterator it = m_dataItems.begin();
          it != m_dataItems.end(); ++it ) {
        K3b::DataItem* item = *it;
        if ( m_checkHideOnRockRidge->checkState() != Qt::PartiallyChecked )
            item->setHideOnRockRidge( m_checkHideOnRockRidge->isChecked() );
        if ( m_checkHideOnJoliet->checkState() != Qt::PartiallyChecked )
            item->setHideOnJoliet( m_checkHideOnJoliet->isChecked() );
        if ( m_editSortWeight->isModified() )
            item->setSortWeight( m_editSortWeight->text().toInt() );
    }

    QDialog::accept();
}



