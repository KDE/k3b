
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

    QRect rect = contentsRect();
    rect.setLeft( rect.left() + 20 );
    rect.setRight( rect.right() - 20 );
    rect.setTop( rect.top() + 10 );
    rect.setBottom( rect.top() + 40 );

    p->drawText( rect, AlignTop|AlignHCenter|WordBreak, m_text );
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

  m_mainLayout = new QGridLayout( this );
  m_mainLayout->addWidget( m_labelSetupLogo, 0, 0 );

  m_mainLayout->setMargin( KDialog::marginHint() );
  m_mainLayout->setSpacing( 0 );
  m_mainLayout->addColSpacing( 1, KDialog::marginHint() );
  m_mainLayout->setRowStretch( 1, 1 );
  m_mainLayout->setColStretch( 2, 1 );
}


K3bSetupTab::~K3bSetupTab()
{
}


void K3bSetupTab::setMainWidget( QWidget* mainWidget )
{
  m_mainWidget = mainWidget;
  m_mainLayout->addMultiCellWidget( mainWidget, 0, 0, 2, 2 );
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
  readSettings();
}


#include "k3bsetuptab.moc"
