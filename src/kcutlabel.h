
#ifndef KCUTLABEL_H
#define KCUTLABEL_H

#include <qlabel.h>



/*
 * @ref QLabel
 */
class KCutLabel : public QLabel 
{
  Q_OBJECT

 public:
  /**
   * Default constructor.
   */
  KCutLabel( QWidget *parent = 0, const char *name = 0);
  KCutLabel( const QString &text, QWidget *parent = 0, const char *name = 0 );

  virtual QSize minimumSizeHint() const;

 public slots:
  void setText( const QString & );

 protected:
  /**
   * used when widget is resized
   */
  void resizeEvent( QResizeEvent * );
  /**
   * does the dirty work
   */
  void cutTextToLabel();
  QString cutToWidth( const QString&, int );
  QString m_fullText;
};

#endif // KCUTLABEL_H
