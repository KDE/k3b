#include "k3bdatavolumedescwidget.h"

#include "../k3bisooptions.h"
#include "k3bisovalidator.h"

#include <qlineedit.h>


K3bDataVolumeDescWidget::K3bDataVolumeDescWidget( QWidget* parent, const char* name )
  : base_K3bDataVolumeDescWidget( parent, name )
{
  // are this really the allowed characters? What about Joliet or UDF?
  K3bIsoValidator* isoValidator = new K3bIsoValidator( this, "isoValidator" );

  // TODO: no whitespaces! They get converted to "_" in the DataJob and the IsoImager

  m_editVolumeName->setValidator( isoValidator );
  m_editVolumeSetName->setValidator( isoValidator );
  m_editPublisher->setValidator( isoValidator );
  m_editPreparer->setValidator( isoValidator );
  m_editSystem->setValidator( isoValidator );
  m_editApplication->setValidator( isoValidator );
}


K3bDataVolumeDescWidget::~K3bDataVolumeDescWidget()
{
}


void K3bDataVolumeDescWidget::load( const K3bIsoOptions& o )
{
  m_editVolumeName->setText( o.volumeID() );
  m_editVolumeSetName->setText( o.volumeSetId() );
  m_editPublisher->setText( o.publisher() );
  m_editPreparer->setText( o.preparer() );
  m_editSystem->setText( o.systemId() );
  m_editApplication->setText( o.applicationID() );
}


void K3bDataVolumeDescWidget::save( K3bIsoOptions& o )
{
  o.setVolumeID( m_editVolumeName->text() );
  o.setVolumeSetId( m_editVolumeSetName->text() );
  o.setPublisher( m_editPublisher->text() );
  o.setPreparer( m_editPreparer->text() );
  o.setSystemId( m_editSystem->text() );
  o.setApplicationID( m_editApplication->text() );
}


#include "k3bdatavolumedescwidget.moc"
