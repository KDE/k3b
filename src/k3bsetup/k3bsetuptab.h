

#ifndef K3BSETUPTAB_H
#define K3BSETUPTAB_H

#include <qvariant.h>
#include <qwidget.h>


class QLabel;
class QGridLayout;
class K3bSetup;
class K3bSetupWizard;


class K3bSetupTab : public QWidget
{ 
    Q_OBJECT

 public:
  /**
   * create a new K3bSetupTab and insert it into the K3bSetupWizard
   */
  K3bSetupTab( int, int, const QString& info, K3bSetupWizard* parent, const char* = 0 );
  ~K3bSetupTab();

  virtual void readSettings();
  virtual bool saveSettings();

  virtual bool appropriate();

 protected:
  void setMainWidget( QWidget* );
  K3bSetup* setup() { return m_setup; }

 private:
  QLabel* m_labelSetupLogo;
  QLabel* m_labelInfoText;

  QWidget* m_mainWidget;

  QGridLayout* m_mainLayout;

  K3bSetup* m_setup;
};

#endif
