/***************************************************************************
                          k3bdvdripperwidget.cpp  -  description
                             -------------------
    begin                : Sun Mar 3 2002
    copyright            : (C) 2002 by Sebastian Trueg
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

#include "k3bdvdripperwidget.h"
#include "k3bdvdcontent.h"
#include "k3bdvdcopy.h"
#include "../k3bburnprogressdialog.h"
#include "k3bdvdrippingprocess.h"
#include "../k3b.h"
#include "../device/k3bdevicemanager.h"
#include "../device/k3bdevice.h"
#include "../kdiskfreesp.h"
#include "../tools/k3bexternalbinmanager.h"
#include "k3bdvdfilldisplay.h"
#include "k3bdvdextraripstatus.h"

#include <qlayout.h>
#include <qvgroupbox.h>
#include <qhgroupbox.h>
#include <qfont.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qdatastream.h>
#include <qmessagebox.h>

#include <qpainter.h>
#include <qrect.h>

#include <kaction.h>
#include <kconfig.h>
#include <klocale.h>
#include <kapplication.h>
#include <kdialog.h>
#include <kfiledialog.h>
#include <klineedit.h>
#include <kprocess.h>

K3bDvdRipperWidget::K3bDvdRipperWidget(const QString& device, QWidget *parent, const char *name )
    : KDialogBase( parent, name, true, i18n("Ripping DVD"), KDialogBase::Close|KDialogBase::Apply ) {
    m_device = k3bMain()->deviceManager()->deviceByName( device )->ioctlDevice();
    m_bytes = 0;
    m_finalClose = false;
    m_ripProcess = 0;
    m_ripJob = 0;
    KConfig* c = kapp->config();
    c->setGroup("DVDRipping");
}

K3bDvdRipperWidget::~K3bDvdRipperWidget(){
    if( m_ripProcess != 0 )
        delete m_ripProcess;
    if( m_ripJob != 0 )
        delete m_ripJob;
}

void K3bDvdRipperWidget::setupGui(){
    setMinimumWidth( 500 );
    QFrame *frame = makeMainWidget();
    QGridLayout *mainLayout = new QGridLayout( frame );

    QGroupBox *groupPattern = new QGroupBox( i18n( "Destination Directory" ), frame, "pattern" );
    groupPattern->setColumnLayout(0, Qt::Vertical );
    QGridLayout *dirsLayout = new QGridLayout( groupPattern->layout() );
    dirsLayout->setSpacing( KDialog::spacingHint() );
    dirsLayout->setMargin( KDialog::marginHint() );
    m_editStaticRipPath = new KLineEdit(groupPattern, "staticeditpattern");
    m_editStaticRipPath->setText( QDir::homeDirPath() );
    m_buttonStaticDir = new QPushButton( groupPattern, "m_buttonStaticDir" );
    m_buttonStaticDir->setText( tr( "..." ) );

    m_editStaticRipPathVob = new KLineEdit(groupPattern, "staticeditpatternvob");
    m_editStaticRipPathVob->setText( QDir::homeDirPath() + "/vob" );
    m_buttonStaticDirVob = new QPushButton( groupPattern, "m_buttonStaticDir" );
    m_buttonStaticDirVob->setText( tr( "..." ) );

    m_editStaticRipPathTmp = new KLineEdit(groupPattern, "staticeditpatterntmp");
    m_editStaticRipPathTmp->setText( QDir::homeDirPath() + "/tmp" );
    m_buttonStaticDirTmp = new QPushButton( groupPattern, "m_buttonStaticDir" );
    m_buttonStaticDirTmp->setText( tr( "..." ) );

    dirsLayout->addMultiCellWidget( m_editStaticRipPath, 0,0,0,2 );
    dirsLayout->addMultiCellWidget( m_editStaticRipPathVob, 1,1,0,2 );
    dirsLayout->addMultiCellWidget( m_editStaticRipPathTmp, 2,2,0,2 );
    dirsLayout->addMultiCellWidget( m_buttonStaticDir, 0,0,3,3 );
    dirsLayout->addMultiCellWidget( m_buttonStaticDirVob, 1,1,3,3 );
    dirsLayout->addMultiCellWidget( m_buttonStaticDirTmp, 2,2,3,3 );
    dirsLayout->setColStretch( 0, 20 );

    QGroupBox *groupSize = new QGroupBox( i18n( "Available Space" ), frame, "size" );
    groupSize->setColumnLayout(0, Qt::Vertical );
    QGridLayout *sizeLayout = new QGridLayout( groupSize->layout() );

    m_fillDisplay = new K3bDvdFillDisplay( groupSize );
    m_fillDisplay->setFixedHeight( 20 );
    m_hardDiskSpace = new QLabel( groupSize );

    sizeLayout->addMultiCellWidget( m_fillDisplay, 0, 0, 0, 1 );
    sizeLayout->addMultiCellWidget( m_hardDiskSpace, 0, 0, 2, 2 );
    sizeLayout->setColStretch( 0, 20 );

    mainLayout->addMultiCellWidget( groupPattern, 0, 0, 0, 1 );
    mainLayout->addMultiCellWidget( groupSize, 1, 1, 0, 1 );

    setButtonApplyText( i18n( "Start Ripping" ), i18n( "This starts the DVD copy.") );


    connect( this, SIGNAL( closeClicked() ), this, SLOT( close() ) );
    connect( this, SIGNAL( applyClicked() ), this, SLOT(rip() ) );
    connect(m_buttonStaticDir, SIGNAL(clicked()), this, SLOT(slotFindStaticDir()) );
    connect(m_buttonStaticDirVob, SIGNAL(clicked()), this, SLOT(slotFindStaticDirVob()) );
    connect(m_buttonStaticDirTmp, SIGNAL(clicked()), this, SLOT(slotFindStaticDirTmp()) );
    connect(m_editStaticRipPath, SIGNAL( textChanged( const QString& )), this, SLOT( slotSetDependDirs( const QString& )) );


}

void K3bDvdRipperWidget::init( const QValueList<K3bDvdContent>& list ){
    m_ripTitles.clear();
    m_ripTitles = list;
    checkSize();
}

void K3bDvdRipperWidget::rip(){
    if( !m_enoughSpace )
         return;
    if( !createDirs() ){
        return;
    }
    m_ripJob = new K3bDvdCopy(m_device, m_editStaticRipPath->text(), m_editStaticRipPathVob->text(), m_editStaticRipPathTmp->text(), m_ripTitles, this );
    m_ripJob->setRipSize( m_vobSize );

    m_ripDialog = new K3bBurnProgressDialog( this, "Ripping", false );
    m_ripDialog->setCaption( i18n("Ripping process") );
    m_ripDialog->setJob( m_ripJob );
    K3bDvdExtraRipStatus *ripStatus = new K3bDvdExtraRipStatus( m_ripDialog );
    connect( m_ripJob, SIGNAL( dataRate( float )), ripStatus, SLOT( slotDataRate( float )) );
    connect( m_ripJob, SIGNAL( estimatedTime( unsigned int )), ripStatus, SLOT( slotEstimatedTime( unsigned int )) );
    m_ripDialog->setExtraInfo( ripStatus );

    m_ripJob->start();
    m_ripDialog->exec();
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
    QString path = KFileDialog::getExistingDirectory( m_editStaticRipPath->text(), this, i18n("Select Ripping Directory") );
    if( !path.isEmpty() ) {
        m_editStaticRipPath->setText( path );
        slotSetDependDirs( path );
    }
}

void K3bDvdRipperWidget::slotFindStaticDirVob() {
    QString path = KFileDialog::getExistingDirectory( m_editStaticRipPath->text(), this, i18n("Select Ripping Directory") );
    if( !path.isEmpty() ) {
        m_editStaticRipPathVob->setText( path );
    }
}

void K3bDvdRipperWidget::slotFindStaticDirTmp() {
    QString path = KFileDialog::getExistingDirectory( m_editStaticRipPath->text(), this, i18n("Select Ripping Directory") );
    if( !path.isEmpty() ) {
        m_editStaticRipPathTmp->setText( path );
    }
}

void K3bDvdRipperWidget::slotSetDependDirs( const QString& p ) {
    QString path = p;
    if( path.endsWith("/") )
         path = path.left( path.length()-1 );
    QString vob = m_editStaticRipPathVob->text();
    int index = vob.findRev("/");
    vob = vob.mid( index+1 );
    m_editStaticRipPathVob->setText( path + "/" + vob );
    QString tmp = m_editStaticRipPathTmp->text();
    index = tmp.findRev("/");
    tmp = tmp.mid( index+1 );
    m_editStaticRipPathTmp->setText( path + "/" + tmp );

    QDir space( p );
    if( space.exists() ){
        connect( KDiskFreeSp::findUsageInfo( p ),
	     SIGNAL(foundMountPoint(const QString&, unsigned long, unsigned long, unsigned long)),
	     this, SLOT(slotFreeTempSpace(const QString&, unsigned long, unsigned long, unsigned long)) );
    }
}

void K3bDvdRipperWidget::slotFreeTempSpace( const QString & mountPoint, unsigned long kBSize,
        unsigned long kBUsed, unsigned long kBAvail ){
    m_fillDisplay->setKbSize( kBSize );
    m_fillDisplay->setKbAvailable( kBAvail );
    m_fillDisplay->setKbUsed( kBUsed );
    kdDebug() << "(K3bDvdRipperWidget) DVD VobSize: " << m_vobSize << endl;
    m_fillDisplay->setKbDvd( (unsigned long) (m_vobSize/1000) );
     if( kBAvail > m_vobSize/1000 ) {
        m_enoughSpace = true;
    } else {
        m_enoughSpace = false;
    }
    m_fillDisplay->repaint();
    QString overview = m_fillDisplay->freeWithDvd() + QString().sprintf( " / %.2f GBytes", ((float)kBSize)/1000000);
    m_hardDiskSpace->setText( overview );
}
bool K3bDvdRipperWidget::createDirs(){
    bool result = true;
    QString dir = m_editStaticRipPath->text();
    result = result & createDirectory( m_editStaticRipPath->text() );
    result = result & createDirectory( m_editStaticRipPathVob->text() );
    result = result & createDirectory( m_editStaticRipPathTmp->text() );
    return result;
}

bool K3bDvdRipperWidget::createDirectory( const QString& dir ){
    QDir destDir( dir );
    if( !destDir.exists() ){
        if( !destDir.mkdir( dir ) ){
            QMessageBox::critical( 0, i18n("Ripping Error"), i18n("Unable to create directory %1").arg(dir), i18n("OK") );
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
    K3bExternalBin *bin = k3bMain()->externalBinManager()->binObject("tccat");
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
            connect( &p, SIGNAL(processExited(KProcess*)), this, SLOT(slotExited( KProcess* )) );
            if( !p.start( KProcess::Block, KProcess::Stderr ) ) {
                kdDebug() << "(K3bDvdRipperWidget) Can't detect size of title" << endl;
            }
            //kdDebug() << "VobSize: " << (float) m_vobSize << ", titlesize " << (float) m_titleSize << endl;

            m_vobSize += m_titleSize;
        } else {
            m_supportSizeDetection = false;
        }
    }
    setupGui();
    slotSetDependDirs( m_editStaticRipPath->text() );
}

void K3bDvdRipperWidget::slotParseError( KProcess *p, char *text, int len ){
    QString tmp = QString::fromLatin1( text, len );
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

#include "k3bdvdripperwidget.moc"
