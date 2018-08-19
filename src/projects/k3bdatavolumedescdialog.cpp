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

#include "k3bdatavolumedescdialog.h"

#include "k3bisooptions.h"
#include "k3bvalidators.h"

#include <KLocalizedString>

#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QToolButton>


K3b::DataVolumeDescDialog::DataVolumeDescDialog( QWidget* parent )
    : QDialog( parent)
{
    setupUi( this );

    setWindowTitle( i18n("Volume Descriptor") );
    setModal( true );

    // the maximal number of characters that can be inserted are set in the ui file!

    QValidator* isoValidator = new K3b::Latin1Validator( this );

    m_editVolumeName->setValidator( isoValidator );
    m_editVolumeSetName->setValidator( isoValidator );
    m_editPublisher->setValidator( isoValidator );
    m_editPreparer->setValidator( isoValidator );
    m_editSystem->setValidator( isoValidator );
    m_editApplication->setValidator( isoValidator );

    connect( m_spinVolumeSetSize, SIGNAL(valueChanged(int)),
             this, SLOT(slotVolumeSetSizeChanged(int)) );

    // for now we hide the volume set size stuff since it's not working anymore in mkisofs 2.01a34
    textLabel1->hide();
    textLabel2->hide();
    m_spinVolumeSetSize->hide();
    m_spinVolumeSetNumber->hide();

    // FIXME: show the buttons and allow the selection of a file from the project
    m_buttonFindAbstract->hide();
    m_buttonFindCopyright->hide();
    m_buttonFindBiblio->hide();

    // give ourselves a reasonable size
    QSize s = sizeHint();
    s.setWidth( qMax(s.width(), 300) );
    resize( s );
}


K3b::DataVolumeDescDialog::~DataVolumeDescDialog()
{
}


void K3b::DataVolumeDescDialog::load( const K3b::IsoOptions& o )
{
    m_editVolumeName->setText( o.volumeID() );
    m_editVolumeSetName->setText( o.volumeSetId() );
    m_spinVolumeSetSize->setValue( o.volumeSetSize() );
    m_spinVolumeSetNumber->setValue( o.volumeSetNumber() );
    m_editPublisher->setText( o.publisher() );
    m_editPreparer->setText( o.preparer() );
    m_editSystem->setText( o.systemId() );
    m_editApplication->setText( o.applicationID() );
}


void K3b::DataVolumeDescDialog::save( K3b::IsoOptions& o )
{
    o.setVolumeID( m_editVolumeName->text() );
    o.setVolumeSetId( m_editVolumeSetName->text() );
    o.setVolumeSetSize( 1/*m_spinVolumeSetSize->value() */);
    o.setVolumeSetNumber( 1/*m_spinVolumeSetNumber->value() */);
    o.setPublisher( m_editPublisher->text() );
    o.setPreparer( m_editPreparer->text() );
    o.setSystemId( m_editSystem->text() );
    o.setApplicationID( m_editApplication->text() );
}


void K3b::DataVolumeDescDialog::slotVolumeSetSizeChanged( int i )
{
    m_spinVolumeSetNumber->setMaximum( i );
}


