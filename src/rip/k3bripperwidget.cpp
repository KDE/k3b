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

#include <kcombobox.h>
#include <klocale.h>
#include <kapp.h>
#include <kconfig.h>
#include <klistview.h>
#include <klineedit.h>
#include <kprogress.h>
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

K3bRipperWidget::K3bRipperWidget(QString device, K3bCddb *cddb, K3bCdView *parent, const char *name )
	: QWidget(0,name)
{
    m_device = device;
    m_cddb = cddb;
    m_cdview = parent;
    m_copy = 0;
    m_bytes = 0;
    m_finalClose = false;
    m_album = m_cddb->getAlbum();
    KConfig* c = kapp->config();
    c->setGroup("Ripping");
    //m_useFilePattern = c->readBoolEntry("useFilePattern", false);
    m_useConfigDirectoryPattern = c->readBoolEntry("usePattern", false);
    m_filePatternList = c->readListEntry("filePattern");
    m_dirPatternList = c->readEntry("dirBasePath");
    m_dirPatternList += c->readEntry("dirGroup1");
    m_dirPatternList += c->readEntry("dirGroup2");
    setupGui();
}

void K3bRipperWidget::setupGui(){
    this->setCaption(i18n("Ripping CD") );
    Form1Layout = new QGridLayout( this );
    Form1Layout->setSpacing( KDialog::spacingHint() );
    Form1Layout->setMargin( KDialog::marginHint() );

    QVGroupBox *groupListView = new QVGroupBox(this, "list" );
    m_viewTracks = new KListView( groupListView, "m_viewTracks" );
    m_viewTracks->addColumn(i18n( "No") );
    m_viewTracks->addColumn(i18n( "Artist") );
    m_viewTracks->addColumn(i18n( "Title") );
    m_viewTracks->addColumn(i18n( "Time") );
    m_viewTracks->addColumn(i18n( "Size") );
    m_viewTracks->addColumn(i18n( "Filename") );

    QGroupBox *groupPattern = new QGroupBox( this, "pattern" );
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

    QHGroupBox *groupProgress = new QHGroupBox( this, "pattern" );
    groupProgress->setTitle( i18n( "Ripping" ) );
    m_progress = new KProgress(groupProgress, "m_progress");
    m_buttonStart = new QPushButton( groupProgress, "m_buttonStart" );
    m_buttonStart->setText( tr( "Start Ripping" ) );

    Form1Layout->addWidget( groupListView, 0, 0 );
    Form1Layout->addWidget( groupPattern, 1, 0 );
    Form1Layout->addWidget( groupProgress, 2, 0 );

    m_parser = new K3bPatternParser( &m_dirPatternList, &m_filePatternList, m_viewTracks , m_cddb );

    connect(m_buttonStart, SIGNAL(clicked() ), this, SLOT(rip() ) );
    connect(m_useStatic, SIGNAL( clicked() ), this, SLOT( useStatic() ) );
    connect(m_usePattern, SIGNAL( clicked() ), this, SLOT( usePattern() ) );
    connect(m_buttonPattern, SIGNAL(clicked() ), this, SLOT( showPatternDialog() ) );

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
}

void K3bRipperWidget::rip(){
    m_buttonStart->setDisabled(true);
    QStringList::Iterator it;
    int index = 0;
    //K3bCdda *cdda = new K3bCdda();
    //struct cdrom_drive *drive = cdda->pickDrive( m_device );
    for( it = m_list.begin(); it != m_list.end(); ++it ){
        QString *tmp = new QString(*it);
        if( m_useCustomDir ){
            // check if directory exists else create it
            QString dest = m_labelSummaryName->text(); //
            // mixed cd can have differnt artist viewTracks->itemAtIndex( index )->text( COLUMN_DIRECTORY );
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
            qDebug("(K3bRipperWidget) Final destination" + (*it) );
            (*it) = dest + "/" + KIO::encodeFileName(*tmp);
        } else {
            (*it) = m_editStaticRipPath->text() + "/" + *tmp;
        }
         m_bytes += m_size[ index ]; //cdda->getRawTrackSize( m_tracks[ index ], drive);
         qDebug("K3bRipperWidget) KBytes: " + QString::number(m_bytes/1000) );
         index++;
    }
    //cdda->closeDrive( drive );
    //delete cdda;
    if( index == 0 ){
        QMessageBox::critical( this, i18n("Ripping Error"), i18n("There is nothing to rip."), i18n("Ok") );
        return;
    }
    m_copy = new K3bCddaCopy(m_viewTracks->childCount() );
    m_copy->setProgressBar(m_progress, m_bytes);
    m_copy->setDrive(m_device);
    m_copy->setCopyTracks(m_tracks);
    m_copy->setCopyFiles(m_list);
/*
#ifdef QT_THREAD_SUPPORT
    qDebug("(K3bRipperWidget) Run thread.");
    m_copy->start();
#else
*/
    if( !m_copy->run() )
            m_buttonStart->setEnabled( true );
    connect(m_copy, SIGNAL(endRipping() ), SLOT( waitForClose() ));
//#endif
}

void K3bRipperWidget::useStatic(){
    m_useStatic->setChecked( true );
    m_usePattern->setChecked( false );
    m_buttonPattern->setDisabled( true );
    m_editStaticRipPath->setEnabled( true );
    m_useCustomDir = false;
}
void K3bRipperWidget::usePattern(){
    m_usePattern->setChecked( true );
    m_useStatic->setChecked( false );
    m_buttonPattern->setEnabled( true );
    m_editStaticRipPath->setDisabled( true );
    m_useCustomDir = true;
    QString dir = m_parser->prepareDirectory( m_viewTracks->itemAtIndex( 0 ) );
    qDebug("dir: " + dir);
    dir = m_parser->prepareReplaceDirectory( dir );
    qDebug("dir: " + dir);
    m_labelSummaryName->setText( dir );
}

void K3bRipperWidget::showPatternDialog(){
    K3bFilenamePatternDialog *_dialog = new K3bFilenamePatternDialog(this);
    _dialog->init(m_viewTracks->itemAtIndex( 0 ), m_album);
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
    m_titles = m_cddb->getTitles();
    // print it out
    int no = 1;
    // raw file length (wav has +44 byte header data)
    QString filename;
    int ripTrackIndex= 0;

    for ( QStringList::Iterator it = m_titles.begin(); it != m_titles.end(); ++it ) {
        if( m_tracks[ripTrackIndex] == no ){
            if( m_usePattern ){
                filename = m_parser->prepareFilename( *it, no );
                // replace spaces
                if( m_spaceFile )
                    filename = K3bPatternParser::prepareReplaceName( filename, m_editFile, true );
            } else
                filename =  KIO::decodeFileName(*it) + ".wav";
            qDebug("(K3bRipperWidget) Filename: " + filename + ", Index: " +QString::number(ripTrackIndex ) );
            QListViewItem *item = m_viewTracks->itemAtIndex( ripTrackIndex );
            item->setText(5, KIO::decodeFileName( filename ));
            m_list[ ripTrackIndex ] = filename;
            ripTrackIndex++;
            if( ripTrackIndex == m_tracks.size() )
                break;
        // add item to cdViewItem
        }
        no++;
    }
    // at the moment we don't support proper handling of mixed cds
    QString dir = m_parser->prepareDirectory( m_viewTracks->itemAtIndex( 0 ) );
    if( m_spaceDir )
        dir = K3bPatternParser::prepareReplaceName( dir, m_editDir, true );
    m_labelSummaryName->setText( dir );
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

// this is a really strange solution, but i don't know any other soluten
void K3bRipperWidget::closeEvent( QCloseEvent *e){
    qDebug("(K3bRipperWidget) Interrupting ripping process.");
/*
#ifdef QT_THREAD_SUPPORT
    if(m_copy != 0){
        m_copy->setFinish(true);
        if (m_copy->finished() ){
            m_cdview->setEnabled(true);
            qDebug("(K3bRipperWidget) Exit.");
            //delete m_copy;
            e->accept();
        } else {
            waitForClose();
        }
    } else {
        m_cdview->setEnabled(true);
        e->accept();
    }
#else
*/
    m_cdview->setEnabled(true);
    if( m_copy != 0)
        m_copy->setFinish(true);
        if( m_copy->finished() )
            m_finalClose = true;
    else
        // no rip -> no wait
        m_finalClose = true;
    if( m_finalClose ){
        qDebug("(K3bRipperWidget) Exit.");
        e->accept();
    }
//#endif
}

void K3bRipperWidget::waitForClose(){
/*
#ifdef QT_THREAD_SUPPORT
    qDebug("(K3bRipperWidget) Wait for shuting down.");
    if ( m_copy->running() ){
        QTimer::singleShot(350, this, SLOT(waitForClose()) );
    }
    if ( m_copy->finished() ){
         qDebug("(K3bRipperWidget) Close finally.");
         close();
    }
#endif
*/
    m_finalClose=true;
    close();
}
