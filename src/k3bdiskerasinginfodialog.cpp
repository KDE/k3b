#include "k3bdiskerasinginfodialog.h"

#include "../k3bbusywidget.h"

#include <qlabel.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qframe.h>

#include <klocale.h>


K3bErasingInfoDialog::K3bErasingInfoDialog( QWidget* parent, const char* name ) 
  : KDialogBase( parent, name, true, i18n("Erasing"), Cancel|Ok, Ok, true )
{
  QFrame* main = makeMainWidget();
  QGridLayout* mainLayout = new QGridLayout( main );
  mainLayout->setMargin( marginHint() );
  mainLayout->setSpacing( spacingHint() );

  m_label = new QLabel( i18n("Erasing CD-RW"), main );
  m_busyWidget = new K3bBusyWidget( main );

  mainLayout->addWidget( m_label, 0, 0 );
  mainLayout->addWidget( m_busyWidget, 1, 0 );

  showButtonOK( false );
  m_busyWidget->showBusy( true );
}


K3bErasingInfoDialog::~K3bErasingInfoDialog()
{}


void K3bErasingInfoDialog::slotFinished( bool success )
{
  m_busyWidget->showBusy( false );

  showButtonOK( true );
  showButtonCancel( false );
  
  if( success )
    m_label->setText( i18n("Disk successfully erased. Please reload the disk.") );
  else
    m_label->setText( i18n("K3b was unable to erase the disk.") );
}


#include "k3bdiskerasinginfodialog.moc"
