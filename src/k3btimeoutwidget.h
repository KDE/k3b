/* 
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_TIMEOUT_WIDGET_H_
#define _K3B_TIMEOUT_WIDGET_H_

#include <QWidget>

class QPaintEvent;
class QResizeEvent;


namespace K3b {
class TimeoutWidget : public QWidget
{
  Q_OBJECT

 public:
  explicit TimeoutWidget( QWidget* parent );
  ~TimeoutWidget() override;

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

 public Q_SLOTS:
  void setTimeout( int msecs );
  void start();
  void stop();
  void pause();
  void resume();

  Q_SIGNALS:
  void timeout();

 protected:
  void paintEvent( QPaintEvent* ) override;
  void resizeEvent( QResizeEvent* ) override;

 private Q_SLOTS:
  void timeStep();
  void startTimer();

 private:
  class Private;
  Private* d;
};
}

#endif
