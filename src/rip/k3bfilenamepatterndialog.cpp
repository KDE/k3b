/***************************************************************************
                          k3bfilenamepatterndialog.cpp  -  description
                             -------------------
    begin                : Sun Nov 18 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "k3bfilenamepatterndialog.h"
#include "k3bpatternwidget.h"

#include <klocale.h>




K3bFilenamePatternDialog::K3bFilenamePatternDialog(QWidget *parent, const char *name )
 : KDialogBase( parent, name, true, i18n("Filename Pattern"), Apply|Ok|Cancel, Ok, false ) 
{
  m_frame = new K3bPatternWidget(this);
  setMainWidget( m_frame );
}


K3bFilenamePatternDialog::~K3bFilenamePatternDialog()
{
}


void K3bFilenamePatternDialog::init( const QString& album, const QString& artist, const QString& title, const QString& number){
  m_frame->init(album, artist, title, number);
  m_frame->readSettings();
}

// slots
// -------------------------------------------------
void K3bFilenamePatternDialog::slotApply( )
{
  m_frame->apply();
}


void K3bFilenamePatternDialog::slotOk( )
{
  slotApply();
  KDialogBase::slotOk();
}

#include "k3bfilenamepatterndialog.moc"

