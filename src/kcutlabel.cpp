
#include "kcutlabel.h"

KCutLabel::KCutLabel( const QString &text , QWidget *parent)
 : QLabel ( parent) {
  QSizePolicy myLabelSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
  setSizePolicy(myLabelSizePolicy);
  fullText = text;
  cutTextToLabel();
}

KCutLabel::KCutLabel( const QString &text , QWidget *parent, const char *name )
 : QLabel ( parent, name ) {
  QSizePolicy myLabelSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
  setSizePolicy(myLabelSizePolicy);
  fullText = text;
  cutTextToLabel();
}

KCutLabel::KCutLabel( QWidget *parent)
 : QLabel ( parent) {
  QSizePolicy myLabelSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
  setSizePolicy(myLabelSizePolicy);
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
  fullText = text;
  cutTextToLabel();
}

void KCutLabel::cutTextToLabel() {
  QFontMetrics fm(fontMetrics());
  int labelWidth = size().width();
  int textWidth = fm.width(fullText);
  if (textWidth > labelWidth) {
    // start with the dots only
    QString squeezedText = "...";
    int squeezedWidth = fm.width(squeezedText);

    // estimate how many letters we can add to the dots on both sides
    int letters = fullText.length() * (labelWidth - squeezedWidth) / textWidth;
    squeezedText = fullText.left(letters) + "...";
    squeezedWidth = fm.width(squeezedText);

    if (squeezedWidth < labelWidth) {
        // we estimated too short
        // add letters while text < label
        do {
                letters++;
                squeezedText = fullText.left(letters) + "...";
                squeezedWidth = fm.width(squeezedText);
        } while (squeezedWidth < labelWidth);
        letters--;
        squeezedText = fullText.left(letters) + "...";
    } else if (squeezedWidth > labelWidth) {
        // we estimated too long
        // remove letters while text > label
        do {
            letters--;
            squeezedText = fullText.left(letters) + "...";
            squeezedWidth = fm.width(squeezedText);
        } while (squeezedWidth > labelWidth);
    }

    if (letters < 5) {
    	// too few letters added -> we give up squeezing
    	QLabel::setText(fullText);
    } else {
	QLabel::setText(squeezedText);
    }

    QToolTip::remove( this );
    QToolTip::add( this, fullText );

  } else {
    QLabel::setText(fullText);

    QToolTip::remove( this );
    QToolTip::hide();

  };
}


#include "kcutlabel.moc"
