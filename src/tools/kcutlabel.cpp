/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "kcutlabel.h"

#include <k3bstringutils.h>

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
      QString squeezedText = K3b::cutToWidth( fontMetrics(), *it, size().width() );
      newText += squeezedText;
      newText += "\n";
      if( squeezedText != *it )
	QToolTip::add( this, m_fullText );
    }
    newText.truncate( newText.length() - 1 ); // get rid of the last newline

    QLabel::setText( newText );
  }
  else {
    QString squeezedText = K3b::cutToWidth( fontMetrics(), m_fullText, size().width() );
    QLabel::setText( squeezedText );
    if( squeezedText != m_fullText )
      QToolTip::add( this, m_fullText );      
  }
}

#include "kcutlabel.moc"
