/* 
 *
 * $Id: $
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "kcutlabel.h"

#include <qtooltip.h>
#include <qstringlist.h>
#include <kdebug.h>


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

QSize KCutLabel::minimumSizeHint() const
{
  QSize sh = QLabel::minimumSizeHint();
  sh.setWidth(-1);
  return sh;
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
    newText.truncate( newText.length() - 1 ); // get rid of the last newline

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

  if( fm.width(fullText.right(1) + "..." ) > cutWidth ) {
    kdDebug() << "(KCutLabel) not able to cut text to " << cutWidth << "!" << endl;
    return fullText.right(1) + "...";
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
