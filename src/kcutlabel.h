
#ifndef KCUTLABEL_H
#define KCUTLABEL_H

#include <qlabel.h>
#include <qtooltip.h>


/*
 * @ref QLabel
 */
class KCutLabel : public QLabel {
  Q_OBJECT

public:
  /**
   * Default constructor.
   */
  KCutLabel( QWidget *parent );
  KCutLabel( QWidget *parent, const char *name);                       //### merge with the above
  KCutLabel( const QString &text, QWidget *parent );
  KCutLabel( const QString &text, QWidget *parent, const char *name ); //### merge with the above

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
  QString fullText;
};

#endif // KCUTLABEL_H
