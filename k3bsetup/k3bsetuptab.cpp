
#include "k3bsetuptab.h"
#include "k3bsetup.h"
#include "k3bsetupwizard.h"

#include <kstandarddirs.h>
#include <kdialog.h>
#include <klocale.h>

#include <qlabel.h>
#include <qlayout.h>
#include <qfont.h>
#include <qcolor.h>
#include <qpixmap.h>
#include <qpainter.h>


class K3bSetupTab::PrivatePicLabel : public QLabel
{
public:
  PrivatePicLabel( const QString& text, QWidget* parent )
    : QLabel( parent ), m_text( text ) {}

protected:
  void drawContents( QPainter* p )
  {
    QLabel::drawContents(p);

//     QRect rect = contentsRect();
//     rect.setLeft( rect.left() + 20 );
//     rect.setRight( rect.right() - 20 );
//     rect.setTop( rect.top() + 10 );
//     //    rect.setBottom( rect.top() + 40 );

//     p->drawText( rect, AlignTop|AlignHCenter|WordBreak, m_text );
  }

private:
  QString m_text;
};



K3bSetupTab::K3bSetupTab( int index, int overall, const QString& info, K3bSetupWizard* parent, const char* name )
  : QWidget( parent, name )
{
  parent->addPage( this, i18n("Step %1 of %2").arg(index).arg(overall) );

  m_setup = parent->setup();


  static QPixmap setupLogo( locate( "data", "k3b/pics/k3bsetup_1.png" ) );

  m_labelSetupLogo = new PrivatePicLabel( info, this );
  m_labelSetupLogo->setPixmap( setupLogo );
  QFont f( m_labelSetupLogo->font() );
  f.setBold(true);
  m_labelSetupLogo->setFont(f);

  QLabel* topLabel = new QLabel( info, this );
  topLabel->setMargin( KDialog::marginHint() );
  topLabel->setPaletteBackgroundColor( QColor(201, 208, 255) );
  topLabel->setAlignment( AlignCenter | WordBreak );
  topLabel->setFont( f );
  
  m_mainLayout = new QGridLayout( this );
  m_mainLayout->addWidget( m_labelSetupLogo, 1, 0 );
  m_mainLayout->addWidget( topLabel, 0, 0 );
  m_mainLayout->setMargin( 0 );
  m_mainLayout->setSpacing( 0 );
  m_mainLayout->addColSpacing( 1, 2*KDialog::marginHint() );
  m_mainLayout->setRowStretch( 0, 1 );
  m_mainLayout->setColStretch( 2, 1 );

  m_initialized = false;
}


K3bSetupTab::~K3bSetupTab()
{
}


void K3bSetupTab::setMainWidget( QWidget* mainWidget )
{
  m_mainWidget = mainWidget;
  m_mainLayout->addMultiCellWidget( mainWidget, 0, 1, 2, 2 );
}

void K3bSetupTab::readSettings()
{
}


bool K3bSetupTab::saveSettings()
{
  return true;
}


bool K3bSetupTab::appropriate()
{
  return true;
}


void K3bSetupTab::aboutToShow()
{
  if( !m_initialized ) {
    readSettings();
    m_initialized = true;
  }
}


void K3bSetupTab::setPixmap( const QPixmap& pix )
{
  m_labelSetupLogo->setPixmap( pix );
}

#include "k3bsetuptab.moc"
