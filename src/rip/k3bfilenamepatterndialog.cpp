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
#include "../rip/k3bpatternwidget.h"
#include "k3bripperwidget.h"
//#include "k3bcdview.h"

#include <qlistview.h>
#include <qgroupbox.h>
#include <qvbox.h>
#include <qhbox.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qframe.h>
#include <qstringlist.h>

#include <klocale.h>
#include <kdialog.h>
#include <kdialogbase.h>
#include <klineedit.h>
#include <kcombobox.h>

#define DEFAULT_ARTIST   "artist"
#define DEFAULT_ALBUM    "album"
#define DEFAULT_TITLE      "title"

#define SPACE_1     1
#define SPACE_2     3
#define FILE_1          0
#define FILE_2          2
#define FILE_3          4


K3bFilenamePatternDialog::K3bFilenamePatternDialog(K3bRipperWidget *parent, const char *name )
 : KDialogBase( parent, name, true, i18n("Filename Pattern"), Apply|Ok|Cancel, Ok, false ) 
{
  m_frame = new K3bPatternWidget(this);
  setMainWidget( m_frame );

  m_parent = parent;
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
  m_parent->setFilePatternList(QStringList::split( ',', m_frame->getFilePattern()) );
  qDebug("DirPattern: " + m_frame->getDirPattern() );
  m_parent->setDirPatternList(QStringList::split( ',', m_frame->getDirPattern()) );
  m_parent->setReplacements( m_frame->getSpaceReplaceDir(), m_frame->getReplaceCharDir(),
			     m_frame->getSpaceReplaceFile(), m_frame->getReplaceCharFile(), m_frame->getParseMixed() );
  m_parent->refresh();
}


void K3bFilenamePatternDialog::slotOk( )
{
  slotApply();
  done( 0 );
}

#include "k3bfilenamepatterndialog.moc"

