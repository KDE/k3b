#include "k3btoolbox.h"

#include <kaction.h>

#include <qtoolbutton.h>
#include <qsizepolicy.h>
#include <qlayout.h>
#include <qwhatsthis.h>
#include <qlabel.h>


K3bToolBox::K3bToolBox( QWidget* parent, const char* name )
  : QFrame( parent, name )
{
  setSizePolicy( QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed) );

  m_mainLayout = new QHBoxLayout( this );
  m_mainLayout->setMargin( 1 );
  m_mainLayout->setSpacing( 0 );
}


K3bToolBox::~K3bToolBox()
{
}


void K3bToolBox::addButton( KAction* action )
{
  QToolButton* button = addClearButton( action );

  connect( button, SIGNAL(clicked()), action, SLOT(activate()) );
  connect( action, SIGNAL(enabled(bool)), button, SLOT(setEnabled(bool)) );
}


void K3bToolBox::addSpacing()
{
  m_mainLayout->addSpacing( 5 );
}


void K3bToolBox::addLabel( const QString& text )
{
  QLabel* label = new QLabel( text, this );
  label->setSizePolicy( QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed) );

  m_mainLayout->addWidget( label );
}


void K3bToolBox::addWidget( QWidget* w )
{
  // TODO: reparent??

  m_mainLayout->addWidget( w );
}


void K3bToolBox::addToggleButton( KToggleAction* action )
{
  QToolButton* button = addClearButton( action );
  button->setToggleButton( true );

  if( action->isChecked() )
    button->toggle();

  connect( action, SIGNAL(toggled(bool)), button, SLOT(toggle()) );
  connect( button, SIGNAL(toggled(bool)), action, SLOT(setChecked(bool)) );
  connect( action, SIGNAL(enabled(bool)), button, SLOT(setEnabled(bool)) );
}


QToolButton* K3bToolBox::addClearButton( KAction* action )
{
  QToolButton* button = new QToolButton( this );
  button->setSizePolicy( QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed) );

  button->setIconSet( action->iconSet() );
  button->setTextLabel( action->toolTip(), true );
  button->setTextLabel( action->text() );
  QWhatsThis::add( button, action->whatsThis() );

  button->setAutoRaise( true );

  m_mainLayout->addWidget( button );

  return button;
}


#include "k3btoolbox.moc"
