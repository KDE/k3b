
#ifndef K3BSPLASH_H
#define K3BSPLASH_H

#include <qvbox.h>

class QLabel;
class QMouseEvent;
class QPaintEvent;
class QString;


class K3bSplash : public QVBox
{
Q_OBJECT

 public:
  K3bSplash( QWidget* parent = 0, const char* name = 0 );
  ~K3bSplash();

 public slots:
  void addInfo( const QString& );

 protected:
  void mousePressEvent( QMouseEvent* );
  //  void paintEvent( QPaintEvent* );

 private:
  QLabel* m_infoBox;
};

#endif
