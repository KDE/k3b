#ifndef K3B_BUSY_WIDGET_H
#define K3B_BUSY_WIDGET_H


#include <qframe.h>


class QPainter;
class QTimer;


class K3bBusyWidget : public QFrame
{
  Q_OBJECT;

 public:
  K3bBusyWidget( QWidget* parent = 0, const char* name = 0 );
  ~K3bBusyWidget();

  void showBusy( bool b );

  QSize sizeHint() const;
  QSize minimumSizeHint() const;

 protected:
  void drawContents( QPainter* p );

 private slots:
  void animateBusy();

 private:
  bool m_bBusy;
  int m_iBusyPosition;

  QTimer* m_busyTimer;
};


#endif
