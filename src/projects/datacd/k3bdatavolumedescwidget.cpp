/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bdatavolumedescwidget.h"

#include "k3bisooptions.h"
#include "k3bisovalidator.h"

#include <qlineedit.h>
#include <qspinbox.h>


K3bDataVolumeDescWidget::K3bDataVolumeDescWidget( QWidget* parent, const char* name )
  : base_K3bDataVolumeDescWidget( parent, name )
{
  // the maximal number of characters that can be inserted are set in the ui file!

  // are this really the allowed characters? What about Joliet or UDF?
  K3bIsoValidator* isoValidator = new K3bIsoValidator( this, "isoValidator" );

  m_editVolumeName->setValidator( isoValidator );
  m_editVolumeSetName->setValidator( isoValidator );
  m_editPublisher->setValidator( isoValidator );
  m_editPreparer->setValidator( isoValidator );
  m_editSystem->setValidator( isoValidator );
  m_editApplication->setValidator( isoValidator );

  connect( m_spinVolumeSetSize, SIGNAL(valueChanged(int)),
	   this, SLOT(slotVolumeSetSizeChanged(int)) );
}


K3bDataVolumeDescWidget::~K3bDataVolumeDescWidget()
{
}


void K3bDataVolumeDescWidget::load( const K3bIsoOptions& o )
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


void K3bDataVolumeDescWidget::save( K3bIsoOptions& o )
{
  o.setVolumeID( m_editVolumeName->text() );
  o.setVolumeSetId( m_editVolumeSetName->text() );
  o.setVolumeSetSize( m_spinVolumeSetSize->value() );
  o.setVolumeSetNumber( m_spinVolumeSetNumber->value() );
  o.setPublisher( m_editPublisher->text() );
  o.setPreparer( m_editPreparer->text() );
  o.setSystemId( m_editSystem->text() );
  o.setApplicationID( m_editApplication->text() );
}


void K3bDataVolumeDescWidget::slotVolumeSetSizeChanged( int i )
{
  m_spinVolumeSetNumber->setMaxValue( i );
}

#include "k3bdatavolumedescwidget.moc"
