/***************************************************************************
                          k3bcdview.cpp  -  description
                             -------------------
    begin                : Sun Oct 28 2001
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

#include "k3bcdview.h"
#include "k3bcddb.h"
#include "k3bcdda.h"
#include "k3bcddacopy.h"
#include "../k3b.h"
#include "../k3bglobals.h"
#include "../k3bripperwidget.h"

#include <qwidget.h>
#include <qlayout.h>
#include <qdir.h>
#include <qthread.h>
#include <qarray.h>

#include <kaction.h>
#include <kiconloader.h>
#include <kicontheme.h>
#include <ktoolbar.h>
#include <ktoolbarbutton.h>
#include <kapp.h>
#include <kconfig.h>
#include <klocale.h>
#include <kdockwidget.h>
#include <klistview.h>


#define RELOAD_BUTTON_INDEX         0
#define GRAB_BUTTON_INDEX             1
#define SELECTION_BUTTON_INDEX    2
//#define DEFAULT_CDROM                 "/dev/cdrom"
#define DEFAULT_CDDB_HOST           "localhost:888"
#define COLUMN_FILENAME     5
#define COLUMN_NUMBER       0

K3bCdView::K3bCdView(QWidget *parent, const char *name=0)
        : QVBox(parent, name){
    m_initialized=false;
}

K3bCdView::~K3bCdView(){
}

void K3bCdView::setupGUI(){
    KToolBar *toolBar = new KToolBar( k3bMain(), this, "cdviewtoolbar" );
    m_listView = new KListView(this, "cdviewcontent");
    m_listView->addColumn(i18n( "No") );
    m_listView->addColumn(i18n( "Artist") );
    m_listView->addColumn(i18n( "Title") );
    m_listView->addColumn(i18n( "Time") );
    m_listView->addColumn(i18n( "Size") );
    m_listView->addColumn(i18n( "Filename") );
    m_listView->setItemsRenameable( false );
    m_listView->setSelectionMode(QListView::Multi);
    m_listView->setShowSortIndicator(true);
    m_listView->setAllColumnsShowFocus(true);

    KIconLoader *_il = new KIconLoader("k3b");
    toolBar->insertButton( _il->iconPath("reload", KIcon::Toolbar), 	RELOAD_BUTTON_INDEX);
    toolBar->insertButton( _il->iconPath("editcopy", KIcon::Toolbar), 	GRAB_BUTTON_INDEX);
    toolBar->insertButton( _il->iconPath("view_choose", KIcon::Toolbar), 	SELECTION_BUTTON_INDEX);
    KToolBarButton *_buttonGrab = toolBar->getButton(GRAB_BUTTON_INDEX);
    KToolBarButton *_buttonReload = toolBar->getButton(RELOAD_BUTTON_INDEX);
    KToolBarButton *_buttonSelectionMode = toolBar->getButton(SELECTION_BUTTON_INDEX);
    _buttonSelectionMode->setText(i18n("Selection Mode") );

    m_cddb = new K3bCddb(  );
    m_cdda = new K3bCdda();
    // connect to the actions
    connect( _buttonReload, SIGNAL(clicked()), this, SLOT(reload()) );
    connect( _buttonGrab, SIGNAL(clicked()), this, SLOT(prepareRipping()) );
    connect( _buttonSelectionMode, SIGNAL(clicked()), this, SLOT(changeSelectionMode()) );
}

void K3bCdView::show(){
    if( !m_initialized){
        m_initialized=true;
        setupGUI();
    }
    QWidget::show();
}

void K3bCdView::showCdContent(struct cdrom_drive *drive ){
    // read cddb settings each time to get changes in optiondialog
    applyOptions();
    // clear old entries
    m_listView->clear();
    m_cddb->updateCD( drive );
    QStringList titles = m_cddb->getTitles();
    // print it out
    int no = 1;
    // raw file length (wav has +44 byte header data)
    long totalByteCount = 0;
    QString filename;
    // TODO: add filename and length
    for ( QStringList::Iterator it = titles.begin(); it != titles.end(); ++it ) {
        totalByteCount = m_cdda->getRawTrackSize(no, drive);
        filename = prepareFilename( (*it).latin1() );
        // add item to cdViewItem
        addItem(no, m_cddb->getArtist(), (*it).latin1(), K3b::sizeToTime(totalByteCount), totalByteCount, filename);
        no++;
    }
}

void K3bCdView::showCdView(QString device){
    if( this->isEnabled() ){
        m_device = device;
        m_drive = m_cdda->pickDrive(m_device);
        showCdContent(m_drive);
        m_cdda->closeDrive(m_drive);
    }
}

// ===========  slots ==============
void K3bCdView::reload(){
    m_drive = m_cdda->pickDrive(m_device);
    qDebug("(K3bCdView) Reload");
    showCdContent(m_drive);
    m_cdda->closeDrive(m_drive);
}

void K3bCdView::grab(){
    /*
    m_drive = m_cdda->pickDrive(m_device);
    K3bCddaCopy *_copy = new K3bCddaCopy(arraySize);
    _copy->setDrive(m_drive);
    _copy->setCopyTracks(tracklist);
    _copy->setCopyFiles(filelist);
    _copy->start();
*/
}

void K3bCdView::changeSelectionMode(){
    if (m_listView->isMultiSelection() ){
            m_listView->setSelectionMode(QListView::Extended);
    } else {
            m_listView->setSelectionMode(QListView::Multi);
    }
}

void K3bCdView::prepareRipping(){
    K3bRipperWidget *rip = new K3bRipperWidget(m_device, this);
    rip->show();
    qDebug("(K3bCdView) show ripperwidget.");
    QListViewItem *item;
    QList<QListViewItem> selectedList = m_listView->selectedItems();
    int arraySize = selectedList.count();
    QStringList filelist;
    QArray<int> tracklist(arraySize);
    int index=0;
    for ( item=selectedList.first(); item != 0; item=selectedList.next() ){
        filelist.append( item->text(COLUMN_FILENAME) );
        tracklist[index] = item->text(COLUMN_NUMBER).toInt();
        qDebug("filelist:" + filelist[index]);
        index++;
        qDebug("(K3bCdView) add song.");
        rip->addTrack( item );
    }
    rip->setFileList(filelist);
    rip->setTrackNumbers(tracklist);
    this->setDisabled(true);
}
/*
void K3bCdView::copied(){
    qDebug("(K3bCdView) get copied signal.");
    m_cdda->closeDrive(m_drive);
}
*/
//  helpers
// -----------------------------------------
void K3bCdView::addItem(int no, QString artist, QString title, QString time, long length, QString filename){
    QString number;
    QString size;
    if (no < 10)
        number = "0" + QString::number(no);
    else
        number = QString::number(no);
    size = QString::number( (double) length / 1000000, 'g', 3) + " MB";
    KListViewItem *song = new KListViewItem(m_listView, number, artist, title, time, size, filename);
}

void K3bCdView::applyOptions(){
    KConfig *c = kapp->config();
    c->setGroup("Cddb");
    bool useCddb = c->readBoolEntry("useCddb", false);
    QString hostString = c->readEntry("cddbServer", DEFAULT_CDDB_HOST);
    int index = hostString.find(":");
    QString server = hostString.left(index);
    unsigned int port = hostString.right(hostString.length()-index-1).toUInt();
    qDebug("(K3bCdView) CddbServer: " + server + ":" + QString::number(port) );
    m_cddb->setServer(server);
    m_cddb->setPort(port);
    m_cddb->setUseCddb(useCddb);
}

QString K3bCdView::prepareFilename(QString filename){
    /*
    // parse filename and pattern it.
    qDebug("(K3bCdView) prepare filename: " + filename);
    const char *str = filename.latin1();
    //int index = str->find("/");
    char *dest = new char[100];
    const char *a =  strcpy(dest, filename.latin1() );
//    qDebug("K3bCdview) index: " + filename.utf8() );
//    qDebug("K3bCdview) index: " + filename.local8Bit() );
    QString *b = new QString(a);
    qDebug("K3bCdview) index: %s", a );
    qDebug("K3bCdview) index: " + *b );
    QString tmp = filename;
//    tmp += "/";
//    tmp += tmp.right(filename.length() - index );

    //qDebug("(K3bCdView) prepare filename: " + tmp);
    */
    return filename + ".wav";
}

struct cdrom_drive* K3bCdView::pickDrive(QString device){
    m_device = device;
    return m_cdda->pickDrive(device);
}
