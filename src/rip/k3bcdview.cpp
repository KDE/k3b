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
#include "../k3bcddb.h"
#include "k3bcdda.h"
#include "k3bcddacopy.h"
#include "../k3b.h"
#include "../device/k3bdevicemanager.h"
#include "../device/k3bdevice.h"
#include "../k3bglobals.h"
#include "k3bripperwidget.h"
#include "k3bfilenamepatterndialog.h"
#include "k3bpatternparser.h"

#include <qwidget.h>
#include <qlayout.h>
#include <qmessagebox.h>
#include <qarray.h>
#include <qfile.h>

#include <kiconloader.h>
#include <ktoolbar.h>
#include <ktoolbarbutton.h>
#include <kapp.h>
#include <kconfig.h>
#include <klocale.h>
#include <klistview.h>
#include <kpopupmenu.h>
#include <kio/global.h>

extern "C" {
#include "../libwm/include/workman.h"
}


#define RELOAD_BUTTON_INDEX         0
#define GRAB_BUTTON_INDEX             1
#define SELECTION_BUTTON_INDEX    2
#define PLAY_BUTTON_INDEX              3
#define STOP_BUTTON_INDEX              4
//#define DEFAULT_CDROM                 "/dev/cdrom"
#define DEFAULT_CDDB_HOST           "freedb.org:888"
#define COLUMN_NUMBER       0
#define COLUMN_ARTIST         1
#define COLUMN_FILENAME     5

#define ID_PATTERN                 0
#define ID_PLAYSONG              1

K3bCdView::K3bCdView(QWidget *parent, const char *name=0)
  : QVBox(parent, name){
  m_initialized=false;
  readSettings();
}

K3bCdView::~K3bCdView(){
  if( m_initialized ){
    delete m_cddb;
    delete m_cdda;
  }
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
  toolBar->insertButton( _il->iconPath("1rightarrow", KIcon::Toolbar), 	PLAY_BUTTON_INDEX);
  toolBar->insertButton( _il->iconPath("player_stop", KIcon::Toolbar), 	STOP_BUTTON_INDEX);
  KToolBarButton *_buttonGrab = toolBar->getButton(GRAB_BUTTON_INDEX);
  KToolBarButton *_buttonReload = toolBar->getButton(RELOAD_BUTTON_INDEX);
  KToolBarButton *_buttonSelectionMode = toolBar->getButton(SELECTION_BUTTON_INDEX);
  _buttonSelectionMode->setText(i18n("Selection Mode") );
  KToolBarButton *_buttonPlay = toolBar->getButton( PLAY_BUTTON_INDEX);
  KToolBarButton *_buttonStop = toolBar->getButton( STOP_BUTTON_INDEX);

  m_cddb = new K3bCddb(  );
  m_cdda = new K3bCdda();
  m_parser = new K3bPatternParser( &m_dirPatternList, &m_filePatternList, m_cddb );
  // connect to the actions
  connect( _buttonReload, SIGNAL(clicked()), this, SLOT(reload()) );
  connect( _buttonGrab, SIGNAL(clicked()), this, SLOT(prepareRipping()) );
  connect( _buttonSelectionMode, SIGNAL(clicked()), this, SLOT(changeSelectionMode()) );
  connect( _buttonPlay, SIGNAL(clicked()), this, SLOT( play() ) );
  connect( _buttonStop, SIGNAL(clicked()), this, SLOT( stop() ) );
  connect( m_listView, SIGNAL(rightButtonClicked ( QListViewItem *, const QPoint &, int )), this, SLOT(slotMenuActivated(QListViewItem*, const QPoint &, int) ) );
}

void K3bCdView::show(){
  if( !m_initialized){
    m_initialized=true;
    setupGUI();
  }
  QWidget::show();
}

void K3bCdView::checkView( ){
  // read cddb settings each time to get changes in optiondialog
  applyOptions();
  m_cddb->updateCD( m_drive );
  m_titles = m_cddb->getTitles();
  int type = checkCDType( m_titles );
  switch( type ){
  case 0:
    showCdContent();
    break;
  case 1:
    emit notSupportedDisc(m_device);
    break;
  case 2:
    askForView();
    break;
  default:
    break;
  }
}

void K3bCdView::askForView(){
  switch( QMessageBox::information( this, i18n("Select View"), i18n("The cd you have insert is a mixed CD. Do you want to show the audio or data tracks?"), i18n("Audio"), i18n("Data") ) ) {
  case 0:
    showCdContent();
    break;
  case 1:
    emit notSupportedDisc( m_device );
    break;
  default:
    break;
  }
}
void K3bCdView::showCdContent( ){
  m_listView->clear();
  m_album = m_cddb->getAlbum();
  // print it out
  int no = 1;
  // raw file length (wav has +44 byte header data)
  //long totalByteCount = 0;
  QString filename;
  int arraySize = m_titles.count();
  m_size = new QArray<long>( arraySize );
  for ( QStringList::Iterator it = m_titles.begin(); it != m_titles.end(); ++it ) {
    m_size->at(no-1) = m_cdda->getRawTrackSize(no, m_drive);
    if( m_usePattern ){
      filename = m_parser->prepareFilename( *it, no );
      filename = K3bPatternParser::prepareReplaceFilename( filename );
    } else
      filename =  KIO::decodeFileName(*it) + ".wav";
    // add item to cdViewItem
    long length = m_size->at(no-1);
    addItem(no, m_cddb->getArtist(), KIO::decodeFileName(*it), K3b::sizeToTime( length ), length, filename);
    no++;
  }
}

void K3bCdView::showCdView(const QString& device)
{
  if( isEnabled() ) {
    m_device = device;
    reload();
  }
}

void K3bCdView::readSettings( ){
  KConfig* c = kapp->config();
  c->setGroup("Ripping");
  m_usePattern = c->readBoolEntry("usePattern", false);
  m_filePatternList = c->readListEntry("filePattern");
  m_dirPatternList = c->readEntry("dirBasePath");
  m_dirPatternList += c->readEntry("dirGroup1");
  m_dirPatternList += c->readEntry("dirGroup2");
}

// ===========  slots ==============
void K3bCdView::reload(){
  readSettings();
  // clear old entries
  m_listView->clear();
  m_drive = m_cdda->pickDrive(m_device );
  int result = K3bCdda::driveType( m_drive );
  qDebug("(K3bCdView) CD type: (Audio=0, Data/DVD=1, Audio/Data=2) %i", result );
  if( result == 0 || result == 2 ){
        qDebug("(K3bCdView) Reload");
        checkView();
        m_cdda->closeDrive( m_drive);
  } else {
    m_cdda->closeDrive( m_drive);
    emit notSupportedDisc( m_device );
  }
}

void K3bCdView::slotMenuItemActivated(int itemId){
  switch( itemId ){
  case ID_PATTERN: {
    /*K3bFilenamePatternDialog *_dialog = new K3bFilenamePatternDialog(this);
      _dialog->init(m_testItemPattern, m_album);
      _dialog->show();*/
  }
    break;
  case ID_PLAYSONG:
    break;
  default:
    break;
  }
}

void K3bCdView::play(){
  qDebug("(K3bCdView) play");
  // the sgx devices dont work, must use the real device i.e sr0, scd0
  //    cd_device = (char *)qstrdup(QFile::encodeName("/dev/cdrom"));
  K3bDevice* dev = k3bMain()->deviceManager()->deviceByName( m_device );
  if( dev ) {
    cd_device = (char*)(dev->ioctlDevice().latin1());    // Aaaaaarghhhh!!!!!
    qDebug( "(K3bCdView) playing cd in device %s", cd_device );
  }
  else {
    qDebug("(K3bCdView) Could not open cd device " + m_device );
    return;
  }
  wm_drive *drive = find_drive_struct();
  if( drive == 0 ) {
    qDebug("(K3bCdView) could not wmfind device %s", cd_device );
    return;
  }

  if( wmcd_open(drive) != 0 ) {
    qDebug("(K3bCdView) could not wmopen device %s", cd_device );
    return;
  }
  // this call also initializes the cd drive and all the stuff
  int i = wm_cd_status();
  qDebug("(K3bCdView) Status of cd: %i", i );
  if( i == 0 ){
    QMessageBox::critical( this, i18n("Player Error"), i18n("Sorry, this is only a demonstration which may not work."), i18n("Ok") );
    return;
  }
  QList<QListViewItem> items = m_listView->selectedItems();
  int itemIndex = 1;
  if( !items.isEmpty() ){
    itemIndex = m_listView->itemIndex( items.first() ) + 1;
    qDebug("(K3bCdView) Play first song in selected list.");
  } else {
    qDebug("(K3bCdView) Play first song of audio cd.");
  }
  wm_cd_play( itemIndex, 0, itemIndex+1);
  qDebug("(K3bCdView) Have started playing song.");
  //qDebug("(K3bCdView) Check for CD-TEXT.");
  wm_cd_status();
  //int stat = -1;
  //stat = wm_get_cdtext( drive );
  //wm_cd_get_cdtext( );
  //qDebug("Index" + QString::number( stat ) + ", " + QString::number(wm_cdtext_info.count_of_entries) );
  /*qDebug("Index" + QString::number(wm_cdtext_info.count_of_valid_packs) );
  qDebug("Index" + QString::number(wm_cdtext_info.count_of_invalid_packs) );
  qDebug("Valid" + QString::number(wm_cdtext_info.valid) );
  //struct cdtext_info_block *cdtext[8];
  //cdtext = wm_cdtext_info.blocks;
  //if( wm_cdtext_info.valid ){
  /*     QString().sprintf("%s", (const char*)(wm_cdtext_info.blocks[0]->block_unicode ) );
        int at;
        for (at = 1 ; at < (wm_cdtext_info.count_of_entries); ++at ) {
            qDebug("Entries: " + QString().sprintf("%02d: %s", at, wm_cdtext_info.blocks[0]->name[at]) );
        }
  //}
/*        tracktitlelist.append(QString().sprintf("%s / %s", (const char*)(wm_cdtext_info.blocks[0]->name[0]),(const char*)(wm_
        titlelabel->setText(QString((const char*)(wm_cdtext_info.blocks[0]->name[1])));\
        artistlabel->setText(tracktitlelist.first());\
        songListCB->clear();\
        for (at = 1 ; at < (wm_cdtext_info.count_of_entries); ++at ) {\
            songListCB->insertItem( QString().sprintf("%02d: %s", at, wm_cdtext_info.blocks[0]->name[at]));\
            tracktitlelist.append((const char*)(wm_cdtext_info.blocks[0]->name[at]));\
        }\
        for(; at < cur_ntracks; ++at){\
            songListCB->insertItem( QString::fromUtf8( QCString().sprintf(i18n("%02d: <Unknown>").utf8(), at)));\
        }\
  */
  //wm_free_cdtext();
  //i= wm_cd_eject();
  //qDebug("cd eject: %i" ,i );
}

void K3bCdView::stop(){
  wm_cd_stop();
}

void K3bCdView::slotMenuActivated( QListViewItem *_item, const QPoint &point, int id){
  /*KPopupMenu *_popup = new KPopupMenu(this, "editmenu");
    _popup->setTitle(i18n("Helpers") );
    _popup->insertItem(i18n("Filename Pattern"), ID_PATTERN, 1 );
    _popup->insertItem(i18n("Play Song"), ID_PLAYSONG, 2 );
    _popup->popup(point);
    if( _item->isSelected() )
    _item->setSelected(false);
    else
    _item->setSelected(true);
    m_testItemPattern = _item;
    connect( _popup, SIGNAL( activated(int)), this, SLOT(slotMenuItemActivated(int) ));
  */
}

void K3bCdView::changeSelectionMode(){
  if (m_listView->selectionMode() == QListView::Multi ){
    m_listView->setSelectionMode(QListView::Extended);
  } else {
    m_listView->setSelectionMode(QListView::Multi);
  }
}

void K3bCdView::prepareRipping(){
  QList<QListViewItem> selectedList = m_listView->selectedItems();
  if( selectedList.isEmpty() ){
    QMessageBox::critical( this, i18n("Ripping Error"), i18n("Please select the title to rip."), i18n("Ok") );
    return;
  }
  K3bRipperWidget *rip = new K3bRipperWidget(m_device, m_cddb );
  //K3bRipperWidget *rip = new K3bRipperWidget(m_device, m_cddb, this);
  rip->show();
  qDebug("(K3bCdView) show ripperwidget.");
  QListViewItem *item;
  int arraySize = selectedList.count();
  QStringList filelist;
  QArray<int> tracklist(arraySize);
  QArray<long> sizelist(arraySize);
  int index=0;

  for ( item=selectedList.first(); item != 0; item=selectedList.next() ){
    filelist.append( item->text(COLUMN_FILENAME) );
    tracklist[index] = item->text(COLUMN_NUMBER).toInt();
    sizelist[index] = m_size->at(item->text(COLUMN_NUMBER).toInt()-1 );
    index++;
    qDebug("(K3bCdView) add song.");
    rip->addTrack( item );
  }
  rip->setData( filelist, tracklist, sizelist);
  rip->init();
  //this->setDisabled(true);
}

int  K3bCdView::checkCDType( QStringList titles ){
  unsigned int dataIndex=0;
  unsigned int audioIndex=0;

  for ( QStringList::Iterator it = titles.begin(); it != titles.end(); ++it ) {
    if( (*it).find("data") >=0 )
      ++dataIndex;
    else
      ++audioIndex;
  }
  qDebug("(K3bCdView) Audio: %i, Data %i index.", audioIndex, dataIndex);
  if( dataIndex > 0 && audioIndex > 0){
    return 2; // mixed cd
  } else if( dataIndex > 0 ){
    return 1; // dataCD
  } else {
    return 0; // audioCD
  }
}

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

#include "k3bcdview.moc"
