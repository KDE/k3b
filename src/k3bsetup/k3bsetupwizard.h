

#ifndef K3BSETUPWIZARD_H
#define K3BSETUPWIZARD_H

#include <qvariant.h>
#include <kwizard.h>


class QCloseEvent;
class QKeyEvent;

class K3bSetup;
class KSimpleConfig;


class K3bSetupWizard : public KWizard
{ 
    Q_OBJECT

 public:
  K3bSetupWizard( K3bSetup*, QWidget* = 0, const char* = 0, bool = FALSE, WFlags = WType_TopLevel | WDestructiveClose );
  ~K3bSetupWizard();

  /**
   * reimplemented from QWizard
   */
  bool appropriate( QWidget* ) const;

  K3bSetup* setup() const { return m_setup; }

 protected slots:
  void accept();
  void next();

 protected:
  void closeEvent( QCloseEvent* );
  void keyPressEvent( QKeyEvent* );

 private:
  K3bSetup* m_setup;
};

#endif // K3BSETUPWIZARD_H
