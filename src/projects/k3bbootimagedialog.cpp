/*
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bbootimagedialog.h"
#include "k3bbootimagemodel.h"

#include "k3bdatadoc.h"
#include "k3bbootitem.h"
#include "k3bintvalidator.h"

#include <KLocalizedString>
#include <KMessageBox>

#include <QDebug>
#include <QItemSelectionModel>
#include <QString>
#include <QCheckBox>
#include <QFileDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>




K3b::BootImageDialog::BootImageDialog( K3b::DataDoc* doc, QWidget* parent )
    : QDialog( parent ),
      m_doc( doc ),
      m_bootImageModel( new BootImageModel( m_doc, this ) )
{
    setWindowTitle(i18n("Boot Images"));
    setupUi( this );
    
    m_viewImages->setModel( m_bootImageModel );

    connect( m_buttonNew, SIGNAL(clicked()),
             this, SLOT(slotNewBootImage()) );
    connect( m_buttonDelete, SIGNAL(clicked()),
             this, SLOT(slotDeleteBootImage()) );
    connect( m_buttonToggleOptions, SIGNAL(clicked()),
             this, SLOT(slotToggleOptions()) );
    connect( m_viewImages->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
             this, SLOT(slotCurrentChanged(QModelIndex,QModelIndex)) );
    connect( m_radioNoEmulation, SIGNAL(toggled(bool)),
             this, SLOT(slotNoEmulationToggled(bool)) );
    connect( m_radioFloppy, SIGNAL(toggled(bool)),this,SLOT(slotOptionsChanged()) );
    connect( m_radioHarddisk, SIGNAL(toggled(bool)),this,SLOT(slotOptionsChanged()) );
    connect( m_checkNoBoot, SIGNAL(toggled(bool)),this,SLOT(slotOptionsChanged()) );
    connect( m_checkInfoTable, SIGNAL(toggled(bool)),this,SLOT(slotOptionsChanged()) );
    connect( m_radioNoEmulation, SIGNAL(toggled(bool)),this,SLOT(slotOptionsChanged()) );
    connect( m_editLoadSegment, SIGNAL(textChanged(QString)),this,SLOT(slotOptionsChanged()) );
    connect( m_editLoadSize, SIGNAL(textChanged(QString)),this,SLOT(slotOptionsChanged()) );
    connect( m_radioFloppy, SIGNAL(toggled(bool)),this,SLOT(slotOptionsChanged()) );

    K3b::IntValidator* v = new K3b::IntValidator( this );
    m_editLoadSegment->setValidator( v );
    m_editLoadSize->setValidator( v );

    showAdvancedOptions( false );
    loadBootItemSettings(0);
}

K3b::BootImageDialog::~BootImageDialog()
{
}


void K3b::BootImageDialog::slotToggleOptions()
{
    showAdvancedOptions( !m_groupOptions->isVisible() );
}


void K3b::BootImageDialog::showAdvancedOptions( bool show )
{
    if( show ) {
        m_groupOptions->show();
        m_buttonToggleOptions->setText( i18n("Hide Advanced Options") );
    }
    else {
        m_groupOptions->hide();
        m_buttonToggleOptions->setText( i18n("Show Advanced Options") );
    }
}


void K3b::BootImageDialog::slotNewBootImage()
{
    QString file = QFileDialog::getOpenFileName( this, i18n("Please Choose Boot Image") );
    if( !file.isEmpty() ) {
        KIO::filesize_t fsize = QFileInfo( file ).size();
        BootItem::ImageType boottype = K3b::BootItem::FLOPPY;
        if( fsize != 1200*1024 &&
            fsize != 1440*1024 &&
            fsize != 2880*1024 ) {
            switch( KMessageBox::warningYesNoCancel( this,
                                                     i18n("<p>The file you selected is not a floppy image (floppy images are "
                                                          "of size 1200 KB, 1440 KB, or 2880 KB). You may still use boot images "
                                                          "of other sizes by emulating a harddisk or disabling emulation completely. "
                                                          "<p>If you are not familiar with terms like 'harddisk emulation' you most "
                                                          "likely want to use a floppy image here. Floppy images can be created by "
                                                          "directly extracting them from a real floppy disk:"
                                                          "<pre>dd if=/dev/floppy of=/tmp/floppy.img</pre>"
                                                          "or by using one of the many boot floppy generators that can be found on "
                                                          "<a href=\"http://www.google.com/search?q=linux+boot+floppy&ie=UTF-8&oe=UTF-8\">the Internet</a>."),
                                                     i18n("No Floppy image selected"),
                                                     KGuiItem( i18n("Use harddisk emulation") ),
                                                     KGuiItem( i18n("Use no emulation") ),
                                                     KStandardGuiItem::cancel(),
                                                     QString(),
                                                     KMessageBox::AllowLink ) ) {
            case KMessageBox::Yes:
                boottype = K3b::BootItem::HARDDISK;
                break;
            case KMessageBox::No:
                boottype = K3b::BootItem::NONE;
                break;
            default:
                return;
            }
        }

        m_bootImageModel->createBootItem( file, boottype );
    }
}


void K3b::BootImageDialog::slotDeleteBootImage()
{
    QModelIndex index = m_viewImages->currentIndex();
    if( index.isValid() ) {
        m_bootImageModel->removeRow( index.row() );
    }
}


void K3b::BootImageDialog::slotCurrentChanged( const QModelIndex& current, const QModelIndex& /*previous*/ )
{
    loadBootItemSettings( m_bootImageModel->bootItemForIndex( current ) );
}


void K3b::BootImageDialog::loadBootItemSettings( K3b::BootItem* item )
{
    // this is needed to prevent the slots to change stuff
    m_loadingItem = true;

    if( item ) {
        m_groupOptions->setEnabled(true);
        m_groupImageType->setEnabled(true);

        m_checkNoBoot->setChecked( item->noBoot() );
        m_checkInfoTable->setChecked( item->bootInfoTable() );
        m_editLoadSegment->setText( "0x" + QString::number( item->loadSegment(), 16 ) );
        m_editLoadSize->setText( "0x" + QString::number( item->loadSize(), 16 ) );

        if( item->imageType() == K3b::BootItem::FLOPPY )
            m_radioFloppy->setChecked(true);
        else if( item->imageType() == K3b::BootItem::HARDDISK )
            m_radioHarddisk->setChecked(true);
        else
            m_radioNoEmulation->setChecked(true);

        // force floppy size
        KIO::filesize_t fsize = QFileInfo( item->localPath() ).size();
        m_radioFloppy->setDisabled( fsize != 1200*1024 &&
                                    fsize != 1440*1024 &&
                                    fsize != 2880*1024 );
    }
    else {
        m_groupOptions->setEnabled(false);
        m_groupImageType->setEnabled(false);
    }

    m_loadingItem = false;
}


void K3b::BootImageDialog::slotOptionsChanged()
{
    if( !m_loadingItem ) {
        QModelIndex index = m_viewImages->currentIndex();
        if( BootItem* item = m_bootImageModel->bootItemForIndex( index ) ) {
            item->setNoBoot( m_checkNoBoot->isChecked() );
            item->setBootInfoTable( m_checkInfoTable->isChecked() );

            // TODO: create some class K3b::IntEdit : public QLineEdit
            bool ok = true;
            item->setLoadSegment( K3b::IntValidator::toInt( m_editLoadSegment->text(), &ok ) );
            if( !ok )
                qDebug() << "(K3b::BootImageDialog) parsing number failed: " << m_editLoadSegment->text().toLower();
            item->setLoadSize( K3b::IntValidator::toInt( m_editLoadSize->text(), &ok ) );
            if( !ok )
                qDebug() << "(K3b::BootImageDialog) parsing number failed: " << m_editLoadSize->text().toLower();

            if( m_radioFloppy->isChecked() )
                m_bootImageModel->setImageType( index, BootItem::FLOPPY );
            else if( m_radioHarddisk->isChecked() )
                m_bootImageModel->setImageType( index, BootItem::HARDDISK );
            else
                m_bootImageModel->setImageType( index, BootItem::NONE );
        }
    }
}


void K3b::BootImageDialog::slotNoEmulationToggled( bool on )
{
    // it makes no sense to combine no emulation and no boot!
    // the base_widget takes care of the disabling
    if( on )
        m_checkNoBoot->setChecked(false);
}


