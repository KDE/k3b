/***************************************************************************
                          k3bripperwidget.cpp  -  description
                             -------------------
    begin                : Tue Mar 27 2001
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

#include "k3bripperwidget.h"
#include "../k3bcddb.h"
#include "k3bcddacopy.h"
#include "k3bcdview.h"
#include "k3bpatternparser.h"
#include "../k3bburnprogressdialog.h"
#include "songdb/k3bsong.h"
#include "songdb/k3bsongmanager.h"
#include "../k3b.h"
#include "../tools/k3bglobals.h"
#include "../device/k3btrack.h"


#include <kcombobox.h>
#include <klocale.h>
#include <kapplication.h>
#include <kconfig.h>
#include <klistview.h>
#include <klineedit.h>
#include <kfiledialog.h>
#include <kio/global.h>
#include <kiconloader.h>
#include <kstdguiitem.h>

#include <qgroupbox.h>
#include <qheader.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qdir.h>
#include <qstringlist.h>
#include <qmessagebox.h>
#include <qgroupbox.h>
#include <qfont.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qhbox.h>
#include <qtoolbutton.h>



K3bRipperWidget::K3bRipperWidget(const K3bDiskInfo& diskInfo, const K3bCddbEntry& entry, const QValueList<int>& tracks,
				 QWidget *parent, const char *name )
  : KDialogBase( parent, name, true, i18n("Ripping CD"), User1|Ok, Ok, false, KStdGuiItem::close() ),
    m_diskInfo( diskInfo ), m_cddbEntry( entry ), m_trackNumbers( tracks )
{
  setButtonBoxOrientation( Vertical );

  setupGui();

  init();
}


void K3bRipperWidget::setupGui()
{
  QFrame *frame = makeMainWidget();
  QGridLayout* Form1Layout = new QGridLayout( frame );
  Form1Layout->setSpacing( KDialog::spacingHint() );
  Form1Layout->setMargin( 0 );

  //  QVGroupBox *groupListView = new QVGroupBox(frame, "list" );
  m_viewTracks = new KListView( frame, "m_viewTracks" );
  m_viewTracks->addColumn(i18n( "Filename") );
  m_viewTracks->addColumn(i18n( "Length") );
  m_viewTracks->addColumn(i18n( "Filesize") );
  m_viewTracks->addColumn(i18n( "Type") );
  m_viewTracks->addColumn(i18n( "Path") );


  QGroupBox* groupOptions = new QGroupBox( 0, Qt::Vertical, i18n("Options"), frame );
  groupOptions->layout()->setMargin( 0 );
  QGridLayout* groupOptionsLayout = new QGridLayout( groupOptions->layout() );
  groupOptionsLayout->setMargin( marginHint() );
  groupOptionsLayout->setSpacing( spacingHint() );

  QLabel* destLabel = new QLabel( i18n("Destination Base Directory"), groupOptions );
  m_editStaticRipPath = new KLineEdit( groupOptions, "staticeditpattern");
  m_buttonStaticDir = new QToolButton( groupOptions, "m_buttonStaticDir" );
  m_buttonStaticDir->setIconSet( SmallIconSet( "fileopen" ) );
  QFrame* line1 = new QFrame( groupOptions, "line1" );
  line1->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  m_checkUsePattern = new QCheckBox( i18n("Use directory and filename pattern"), groupOptions );
  m_buttonPattern = new QToolButton( groupOptions, "m_buttonPattern" );
  m_buttonPattern->setIconSet( SmallIconSet( "gear" ) );

  groupOptionsLayout->addMultiCellWidget( destLabel, 0, 0, 0, 1 );
  groupOptionsLayout->addWidget( m_editStaticRipPath, 1, 0 );
  groupOptionsLayout->addWidget( m_buttonStaticDir, 1, 1 );
  groupOptionsLayout->addMultiCellWidget( line1, 2, 2, 0, 1 );
  groupOptionsLayout->addWidget( m_checkUsePattern, 3, 0 );
  groupOptionsLayout->addWidget( m_buttonPattern, 3, 1 );
  groupOptionsLayout->setColStretch( 0, 1 );


  m_groupFileType = new QButtonGroup( 4, Qt::Vertical, i18n("Filetype"), frame );
  m_radioWav = new QRadioButton( i18n("Wave"), m_groupFileType );
  m_radioMp3 = new QRadioButton( i18n("MP3"), m_groupFileType );
  m_radioOgg = new QRadioButton( i18n("Ogg Vorbis"), m_groupFileType ); // TODO: test if ogg available


  Form1Layout->addMultiCellWidget( m_viewTracks, 0, 0, 0, 1 );
  Form1Layout->addMultiCellWidget( groupOptions, 1, 1, 0, 0 );
  Form1Layout->addWidget( m_groupFileType, 1, 1 );
  Form1Layout->setColStretch( 0, 20 );

  setButtonOKText( i18n( "Start Ripping" ), i18n( "Starts copying the selected tracks") );

  connect( m_buttonPattern, SIGNAL(clicked() ), this, SLOT(showPatternDialog()) );
  connect( m_buttonStaticDir, SIGNAL(clicked()), this, SLOT(slotFindStaticDir()) );
  connect( m_checkUsePattern, SIGNAL(toggled(bool)), this, SLOT(refresh()) );
  connect( m_radioWav, SIGNAL(toggled(bool)), this, SLOT(refresh()) );
  connect( m_radioOgg, SIGNAL(toggled(bool)), this, SLOT(refresh()) );
  connect( m_radioMp3, SIGNAL(toggled(bool)), this, SLOT(refresh()) );
}

K3bRipperWidget::~K3bRipperWidget()
{
}


void K3bRipperWidget::slotUser1()
{
  slotClose();
}

void K3bRipperWidget::init()
{
  KConfig* c = k3bMain()->config();
  c->setGroup( "Ripping" );

  QString filetype = c->readEntry( "last used filetype", "wav" );
  m_radioWav->setChecked( filetype == "wav" );
  m_radioOgg->setChecked( filetype == "ogg" );
  m_radioMp3->setChecked( filetype == "mp3" );

  m_editStaticRipPath->setText( c->readEntry( "last ripping directory", QDir::homeDirPath() ) );
  m_checkUsePattern->setChecked( !m_cddbEntry.cdArtist.isEmpty() && 
				 m_cddbEntry.titles.count() >= m_diskInfo.toc.count() );
    
  refresh();
}

void K3bRipperWidget::slotOk()
{
  KConfig* c = k3bMain()->config();
  c->setGroup( "Ripping" );

  c->writeEntry( "last ripping directory", m_editStaticRipPath->text() );
  QString filetype;
  if( m_radioOgg->isChecked() )
    filetype = "ogg";
  else if( m_radioMp3->isChecked() )
    filetype = "mp3";
  else
    filetype = "wav";

  c->writeEntry( "last used filetype", filetype );

//   // save all entries with artist title, path filename and so on
//   setSongList();

  K3bCddaCopy* job = new K3bCddaCopy( this );
  job->setDevice( m_diskInfo.device );

  job->setCddbEntry( m_cddbEntry );
  job->setUsePattern( m_checkUsePattern->isChecked() );
  job->setBaseDirectory( m_editStaticRipPath->text() );
  job->setCopyTracks( m_trackNumbers );

  K3bBurnProgressDialog ripDialog( this, "Ripping" );
  ripDialog.setJob( job );

  job->start();
  hide();
  ripDialog.exec();

  delete job;

  slotClose();
}


void K3bRipperWidget::showPatternDialog()
{
  k3bMain()->showOptionDialog( 4 );
  refresh();
}



void K3bRipperWidget::refresh()
{
  KConfig* c = kapp->config();
  c->setGroup( "Ripping" );

  m_viewTracks->clear();


  for( QValueList<int>::const_iterator it = m_trackNumbers.begin();
       it != m_trackNumbers.end(); ++it ) {
    int index = *it - 1;

    QString extension;
    if( m_radioOgg->isChecked() )
      extension = ".ogg";
    else if( m_radioMp3->isChecked() )
      extension = ".mp3";
    else
      extension = ".wav";

    long fileSize = m_diskInfo.toc[index].length() * 2352;

    if( m_diskInfo.toc[index].type() == K3bTrack::DATA ) {
      extension = ".iso";
      continue;  // TODO: find out how to rip the iso data
    }
    else {
      // FIXME: mp3,ogg file size recalculation
    }


    QString filename, directory;

    if( m_checkUsePattern->isChecked() && m_cddbEntry.titles.count() >= *it ) {
      filename = K3bPatternParser::parsePattern( m_cddbEntry, *it, 
						 c->readEntry( "filename pattern", "%a - %t" ),
						 c->readBoolEntry( "replace blank in filename", false ),
						 c->readEntry( "filename replace string", "_" ) ) + extension;
      
      directory = K3bPatternParser::parsePattern( m_cddbEntry, *it, 
						  c->readEntry( "directory pattern", "%r/%m" ),
						  c->readBoolEntry( "replace blank in directory", false ),
						  c->readEntry( "directory replace string", "_" ) );
    }
    else {
      filename = i18n("Track%1").arg( QString::number( *it ).rightJustify( 2, '0' ) ) + extension;
      directory = "";
    }

    (void)new KListViewItem( m_viewTracks,
			     filename,
			     K3b::framesToString( m_diskInfo.toc[index].length(), false ),
			     KIO::convertSize( fileSize ),
			     (m_diskInfo.toc[index].type() == K3bTrack::AUDIO ? i18n("Audio") : i18n("Data") ),
			     directory );
  }
}


void K3bRipperWidget::slotFindStaticDir() {
  QString path = KFileDialog::getExistingDirectory( m_editStaticRipPath->text(), this, i18n("Select Ripping Directory") );
  if( !path.isEmpty() ) {
    m_editStaticRipPath->setText( path );
  }
}


#include "k3bripperwidget.moc"
