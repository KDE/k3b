
#include "kcutlabel.h"

#include <qtooltip.h>



KCutLabel::KCutLabel( const QString &text , QWidget *parent, const char *name )
 : QLabel ( parent, name ) {
  QSizePolicy myLabelSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
  setSizePolicy(myLabelSizePolicy);
  m_fullText = text;
  cutTextToLabel();
}

KCutLabel::KCutLabel( QWidget *parent, const char *name )
 : QLabel ( parent, name ) {
  QSizePolicy myLabelSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
  setSizePolicy(myLabelSizePolicy);
}

void KCutLabel::resizeEvent( QResizeEvent * ) {
  cutTextToLabel();
}

void KCutLabel::setText( const QString &text ) {
  m_fullText = text;
  cutTextToLabel();
}

void KCutLabel::cutTextToLabel()
{
  QToolTip::remove( this );
  QToolTip::hide();

  if( m_fullText.contains( "\n" ) ) {
    QString newText;
    QStringList lines = QStringList::split( "\n", m_fullText );
    for( QStringList::Iterator it = lines.begin(); it != lines.end(); ++it ) {
      QString squeezedText = cutToWidth( *it, size().width() );
      newText += squeezedText;
      newText += "\n";
      if( squeezedText != *it )
	QToolTip::add( this, m_fullText );
    }
    newText.truncate( newText.length() - 2 ); // get rid of the last newline

    QLabel::setText( newText );
  }
  else {
    QString squeezedText = cutToWidth( m_fullText, size().width() );
    QLabel::setText( squeezedText );
    if( squeezedText != m_fullText )
      QToolTip::add( this, m_fullText );      
  }
}


QString KCutLabel::cutToWidth( const QString& fullText, int cutWidth )
{
  QFontMetrics fm(fontMetrics());
  QString squeezedText = "...";
  int squeezedWidth = fm.width(squeezedText);
  int textWidth = fm.width(fullText);

  if( textWidth <= cutWidth ) {
    return fullText;
  }

  // estimate how many letters we can add to the dots
  int letters = fullText.length() * (cutWidth - squeezedWidth) / textWidth;
  squeezedText = fullText.left(letters) + "...";
  squeezedWidth = fm.width(squeezedText);

  if (squeezedWidth < cutWidth) {
    // we estimated too short
    // add letters while text < label
    do {
      letters++;
      squeezedText = fullText.left(letters) + "...";
      squeezedWidth = fm.width(squeezedText);
    } while (squeezedWidth < cutWidth);
    letters--;
    squeezedText = fullText.left(letters) + "...";
  } else if (squeezedWidth > cutWidth) {
    // we estimated too long
    // remove letters while text > label
    do {
      letters--;
      squeezedText = fullText.left(letters) + "...";
      squeezedWidth = fm.width(squeezedText);
    } while (squeezedWidth > cutWidth);
  }

  return squeezedText;
}


#include "kcutlabel.moc"
