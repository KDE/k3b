#ifndef K3B_TOOLBOX_H
#define K3B_TOOLBOX_H

#include <qframe.h>
#include <qstring.h>

class KAction;
class KToggleAction;
class QGridLayout;
class QToolButton;


class K3bToolBox : public QFrame
{
  Q_OBJECT

 public:
  K3bToolBox( QWidget* parent = 0, const char* name = 0 );
  ~K3bToolBox();

  void addButton( KAction* );
  void addToggleButton( KToggleAction* );
  void addWidget( QWidget* );
  void addLabel( const QString& );
  void addSpacing();

 protected:
  QToolButton* addClearButton( KAction* );
  QGridLayout* m_mainLayout;
};


#endif
