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

#include <kcombobox.h>
#include <klocale.h>
#include <kapp.h>
#include <kconfig.h>
#include <klistview.h>
#include <klineedit.h>
#include <kprogress.h>
#include <kfiledialog.h>
#include <kio/job.h>
#include <kio/global.h>

#include <qarray.h>
#include <qgroupbox.h>
#include <qheader.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qdir.h>
#include <qstringlist.h>
#include <qevent.h>
#include <qtimer.h>
#include <qmessagebox.h>
#include <qvgroupbox.h>
#include <qhgroupbox.h>
#include <qcheckbox.h>
#include <qfont.h>

K3bRipperWidget::K3bRipperWidget(const QString& device, K3bCddb *cddb, QWidget *parent, const char *name )
	: KDialogBase( parent, name, true, i18n("Ripping CD"), KDialogBase::Close|KDialogBase::Ok)
{
    m_device = device;
    m_cddb = cddb;
    //m_cdview = parent;
    m_copy = 0;
    m_bytes = 0;
    m_finalClose = false;
    m_album = m_cddb->getAlbum();
    m_titles = m_cddb->getTitles();
    KConfig* c = kapp->config();
    c->setGroup("Ripping");
    //m_useFilePattern = c->readBoolEntry("useFilePattern", false);
    m_useConfigDirectoryPattern = c->readBoolEntry("usePattern", false);
    m_filePatternList = c->readListEntry("filePattern");
    m_dirPatternList = c->readEntry("dirBasePath");
    m_dirPatternList += c->readEntry("dirGroup1");
    m_dirPatternList += c->readEntry("dirGroup2");
    m_spaceDir = c->readBoolEntry("spaceReplaceDir", false);
    m_spaceFile = c->readBoolEntry("spaceReplaceFile", false);
    m_parseMixed = c->readBoolEntry("checkSlashFile", true);
    m_editFile = c->readEntry("spaceReplaceCharFile", "");
    m_editDir = c->readEntry("spaceReplaceCharDir", "");
    setupGui();
}

void K3bRipperWidget::setupGui(){
    QFrame *frame = makeMainWidget();
    Form1Layout = new QGridLayout( frame );
    Form1Layout->setSpacing( KDialog::spacingHint() );
    Form1Layout->setMargin( KDialog::marginHint() );

    QVGroupBox *groupListView = new QVGroupBox(frame, "list" );
    m_viewTracks = new KListView( groupListView, "m_viewTracks" );
    m_viewTracks->addColumn(i18n( "No") );
    m_viewTracks->addColumn(i18n( "Artist") );
    m_viewTracks->addColumn(i18n( "Title") );
    m_viewTracks->addColumn(i18n( "Time") );
    m_viewTracks->addColumn(i18n( "Size") );
    m_viewTracks->addColumn(i18n( "Filename") );

    QGroupBox *groupPattern = new QGroupBox( frame, "pattern" );
    groupPattern->setColumnLayout(0, Qt::Vertical );
    groupPattern->setTitle( i18n( "Destination Directory" ) );
    QGridLayout *optionsLayout = new QGridLayout( groupPattern->layout() );
    m_usePattern = new QCheckBox(i18n("Use pattern for destination directory and filename."), groupPattern, "pattern_box");
    m_useStatic = new QCheckBox(i18n("Use this directory and default filenames."), groupPattern, "dir_box");
    m_labelSummaryName = new QLabel(groupPattern);
    m_labelSummaryName->font().setBold(true);
    m_labelSummaryName->setFrameStyle( QFrame::Panel | QFrame::Sunken );
    QFont font = m_labelSummaryName->font();
    font.setBold(true);
    m_labelSummaryName->setFont( font );

    QFrame* line1 = new QFrame( groupPattern, "line1" );
    line1->setFrameStyle( QFrame::HLine | QFrame::Sunken );
    m_editStaticRipPath = new KLineEdit(groupPattern, "staticeditpattern");
    m_editStaticRipPath->setText( QDir::homeDirPath() );

    m_buttonPattern = new QPushButton( groupPattern, "m_buttonPattern" );
    m_buttonPattern->setText( tr( "....." ) );
    m_buttonStaticDir = new QPushButton( groupPattern, "m_buttonStaticDir" );
    m_buttonStaticDir->setText( tr( "....." ) );
    m_buttonStaticDir->setDisabled( true );

    optionsLayout->setSpacing( KDialog::spacingHint() );
    optionsLayout->setMargin( KDialog::marginHint() );
    optionsLayout ->addMultiCellWidget( m_usePattern, 0, 0, 0, 1);
    optionsLayout ->addMultiCellWidget( m_labelSummaryName, 1, 1, 0, 0);
    optionsLayout ->addMultiCellWidget( m_buttonPattern, 1, 1, 1, 1);
    optionsLayout ->addMultiCellWidget( line1, 2, 2, 0, 1);
    optionsLayout ->addMultiCellWidget( m_useStatic, 3, 3, 0, 1);
    optionsLayout ->addMultiCellWidget( m_editStaticRipPath, 4, 4, 0, 0);
    optionsLayout ->addMultiCellWidget( m_buttonStaticDir, 4, 4, 1, 1);
    optionsLayout ->setColStretch( 0, 10 );

    QHGroupBox *groupClose = new QHGroupBox( frame, "close" );
    groupClose->setTitle( i18n( "General" ) );
    m_closeAfterRipping = new QCheckBox(i18n("Close immediatly after ripping is finished."), groupClose, "check_close");

    Form1Layout->addMultiCellWidget( groupListView, 0, 0, 0, 3 );
    Form1Layout->addMultiCellWidget( groupPattern, 1, 1, 0, 3 );
    Form1Layout->addMultiCellWidget( groupClose, 2, 2, 0, 3 );
    Form1Layout->setColStretch( 0, 20 );

    m_parser = new K3bPatternParser( &m_dirPatternList, &m_filePatternList, m_cddb );

    setButtonOKText( i18n( "Start Ripping" ), i18n( "This starts copying the audio CD.") );

    connect( this, SIGNAL( closeClicked() ), this, SLOT( close() ) );
    connect( m_closeAfterRipping, SIGNAL(clicked() ), this, SLOT( slotCloseAfterRipping() ) );
    connect( this, SIGNAL(okClicked() ), this, SLOT(rip() ) );
    connect(m_useStatic, SIGNAL( clicked() ), this, SLOT( useStatic() ) );
    connect(m_usePattern, SIGNAL( clicked() ), this, SLOT( usePattern() ) );
    connect(m_buttonPattern, SIGNAL(clicked() ), this, SLOT( showPatternDialog() ) );
    connect(m_buttonStaticDir, SIGNAL(clicked()), this, SLOT(slotFindStaticDir()) );
}

K3bRipperWidget::~K3bRipperWidget(){
    if( m_copy !=0 )
        delete m_copy;
}

void K3bRipperWidget::init(){
    if( m_useConfigDirectoryPattern ){
        usePattern();
    } else {
        useStatic();
    }
    KConfig* c = kapp->config();
    c->setGroup("Ripping");
    m_closeAfterRipping->setChecked( c->readBoolEntry( "closeAfterRipping", false ) );
    refresh();
}

void K3bRipperWidget::rip(){
    QStringList::Iterator it;
    int index = 0;
    for( it = m_list.begin(); it != m_list.end(); ++it ){
        QString *tmp = new QString(*it);
        if( m_useCustomDir ){
            // check if directory exists else create it
            //QString dest = m_labelSummaryName->text(); //
            QString dest = m_directories[ index ];
            QDir destDir( dest );
            if( !destDir.exists() ){
                QStringList dirs = QStringList::split("/", dest);
                QStringList::Iterator d = dirs.begin();
                destDir = "/" + *d;
                for( ++d ; d != dirs.end(); ++d ){
                    destDir = destDir.absPath() +"/" + *d;
                    if( destDir.exists( ) )
                        continue;
                    bool successful = destDir.mkdir( destDir.absPath() );
                    if( !successful ){
                        QMessageBox::critical( this, i18n("Ripping Error"), i18n("Couldn't create directory: ") + dest, i18n("Ok") );
                        return;
                    }
                }
            } // end directory
            (*it) = dest + "/" + KIO::encodeFileName(*tmp);
            qDebug("(K3bRipperWidget) Final destination" + (*it) );
        } else {
            (*it) = m_editStaticRipPath->text() + "/" + *tmp;
        }
        m_bytes += m_size[ index ]; //cdda->getRawTrackSize( m_tracks[ index ], drive);
        qDebug("(K3bRipperWidget) KBytes: " + QString::number(m_bytes/1000) );
        index++;
    }
    if( index == 0 ){
        QMessageBox::critical( this, i18n("Ripping Error"), i18n("There is nothing to rip."), i18n("Ok") );
        return;
    }
    // save all entries with artist title, path filename and so on
    setSongList();
    m_copy = new K3bCddaCopy(m_viewTracks->childCount() );
    m_copy->setBytes( m_bytes);
    m_copy->setDrive(m_device);
    m_copy->setCopyTracks(m_tracks);
    m_copy->setCopyFiles(m_list);

    m_ripDialog = new K3bBurnProgressDialog( this, "Ripping" );
    m_ripDialog->setCaption( i18n("Ripping process") );
    m_ripDialog->setJob( m_copy );

    m_copy->start();
    m_ripDialog->exec();

    if( m_closeAfterRipping->isChecked() )
      slotClose();
}

void K3bRipperWidget::useStatic(){
    m_useStatic->setChecked( true );
    m_usePattern->setChecked( false );
    m_buttonPattern->setDisabled( true );
    m_editStaticRipPath->setEnabled( true );
    m_buttonStaticDir->setEnabled( true );
    m_useCustomDir = false;
}
void K3bRipperWidget::usePattern(){
    m_usePattern->setChecked( true );
    m_useStatic->setChecked( false );
    m_buttonPattern->setEnabled( true );
    m_editStaticRipPath->setDisabled( true );
    m_buttonStaticDir->setDisabled( true );
    m_useCustomDir = true;
    QString dir = m_parser->prepareDirectory( m_viewTracks->itemAtIndex( 0 ) );
    dir = m_parser->prepareReplaceDirectory( dir );
    m_labelSummaryName->setText( dir );
}

void K3bRipperWidget::showPatternDialog(){
    K3bFilenamePatternDialog *_dialog = new K3bFilenamePatternDialog(this);
    QStringList::Iterator it = m_titles.at( m_tracks[ 0 ] -1 );
    QString title( KIO::decodeFileName(*it) );
     qDebug("PatternTitle:" + *it);
     _dialog->init( m_album, m_cddb->getArtist(), title, "01" );
    _dialog->show();
}
void K3bRipperWidget::setFilePatternList(QStringList list ){
    m_filePatternList = list;
}

void K3bRipperWidget::setDirPatternList(QStringList list ){
    m_dirPatternList = list;
}

void K3bRipperWidget::setReplacements(bool dir, QString sdir, bool file, QString sfile, bool mixed ){
    m_spaceDir = dir;
    m_spaceFile = file;
    m_parseMixed = mixed;
    m_editFile = sfile;
    m_editDir = sdir;
}

void K3bRipperWidget::refresh(){
    // print it out
    int no = 1;
    // raw file length (wav has +44 byte header data)
    QString filename;
    unsigned int ripTrackIndex= 0;
    m_directories.clear();
    for ( QStringList::Iterator it = m_titles.begin(); it != m_titles.end(); ++it ) {
        if( m_tracks[ripTrackIndex] == no ){
            if( m_usePattern ){
                filename = m_parser->prepareFilename( KIO::decodeFileName(*it), no, m_parseMixed );
                // replace spaces
                if( m_spaceFile )
                    filename = K3bPatternParser::prepareReplaceName( filename, m_editFile, true );
            } else
                filename =  KIO::decodeFileName(*it) + ".wav";
            qDebug("(K3bRipperWidget) Filename: " + filename + ", Index: " +QString::number(ripTrackIndex ) );
            QListViewItem *item = m_viewTracks->itemAtIndex( ripTrackIndex );
            item->setText(5, KIO::decodeFileName( filename ));
            QString refArtist;
            K3bPatternParser::prepareParsedName( KIO::decodeFileName(*it), m_cddb->getArtist(), refArtist, m_parseMixed );
            item->setText(1, refArtist );
            item->setText(2, K3bPatternParser::prepareParsedName( KIO::decodeFileName(*it), m_cddb->getArtist(), refArtist, m_parseMixed ) );
            m_list[ ripTrackIndex ] = filename;
            QString dir = m_parser->prepareDirectory( m_viewTracks->itemAtIndex( ripTrackIndex ) );
            if( m_spaceDir )
                dir = K3bPatternParser::prepareReplaceName( dir, m_editDir, true );
            m_directories.append( dir );
            qDebug("(K3bRipperWidget) Directory: " + m_directories[ ripTrackIndex ] + ", Index: " +QString::number(ripTrackIndex ) );
            m_labelSummaryName->setText( dir );
            ripTrackIndex++;
            if( ripTrackIndex == m_tracks.size() )
                break;
        // add item to cdViewItem
        }
        no++;
    }
    //m_labelSummaryName->setText( dir );
}

void K3bRipperWidget::addTrack(QListViewItem *item ){
    // isn't there a other solution
   KListViewItem *_item = new KListViewItem(m_viewTracks, item->text(0),
   item->text(1), item->text(2), item->text(3), item->text(4), item->text(5) );
}


void K3bRipperWidget::setData( QStringList files, QArray<int> tracks, QArray<long> size){
    m_tracks = tracks;
    m_list = files;
    m_size = size;
}
void K3bRipperWidget::setTrackNumbers(QArray<int> tracks){
    m_tracks = tracks;
}

void K3bRipperWidget::setFileList(QStringList files){
    m_list = files;
}

void K3bRipperWidget::closeEvent( QCloseEvent *e){
    e->accept();
}

void K3bRipperWidget::setSongList(){
    K3bSongManager *sm = k3bMain()->songManager();
    //("/home/ft0001/songlist.xml");
    QStringList::Iterator it;
    int index = 0;
    for( it = m_list.begin(); it != m_list.end(); ++it ){
        int col = (*it).findRev("/");
        QString path = (*it).left( col );
        QListViewItem *item = m_viewTracks->itemAtIndex( index );
        K3bSong song( item->text(5), m_cddb->getAlbum(), item->text(1), item->text(2), m_cddb->get_discid(), item->text(0).toInt() );
        sm->addSong( path, song );
        index++;
        //QString tmpSong = sm.getSong( (*it) ).getFilename();
        //qDebug("Song" + tmpSong);
    }
    sm->save();
}

void K3bRipperWidget::slotFindStaticDir() {
  QString path = KFileDialog::getExistingDirectory( m_editStaticRipPath->text(), this, i18n("Select Ripping Directory") );
  if( !path.isEmpty() ) {
    m_editStaticRipPath->setText( path );
  }
}

void K3bRipperWidget::slotCloseAfterRipping(){
    KConfig* c = kapp->config();
    c->setGroup("Ripping");
    c->writeEntry( "closeAfterRipping", m_closeAfterRipping->isChecked() );
    c->sync();
}

#include "k3bripperwidget.moc"
