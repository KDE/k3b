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

#include <qlayout.h>
#include <qvgroupbox.h>
#include <qhgroupbox.h>
#include <qfont.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qdatastream.h>

#include <kaction.h>
#include <kconfig.h>
#include <klocale.h>
#include <kapp.h>
#include <kdialog.h>
#include <kfiledialog.h>
#include <klineedit.h>
#include <kprocess.h>

K3bDvdRipperWidget::K3bDvdRipperWidget(const QString& device, QWidget *parent, const char *name )
    : KDialogBase( parent, name, true, i18n("Ripping DVD"), KDialogBase::Close|KDialogBase::Apply ) {
    m_device = device;
    m_bytes = 0;
    m_finalClose = false;
    m_ripProcess = 0;
    m_ripJob = 0;
    KConfig* c = kapp->config();
    c->setGroup("DVDRipping");
    setupGui();
}

K3bDvdRipperWidget::~K3bDvdRipperWidget(){
    if( m_ripProcess != 0 )
        delete m_ripProcess;
    if( m_ripJob != 0 )
        delete m_ripJob;
}

void K3bDvdRipperWidget::setupGui(){
    QFrame *frame = makeMainWidget();
    QGridLayout *Form1Layout = new QGridLayout( frame );
    Form1Layout->setSpacing( KDialog::spacingHint() );
    Form1Layout->setMargin( KDialog::marginHint() );

    QGroupBox *groupPattern = new QGroupBox( i18n( "Destination Directory" ), frame, "pattern" );
    groupPattern->setColumnLayout(0, Qt::Vertical );
    //groupPattern->setTitle( i18n( "Destination Directory" ) );
    QGridLayout *dirsLayout = new QGridLayout( groupPattern->layout() );
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
    //QHGroupBox *groupNormalize = new QHGroupBox( i18n("PreProcessing"), frame, "Normalize" );
    //m_normalize = new QCheckBox(i18n("Check audio level for normalize."), groupNormalize, "check_normal");

    Form1Layout->addMultiCellWidget( groupPattern, 0, 0, 0, 3 );
    //Form1Layout->addMultiCellWidget( groupNormalize, 1, 1, 0, 3 );
    //Form1Layout->addMultiCellWidget( m_normalize, 2, 2, 0, 3 );
    Form1Layout->setColStretch( 0, 20 );

    setButtonApplyText( i18n( "Start Ripping" ), i18n( "This starts copying the DVD.") );

    connect( this, SIGNAL( closeClicked() ), this, SLOT( close() ) );
    connect( this, SIGNAL( applyClicked() ), this, SLOT(rip() ) );
    connect(m_buttonStaticDir, SIGNAL(clicked()), this, SLOT(slotFindStaticDir()) );
    connect(m_buttonStaticDirVob, SIGNAL(clicked()), this, SLOT(slotFindStaticDirVob()) );
    connect(m_buttonStaticDirTmp, SIGNAL(clicked()), this, SLOT(slotFindStaticDirTmp()) );
}

void K3bDvdRipperWidget::init( const QValueList<K3bDvdContent>& list ){
    m_ripTitles = list;
}

void K3bDvdRipperWidget::rip(){
    m_ripJob = new K3bDvdCopy(m_device, m_editStaticRipPath->text(), m_editStaticRipPathVob->text(), m_editStaticRipPathTmp->text(), m_ripTitles, this );
    /*
    m_ripJob->setDvdTitle( m_ripTitles );
    m_ripJob->setDevice( m_device );
    m_ripJob->setBaseFilename( m_editStaticRipPath->text() + "/dvd" );
    */
    //connect( m_ripJob, SIGNAL( interrupted() ), this, SLOT( slotRipJobDeleted() ) );
    /*
    m_ripProcess = new K3bDvdRippingProcess( this );
    m_ripProcess->setDvdTitle( m_ripTitles );
    K3bDevice *dev = k3bMain()->deviceManager()->deviceByName( m_device );
    m_ripProcess->setDevice( dev->ioctlDevice() );
    m_ripProcess->setBaseFilename( m_editStaticRipPath->text()  );
    m_ripProcess->setJob( m_ripJob );
    connect( m_ripProcess, SIGNAL( interrupted() ), this, SLOT( slotRipJobDeleted() ) );
    */
    m_ripDialog = new K3bBurnProgressDialog( this, "Ripping" );
    m_ripDialog->setCaption( i18n("Ripping process") );
    m_ripDialog->setJob( m_ripJob );
    m_ripDialog->show();

    m_ripJob->start();
    //m_ripProcess->start( );
    //this.hide();
}

void K3bDvdRipperWidget::slotRipJobDeleted(){
    qDebug("(K3bDvdRipperWidget) Rip job finished/interrupted.");
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

#include "k3bdvdripperwidget.moc"