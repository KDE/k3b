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
#include "k3bfilenamepatterndialog.h"
#include "k3bcdda.h"
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
#include <kapp.h>
#include <kconfig.h>
#include <klistview.h>
#include <klineedit.h>
#include <kfiledialog.h>
#include <kio/global.h>
#include <kiconloader.h>
#include <kstdguiitem.h>

#include <qgroupbox.h>
#include <qheader.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qdir.h>
#include <qstringlist.h>
#include <qmessagebox.h>
#include <qvgroupbox.h>
#include <qhgroupbox.h>
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

  m_parser = new K3bPatternParser();

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

  QButtonGroup *groupPattern = new QButtonGroup( 5, Qt::Vertical, i18n( "Destination Directory" ), frame, "pattern" );
  groupPattern->layout()->setSpacing( spacingHint() );
  groupPattern->layout()->setMargin( marginHint() );

  m_usePattern = new QRadioButton(i18n("Use pattern for destination directory and filename."), groupPattern, "pattern_box");
  QHBox* patternPathBox = new QHBox( groupPattern );
  patternPathBox->setSpacing( spacingHint() );
  m_labelSummaryName = new QLabel( patternPathBox );
  m_labelSummaryName->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  m_labelSummaryName->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, m_labelSummaryName->sizePolicy().verData() ) );
  QFont font = m_labelSummaryName->font();
  font.setBold(true);
  m_labelSummaryName->setFont( font );
  m_buttonPattern = new QToolButton( patternPathBox, "m_buttonPattern" );
  m_buttonPattern->setIconSet( SmallIconSet( "gear" ) );

  QFrame* line1 = new QFrame( groupPattern, "line1" );
  line1->setFrameStyle( QFrame::HLine | QFrame::Sunken );

  m_useStatic = new QRadioButton(i18n("Use this directory and default filenames."), groupPattern, "dir_box");

  QHBox* normalPathBox = new QHBox( groupPattern );
  normalPathBox->setSpacing( spacingHint() );
  m_editStaticRipPath = new KLineEdit( normalPathBox, "staticeditpattern");

  // FIXME: zuletzt benutzes verzeichnis abspeichern

  m_editStaticRipPath->setText( QDir::homeDirPath() );
  m_buttonStaticDir = new QToolButton( normalPathBox, "m_buttonStaticDir" );
  m_buttonStaticDir->setIconSet( SmallIconSet( "fileopen" ) );
  m_buttonStaticDir->setDisabled( true );


  m_groupFileType = new QButtonGroup( 4, Qt::Vertical, i18n("Filetype"), frame );
  m_radioWav = new QRadioButton( i18n("Wave"), m_groupFileType );
  m_radioMp3 = new QRadioButton( i18n("MP3"), m_groupFileType );
  m_radioOgg = new QRadioButton( i18n("Ogg Vorbis"), m_groupFileType ); // TODO: test if ogg available


  Form1Layout->addMultiCellWidget( m_viewTracks, 0, 0, 0, 1 );
  Form1Layout->addMultiCellWidget( groupPattern, 1, 1, 0, 0 );
  Form1Layout->addWidget( m_groupFileType, 1, 1 );
  Form1Layout->setColStretch( 0, 20 );

  setButtonOKText( i18n( "Start Ripping" ), i18n( "Starts copying the selected tracks") );

  connect(m_useStatic, SIGNAL( clicked() ), this, SLOT( useStatic() ) );
  connect(m_usePattern, SIGNAL( clicked() ), this, SLOT( usePattern() ) );
  connect(m_buttonPattern, SIGNAL(clicked() ), this, SLOT( showPatternDialog() ) );
  connect(m_buttonStaticDir, SIGNAL(clicked()), this, SLOT(slotFindStaticDir()) );
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
  if( c->readBoolEntry("usePattern", false) ) {
    usePattern();
  } else {
    useStatic();
  }

  // TODO: load default file type

  refresh();
}

void K3bRipperWidget::slotOk(){
//   QStringList::Iterator it;
//   int index = 0;
//   for( it = m_list.begin(); it != m_list.end(); ++it ){
//     QString tmp = *it;
//     if( m_useCustomDir ){
//       // check if directory exists else create it
//       //QString dest = m_labelSummaryName->text(); //
//       QString dest = m_directories[ index ];
//       QDir destDir( dest );
//       if( !destDir.exists() ){
// 	QStringList dirs = QStringList::split("/", dest);
// 	QStringList::Iterator d = dirs.begin();
// 	destDir = "/" + *d;
// 	for( ++d ; d != dirs.end(); ++d ){
// 	  destDir = destDir.absPath() +"/" + *d;
// 	  if( destDir.exists( ) )
// 	    continue;
// 	  bool successful = destDir.mkdir( destDir.absPath() );
// 	  if( !successful ){
// 	    QMessageBox::critical( this, i18n("Ripping Error"), i18n("Couldn't create directory: ") + dest, i18n("Ok") );
// 	    return;
// 	  }
// 	}
//       } // end directory
//       (*it) = dest + "/" + KIO::encodeFileName(tmp);
//       qDebug("(K3bRipperWidget) Final destination" + (*it) );
//     } else {
//       (*it) = m_editStaticRipPath->text() + "/" + tmp;
//     }
//     m_bytes += m_size[ index ]; //cdda->getRawTrackSize( m_tracks[ index ], drive);
//     qDebug("(K3bRipperWidget) KBytes: " + QString::number(m_bytes/1000) );
//     index++;
//   }
//   if( index == 0 ){
//     QMessageBox::critical( this, i18n("Ripping Error"), i18n("There is nothing to rip."), i18n("Ok") );
//     return;
//   }


//   // save all entries with artist title, path filename and so on
//   setSongList();

//    K3bCddaCopy* job = new K3bCddaCopy( this );
//    job->setDiskInfo( m_diskInfo );

//    // FIXME: set only if cdddb used
//    job->setCddbEntry( m_cddbEntry );

//    job->setCopyTracks( m_trackNumbers );

//    K3bBurnProgressDialog ripDialog( this, "Ripping" );
//    ripDialog.setJob( job );

//    job->start();
//    hide();
//    ripDialog.exec();

//    delete job;

//    slotClose();
}

void K3bRipperWidget::useStatic(){
  m_useStatic->setChecked( true );
  m_usePattern->setChecked( false );
  m_buttonPattern->setDisabled( true );
  m_editStaticRipPath->setEnabled( true );
  m_buttonStaticDir->setEnabled( true );
}
void K3bRipperWidget::usePattern(){
  m_usePattern->setChecked( true );
  m_useStatic->setChecked( false );
  m_buttonPattern->setEnabled( true );
  m_editStaticRipPath->setDisabled( true );
  m_buttonStaticDir->setDisabled( true );

  QString dir = m_parser->prepareDirectory( m_cddbEntry );
  dir = m_parser->prepareReplaceDirectory( dir );
  m_labelSummaryName->setText( dir );
}

void K3bRipperWidget::showPatternDialog(){
  K3bFilenamePatternDialog dialog(this);
  
  dialog.init( m_cddbEntry.cdTitle, m_cddbEntry.cdArtist, m_cddbEntry.titles[0], "01" );
  dialog.exec();
  refresh();
}



void K3bRipperWidget::refresh()
{
  // TODO: put here the filesize of the ripped files (that is for mp3 and ogg app. 1/10 
  //       of the full size)
  //       show the track type
  //       data tracks have always the same size
  //       show resulting filename, time, size, and type and not tracknumber, title, artist, and so on

  m_viewTracks->clear();

  for( QValueList<int>::const_iterator it = m_trackNumbers.begin();
       it != m_trackNumbers.end(); ++it ) {
    int index = *it - 1;

    QString extension = ".wav"; // FIXME: mp3,ogg,iso
    long fileSize = m_diskInfo.toc[index].length() * 2352;  // FIXME: mp3,ogg,iso

    (void)new KListViewItem( m_viewTracks,
			     m_parser->prepareFilename( m_cddbEntry, *it ) + extension,
			     K3b::framesToString( m_diskInfo.toc[index].length(), false ),
			     KIO::convertSize( fileSize ),
			     (m_diskInfo.toc[index].type() == K3bTrack::AUDIO ? i18n("Audio") : i18n("Data") ) );
  }
}




void K3bRipperWidget::setSongList(){
//   K3bSongManager *sm = k3bMain()->songManager();
//   //("/home/ft0001/songlist.xml");
//   QStringList::Iterator it;
//   int index = 0;
//   for( it = m_list.begin(); it != m_list.end(); ++it ){
//     int col = (*it).findRev("/");
//     QString path = (*it).left( col );
//     QListViewItem *item = m_viewTracks->itemAtIndex( index );
//     K3bSong song( item->text(5), m_cddbEntry.cdTitle, item->text(1), item->text(2), m_cddbEntry.discid, item->text(0).toInt() );
//     sm->addSong( path, song );
//     index++;
//     //QString tmpSong = sm.getSong( (*it) ).getFilename();
//     //qDebug("Song" + tmpSong);
//   }
//   sm->save();
}

void K3bRipperWidget::slotFindStaticDir() {
  QString path = KFileDialog::getExistingDirectory( m_editStaticRipPath->text(), this, i18n("Select Ripping Directory") );
  if( !path.isEmpty() ) {
    m_editStaticRipPath->setText( path );
  }
}


#include "k3bripperwidget.moc"
