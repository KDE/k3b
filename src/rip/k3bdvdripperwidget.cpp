/*
 *
 * $Id$
 * Copyright (C) 2002 Thomas Froescher <tfroescher@k3b.org>
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


#include "k3bdvdripperwidget.h"
#include "k3bdvdcontent.h"
#include "k3bdvdcopy.h"
#include "k3bdvdfilldisplay.h"
#include "k3bdvdextraripstatus.h"
#include "k3bdvdrippingprocess.h"
#include <k3bjobprogressdialog.h>
#include <device/k3bdevicemanager.h>
#include <device/k3bdevice.h>
#include <k3bexternalbinmanager.h>
#include "../videoEncoding/k3bdivxcodecdata.h"
#include "../videoEncoding/k3bdivxview.h"
#include <k3bcore.h>

#include <qlayout.h>
#include <qvgroupbox.h>
#include <qhgroupbox.h>
#include <qfont.h>
#include <qtoolbutton.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qdatastream.h>
#include <qdir.h>
#include <qfile.h>

#include <kaction.h>
#include <kconfig.h>
#include <klocale.h>
#include <kapplication.h>
#include <kdialog.h>
#include <kfiledialog.h>
#include <klineedit.h>
#include <kprocess.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kcombobox.h>
#include <kurlrequester.h>

// OS determination and specific includes. Unsupported systems
// will fail on the statfs() call below. Per-OS support is as
// follows:
// 	Linux	- the original k3b authors
//	FreeBSD	- kde@freebsd.org or groot@kde.org
#include <qglobal.h>

#ifdef Q_OS_LINUX
#include <sys/vfs.h>
#endif

#ifdef Q_OS_FREEBSD
#include <sys/param.h>
#include <sys/mount.h>
#endif

K3bDvdRipperWidget::K3bDvdRipperWidget(const QString& device, QWidget *parent, const char *name )
  : K3bInteractionDialog( parent, name,
			  i18n("Ripping DVD") )
{
  m_device = k3bcore->deviceManager()->deviceByName( device )->blockDeviceName();
  m_bytes = 0;
  m_finalClose = false;
  m_openEncoding = false;

  m_ripDialog = 0;
  setupGui();
  m_startEncoding = false;
  m_ripProcess = 0;
  m_ripJob = 0;

  slotLoadUserDefaults();
}

K3bDvdRipperWidget::~K3bDvdRipperWidget(){
}

void K3bDvdRipperWidget::setupGui(){
    setMinimumWidth( 500 );
    QWidget *frame = mainWidget();
    QGridLayout *mainLayout = new QGridLayout( frame );

    QGroupBox *groupPattern = new QGroupBox( i18n( "Destination Directory" ), frame, "pattern" );
    groupPattern->setColumnLayout(1, Qt::Vertical ); // 0,
    groupPattern->setInsideMargin( marginHint() );
    m_editStaticRipPath = new KURLRequester( groupPattern, "staticeditpattern" );
    m_editStaticRipPath->setMode( KFile::Directory );

    //QGridLayout *dirsLayout = new QGridLayout( groupPattern->layout() );
    //dirsLayout->setSpacing( KDialog::spacingHint() );
    //dirsLayout->setMargin( KDialog::marginHint() );
    //m_editStaticRipPath = new KLineEdit(groupPattern, "staticeditpattern");
    //m_editStaticRipPath->setText( QDir::homeDirPath() );
    //m_buttonStaticDir = new QToolButton(  groupPattern, "m_buttonStaticDir" );
    //m_buttonStaticDir->setIconSet( SmallIconSet( "fileopen" ) );

    //dirsLayout->addMultiCellWidget( m_editStaticRipPath, 0,0,0,2 );
    //dirsLayout->addMultiCellWidget( m_buttonStaticDir, 0,0,3,3 );

    QGroupBox *ripOptions = new QGroupBox( i18n( "Options" ), frame, "ripOptions" );
    ripOptions->setColumnLayout(0, Qt::Vertical );
    QGridLayout *optionsLayout = new QGridLayout( ripOptions->layout() );
    optionsLayout->setSpacing( KDialog::spacingHint() );
    optionsLayout->setMargin( KDialog::marginHint() );
    m_checkOpenEncoding = new QCheckBox( i18n( "Open encoding dialog after ripping." ), ripOptions );
    m_checkStartEncoding = new QCheckBox( i18n( "Start encoding with default settings after ripping." ), ripOptions );
    // TODO
    m_checkStartEncoding->setEnabled( false );
    m_comboAngle = new KComboBox( FALSE, ripOptions, "comboRipAngle" );
    m_labelAngle = new QLabel(i18n("Select the angle to rip:"), ripOptions);
    optionsLayout->addMultiCellWidget( m_labelAngle, 0, 0, 0, 0);
    optionsLayout->addMultiCellWidget( m_comboAngle, 0, 0, 1, 1);
    optionsLayout->addMultiCellWidget( m_checkOpenEncoding, 1, 1, 0, 0);
    optionsLayout->addMultiCellWidget( m_checkStartEncoding, 2, 2, 0, 0);

    QGroupBox *groupSize = new QGroupBox( i18n( "Available Space" ), frame, "size" );
    groupSize->setColumnLayout(0, Qt::Vertical );
    QGridLayout *sizeLayout = new QGridLayout( groupSize->layout() );
    sizeLayout->setSpacing( KDialog::spacingHint() );
    sizeLayout->setMargin( KDialog::marginHint() );
    m_fillDisplay = new K3bDvdFillDisplay( groupSize );
    m_fillDisplay->setFixedHeight( 20 );
    m_hardDiskSpace = new QLabel( groupSize );
    sizeLayout->addMultiCellWidget( m_fillDisplay, 0, 0, 0, 1 );
    sizeLayout->addMultiCellWidget( m_hardDiskSpace, 0, 0, 2, 2 );
    sizeLayout->setColStretch( 0, 20 );

    mainLayout->addMultiCellWidget( groupPattern, 0, 0, 0, 1 );
    mainLayout->addMultiCellWidget( groupSize, 1, 1, 0, 1 );
    mainLayout->addMultiCellWidget( ripOptions, 2, 2, 0, 1 );

    setStartButtonText( i18n( "Start Ripping" ), i18n( "This starts the DVD copy.") );
    connect( this, SIGNAL( closeClicked() ), this, SLOT( close() ) );
    connect( this, SIGNAL( startClicked() ), this, SLOT( rip() ) );
    //connect(m_buttonStaticDir, SIGNAL(clicked()), this, SLOT(slotFindStaticDir()) );
    connect(m_checkOpenEncoding, SIGNAL( stateChanged( int ) ), this, SLOT( slotCheckOpenEncoding( int ) ));
    connect(m_checkStartEncoding, SIGNAL( stateChanged( int ) ), this, SLOT( slotCheckStartEncoding( int ) ));
    //connect(m_buttonStaticDirVob, SIGNAL(clicked()), this, SLOT(slotFindStaticDirVob()) );
    //connect(m_buttonStaticDirTmp, SIGNAL(clicked()), this, SLOT(slotFindStaticDirTmp()) );
    connect(m_editStaticRipPath, SIGNAL( textChanged( const QString& )), this, SLOT( slotSetDependDirs( const QString& )) );
}

void K3bDvdRipperWidget::init( const QValueList<K3bDvdContent>& list ){
    m_ripTitles.clear();
    m_ripTitles = list;
    checkSize();
    DvdTitle::Iterator dvd;
    // currently only one title to rip supported
    dvd = m_ripTitles.begin( );
    m_comboAngle->insertStringList( *(*dvd).getAngles() );
    if( m_comboAngle->count() == 1 ){
        m_comboAngle->setDisabled( true );
        m_labelAngle->setDisabled( true );
    }
}

void K3bDvdRipperWidget::rip(){
    QString path = m_editStaticRipPath->url();
    if( path.endsWith("/") )
        m_editStaticRipPath->setURL( path.left( path.length()-1 ) );

  if( !m_enoughSpace ) {
    KMessageBox::error( this, i18n("Not enough space left in %1").arg(m_editStaticRipPath->url()) );
    return;
  }
  if( !createDirs() ){
    return;
  }

  m_ripJob = new K3bDvdCopy( m_device, 
			     m_editStaticRipPath->url(), 
			     m_editStaticRipPath->url()+"/vob", 
			     m_editStaticRipPath->url()+"/tmp", 
			     m_ripTitles, 
			     this );
  m_ripJob->setSettings( m_vobSize, m_comboAngle->currentText() );
  
  if( !m_ripDialog ) {
    m_ripDialog = new K3bJobProgressDialog( kapp->mainWidget(), "Ripping", false );
    K3bDvdExtraRipStatus *ripStatus = new K3bDvdExtraRipStatus( m_ripDialog );
    connect( m_ripJob, SIGNAL( dataRate( float )), ripStatus, SLOT( slotDataRate( float )) );
    connect( m_ripJob, SIGNAL( estimatedTime( unsigned int )), 
	     ripStatus, SLOT( slotEstimatedTime( unsigned int )) );
    connect( m_ripJob, SIGNAL( finished( bool )), this, SLOT( slotOpenEncoding( bool )) );
    m_ripDialog->setExtraInfo( ripStatus );
    connect( m_ripDialog, SIGNAL( closeClicked()), this, SLOT( slotRipJobDeleted() ));
  }

  hide(); 

  m_ripDialog->startJob( m_ripJob );
  //} 

  delete m_ripJob;
}

void K3bDvdRipperWidget::slotRipJobDeleted(){
    kdDebug() << "(K3bDvdRipperWidget) Rip job finished/interrupted." << endl;
    m_ripJob->ripFinished( true );
    m_ripDialog->close();
    delete m_ripJob;
    delete m_ripProcess;
    m_ripJob = 0;
    m_ripProcess = 0;
}

void K3bDvdRipperWidget::closeEvent( QCloseEvent *e){
    e->accept();
}


void K3bDvdRipperWidget::slotFindStaticDir() {
    QString path = KFileDialog::getExistingDirectory( m_editStaticRipPath->url(), this, i18n("Select Ripping Directory") );
    if( !path.isEmpty() ) {
        m_editStaticRipPath->setURL( path );
        slotSetDependDirs( path );
    }
}

void K3bDvdRipperWidget::slotSetDependDirs( const QString& p ) {    
    // get real directory of /home/me/abcdef -> /home/me
    QString tmp = p;
    QDir space( p ); // add / to be sure
    if( !space.exists() ){
        int index = p.findRev("/");
        kdDebug() << "P: " << p << endl;
        tmp = p.left( index+1 );
        kdDebug() << "(K3bDvdRipperWidget) new directory. Check existing: " << tmp << endl;
    }
    struct statfs fs;
    ::statfs( QFile::encodeName( tmp ), &fs );
    unsigned int kBfak = (unsigned int)(fs.f_bsize/1024);
    slotFreeTempSpace( tmp, fs.f_blocks*kBfak, (fs.f_blocks-fs.f_bfree)*kBfak, fs.f_bavail*kBfak );
}

void K3bDvdRipperWidget::slotFreeTempSpace( const QString &, unsigned long kBSize,
        unsigned long kBUsed, unsigned long kBAvail ){
    m_fillDisplay->setKbSize( kBSize );
    m_fillDisplay->setKbAvailable( kBAvail );
    m_fillDisplay->setKbUsed( kBUsed );
    kdDebug() << "(K3bDvdRipperWidget) DVD VobSize: " << m_vobSize << ", KBAvail: " << kBAvail << endl;
    m_fillDisplay->setKbDvd( (unsigned long) (m_vobSize/1000) );
     if( kBAvail > m_vobSize/1000 ) {
        m_enoughSpace = true;
    } else {
        m_enoughSpace = false;
    }
    m_fillDisplay->repaint();
    kdDebug() << m_fillDisplay->freeWithDvd()<< endl;
    QString overview = m_fillDisplay->freeWithDvd() + QString().sprintf( " / %.2f GBytes", ((float)kBSize)/1000000);
    m_hardDiskSpace->setText( overview );
}

bool K3bDvdRipperWidget::createDirs(){
  bool result = true;
  QString dir = m_editStaticRipPath->url();
  result = result & createDirectory( m_editStaticRipPath->url() );
  result = result & createDirectory( m_editStaticRipPath->url()+"/vob" );
  result = result & createDirectory( m_editStaticRipPath->url()+"/tmp" );
  return result;
}

bool K3bDvdRipperWidget::createDirectory( const QString& dir ){
  QDir destDir( dir );
  if( !destDir.exists() ){
    if( !destDir.mkdir( dir ) ){
      KMessageBox::error( this, i18n("Unable to create directory %1").arg(dir), i18n("Ripping Error") );
      return false;
    }
  }
  return true;
}

void K3bDvdRipperWidget::checkSize(  ){
  m_vobSize = 0;
  typedef QValueList<K3bDvdContent> DvdTitle;
  DvdTitle::Iterator dvd;
  int max = m_ripTitles.count();
  //kdDebug() << " ripTitles checksite " << max << endl;
  const K3bExternalBin *bin = k3bcore->externalBinManager()->binObject("tccat");
  for( int i = 0; i < max; i++ ){
    dvd = m_ripTitles.at( i );
    if( (*dvd).isAllAngle() ){
      m_detectTitleSizeDone = false;
      m_supportSizeDetection = true;
      int title = (*dvd).getTitleNumber();
      QString t( QString::number(title) );
      KShellProcess p;
      p << bin->path << "-d 1" << "-i" <<  m_device << "-P" << t;
      connect( &p, SIGNAL(receivedStderr(KProcess*, char*, int)), this, SLOT(slotParseError(KProcess*, char*, int)) );
      //connect( &p, SIGNAL(processExited(KProcess*)), this, SLOT(slotExited( KProcess* )) );
      if( !p.start( KProcess::Block, KProcess::Stderr ) ) {
	kdDebug() << "(K3bDvdRipperWidget) Can't detect size of title" << endl;
      }
      //kdDebug() << "VobSize: " << (float) m_vobSize << ", titlesize " << (float) m_titleSize << endl;

      m_vobSize += m_titleSize;
    } else {
      m_supportSizeDetection = false;
    }
  }
  slotSetDependDirs( m_editStaticRipPath->url() );
}

void K3bDvdRipperWidget::slotParseError( KProcess *p, char *text, int len ){
  QString tmp = QString::fromLocal8Bit( text, len );
  kdDebug() << "(K3bDvdRipperWidget) Parse output for size: " << tmp << endl;
  // must be the first line, ignore other. NO, not for encrypted DVDs
  if( !m_detectTitleSizeDone ){
    if( tmp.contains("blocks") ){
      m_detectTitleSizeDone = true;
      m_titleSize = (double) K3bDvdRippingProcess::tccatParsedBytes( text, len );
      kdDebug() << "(K3bDvdRipperWidget) Titlesize to rip: " << m_titleSize << endl;
      p->kill();
    }
  }
}

void K3bDvdRipperWidget::slotCheckOpenEncoding( int state ){
  if (state == 0 )
    m_openEncoding = false;
  else {
    m_openEncoding = true;
    m_startEncoding = false;
    m_checkStartEncoding->setChecked( false );
  }
}

void K3bDvdRipperWidget::slotCheckStartEncoding( int state ){
  if (state == 0 )
    m_startEncoding = false;
  else {
    m_startEncoding = true;
    m_openEncoding = false;
    m_checkOpenEncoding->setChecked( false );
  }
}

void K3bDvdRipperWidget::slotOpenEncoding( bool result ){
    kdDebug() << "(K3bDvdRipperWidget) Finished ripping status: " << result
        << ", Initstatus: " << m_ripJob->isStartFailed() << endl;
    if( !result ){
        m_ripDialog->close();
        delete m_ripProcess;
        m_ripProcess = 0;
    }
    
    if( result && m_openEncoding ){
        m_ripDialog->close();
        delete m_ripProcess;
        m_ripProcess = 0;
        // fill data
        openEncodingDialog();
        // calculate final size depend on quality
        // start encoding
    }
}

void K3bDvdRipperWidget::slotStartEncoding( bool ){

}

void K3bDvdRipperWidget::prepareDataForEncoding(){

}
void K3bDvdRipperWidget::openEncodingDialog(){
  kdDebug() << "(K3bDvdRipperWidget:preparedDataForEncoding)" << endl;

  K3bDivxCodecData* data = new K3bDivxCodecData;
  QString projectFile(m_editStaticRipPath->url() + "/k3bDVDRip.xml");
  QFile f( projectFile );
  if( f.exists() ) {
    data->setProjectFile( projectFile );
    if( !data->projectLoaded() ){
      KMessageBox::error( this, i18n("Error while parsing file: %1").arg(projectFile),
			  i18n("Error Loading Project") );
      close();
      return;
    }
  }

  data->setAviFile( m_editStaticRipPath->url() + "/video-k3b.avi");

  // the divx view takes care of deleting the K3bDivxCodecData
  K3bDivxView d(data, this, "divx");
  d.slotUpdateView();
  d.exec();

  close();

  // TODO set defaults from config
  //m_data->setAudioBitrate( 128 );
  //m_data->setAudioResample( 2 );
  //m_data->setCodec( "xvid" );
  //m_data->setCodecMode( 2 );
  //m_data->setCrispness( 100 );
  //m_data->setDeinterlace( 0 );
  //m_data->setYuv( 2 );
  //QTime t = m_data->getTime();
  //int m_lengthSecs = t.hour()*3600 + t.minute()*60 + t.second();
  //long vBitrate = ( 700*1024 / m_lengthSecs * 8 );// one correct 1.024 K->1024
  //vBitrate = vBitrate - 128;
  //m_data->setVideoBitrate( vBitrate );
  //m_data->setAudioBitrate( 128 );

  /*
    m_divxJob = new K3bDivXEncodingProcess( m_data, this );

    m_divxDialog = new K3bBurnProgressDialog( this, "Encoding", true );
    m_divxDialog->setCaption( i18n("Encoding Process") );
    m_divxDialog->setJob( m_divxJob );

    m_divxJob->start();
    m_divxDialog->exec();
    connect( m_divxJob, SIGNAL( finished( bool )), this, SLOT( slotEncodingFinished( bool )) );
  */
}

void K3bDvdRipperWidget::slotEncodingFinished( bool ){
  //    delete m_divxJob;
}


void K3bDvdRipperWidget::slotLoadK3bDefaults()
{
  m_editStaticRipPath->setURL( QDir::homeDirPath() );
  m_checkOpenEncoding->setChecked(false);
  m_checkStartEncoding->setChecked(false);
}


void K3bDvdRipperWidget::slotLoadUserDefaults()
{
  KConfig* c = kapp->config();
  c->setGroup( "DVD ripping" );

  m_editStaticRipPath->setURL( c->readEntry( "last ripping directory", QDir::homeDirPath() ) );
  m_checkOpenEncoding->setChecked( c->readBoolEntry( "open_encoding", false ) );
  m_checkStartEncoding->setChecked( c->readBoolEntry( "start_encoding", false ) );
}


void K3bDvdRipperWidget::slotSaveUserDefaults()
{
  KConfig* c = kapp->config();
  c->setGroup( "DVD ripping" );

  c->writeEntry( "last ripping directory", m_editStaticRipPath->url() );
  c->writeEntry( "open_encoding", m_checkOpenEncoding->isChecked() );
  c->writeEntry( "start_encoding", m_checkStartEncoding->isChecked() );
}

#include "k3bdvdripperwidget.moc"
