/***************************************************************************
                          k3bdvdcrop.cpp  -  description
                             -------------------
    begin                : Tue Apr 2 2002
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

#include "k3bdvdcrop.h"
#include "k3bdvdpreview.h"
#include "k3bdvdcodecdata.h"

#include <qlabel.h>
#include <qslider.h>
#include <qcheckbox.h>
#include <qvbuttongroup.h>
#include <qradiobutton.h>
#include <qlayout.h>
#include <qframe.h>
#include <qimage.h>
#include <qcanvas.h>

#include <kdialog.h>
#include <klocale.h>
#include <klineedit.h>
#include <kcombobox.h>
#include <knuminput.h>
#include <kprocess.h>

K3bDvdCrop::K3bDvdCrop(K3bDvdCodecData *data, QWidget *parent, const char *name ) : QGroupBox(parent,name) {
     m_data = data;
     setupGui();
}

K3bDvdCrop::~K3bDvdCrop(){
}

void K3bDvdCrop::setupGui(){
    setColumnLayout(0, Qt::Vertical );
    setTitle( i18n( "Cropping Settings" ) );
    QGridLayout *mainLayout = new QGridLayout( layout() );
    mainLayout->setSpacing( KDialog::spacingHint() );
    mainLayout->setMargin( KDialog::marginHint() );

    QCanvas *c = new QCanvas(380, 300);
    //c->setAdvancePeriod(30);
    m_preview = new K3bDvdPreview( c, this );

    m_sliderPreview = new QSlider( Horizontal,  this );

    QVButtonGroup *modeGroup = new QVButtonGroup( i18n("Resize mode"), this );
    m_buttonFast = new QRadioButton( i18n("Fast (-B)"), modeGroup );
    m_buttonExactly = new QRadioButton( i18n("Exact (-Z)"), modeGroup );

    QGroupBox *groupCrop = new QGroupBox( this );
    groupCrop->setColumnLayout(0, Qt::Vertical );
    QGridLayout *cropLayout = new QGridLayout( groupCrop->layout() );
    cropLayout->setSpacing( KDialog::spacingHint() );
    cropLayout->setMargin( KDialog::marginHint() );
    groupCrop->setTitle( i18n( "Crop parameters" ) );
    m_spinBottom = new KIntSpinBox( groupCrop );
    m_spinTop = new KIntSpinBox( groupCrop );
    m_spinLeft = new KIntSpinBox( groupCrop );
    m_spinRight = new KIntSpinBox( groupCrop );

    QFrame* line = new QFrame( groupCrop, "line" );
    line->setFrameStyle( QFrame::HLine | QFrame::Sunken );
    m_autoCrop = new QCheckBox( i18n("Automatically crop"), groupCrop );

    cropLayout->addMultiCellWidget( m_autoCrop, 0, 0, 0, 5);
    cropLayout->addMultiCellWidget( line, 1, 1, 0, 5);
    cropLayout->addMultiCellWidget( m_spinTop, 3, 3, 2, 2);
    cropLayout->addMultiCellWidget( m_spinLeft, 4, 4, 1, 1);
    cropLayout->addMultiCellWidget( m_spinBottom, 5, 5, 2, 2);
    cropLayout->addMultiCellWidget( m_spinRight, 4, 4, 3, 3);

    QGroupBox *groupFinal = new QGroupBox( this );
    groupFinal->setColumnLayout(0, Qt::Vertical );
    QGridLayout *finalLayout = new QGridLayout( groupFinal->layout() );
    finalLayout->setSpacing( KDialog::spacingHint() );
    finalLayout->setMargin( KDialog::marginHint() );
    groupFinal->setTitle( i18n( "Final size" ) );
    QLabel *size = new QLabel( i18n("Size:"), groupFinal );
    QLabel *aspect = new QLabel( i18n("Aspect ratio:"), groupFinal );
    m_finalSize = new QLabel( "", groupFinal );
    m_finalAspect = new QLabel( "", groupFinal );
    finalLayout->addWidget( size, 0, 0 );
    finalLayout->addWidget( aspect, 1, 0);
    finalLayout->addWidget( m_finalSize, 0, 1);
    finalLayout->addWidget( m_finalAspect, 1, 1);

    mainLayout->addMultiCellWidget( m_preview, 0, 2, 0, 1);
    mainLayout->addMultiCellWidget( m_sliderPreview, 3, 3, 0, 1);
    mainLayout->addMultiCellWidget( modeGroup, 0, 0, 2, 2);
    mainLayout->addMultiCellWidget( groupCrop, 1, 1, 2, 2);
    mainLayout->addMultiCellWidget( groupFinal, 2, 2, 2, 2);

    // cropping
    connect( m_spinTop, SIGNAL( valueChanged( int ) ), this, SLOT( slotSpinTop( int )) );
    connect( m_spinBottom, SIGNAL( valueChanged( int ) ), this, SLOT( slotSpinBottom( int )) );
    connect( m_spinLeft, SIGNAL( valueChanged( int ) ), this, SLOT( slotSpinLeft( int )) );
    connect( m_spinRight, SIGNAL( valueChanged( int ) ), this, SLOT( slotSpinRight( int )) );
    // resize mode
    connect( modeGroup, SIGNAL( clicked( int )), this, SLOT( slotResizeMode( int )) );

    // some init stuff
    modeGroup->setButton( 0  );
    setSpinBoxMode( 8 );
}

void K3bDvdCrop::initPreview( ){
     previewProcess = new KShellProcess(); // = new KShellProcess;
     *previewProcess << "/usr/local/bin/transcode -i ";
     *previewProcess << m_data->getProjectDir() + "/vob";
     qDebug("Projectdir: " + m_data->getProjectDir()+"/vob");
     *previewProcess << " -x vob -V -y ppm -w 1200 -a 0 -L 300000 -c 4-5";
     *previewProcess << "-o " + m_data->getProjectDir() + "/preview";
     connect( previewProcess, SIGNAL(receivedStdout(KProcess*, char*, int)),
	       this, SLOT(slotParseProcess(KProcess*, char*, int)) );
     connect( previewProcess, SIGNAL(receivedStderr(KProcess*, char*, int)),
	       this, SLOT(slotParseProcess(KProcess*, char*, int)) );
     if( !previewProcess->start( KProcess::Block, KProcess::AllOutput ) ){
         qDebug("Error process starting");
     }
     delete previewProcess;
     //QCanvasPixmap *pix = new QCanvasPixmap( data->getProjectDir() + "/preview00000.ppm");
     m_preview->setPreviewPicture( m_data->getProjectDir() + "/preview00000.ppm" );
}

void K3bDvdCrop::setSpinBoxMode( int step ){
    m_spinTop->setLineStep( step );
    m_spinLeft->setLineStep( step );
    m_spinBottom->setLineStep( step );
    m_spinRight->setLineStep( step );
}
void K3bDvdCrop::updateSpinBox( int step ){
    setCorrectedSpinValue( m_spinTop, step );
    setCorrectedSpinValue( m_spinLeft, step );
    setCorrectedSpinValue( m_spinBottom, step );
    setCorrectedSpinValue( m_spinRight, step );
}
void K3bDvdCrop::setCorrectedSpinValue( KIntSpinBox *box, int step ){
    int value = box->value();
    int error =  value % step;
    box->setValue( value + (( 8-error )% 8 ) );
}
void K3bDvdCrop::slotUpdateFinalSize(){
    int h = m_data->getHeightValue() - 8* m_data->getResizeHeight();
    h = h - m_data->getCropTop() - m_data->getCropBottom();
    int w = m_data->getWidthValue() - 8* m_data->getResizeWidth();
    w = w - m_data->getCropLeft() - m_data->getCropRight();
    float a = (float) w / (float) h;
    m_finalSize->setText( QString::number(w) + " x " + QString::number(h) );
    m_finalAspect->setText( "1:" + QString::number( a, 'f', 2 ) );
}

void K3bDvdCrop::slotParseProcess( KProcess* p, char *data, int len){
    QString tmp = QString::fromLatin1( data, len );
    qDebug( tmp );
}

void K3bDvdCrop::slotSpinTop( int v){
    m_preview->setTopLine( v );
    m_data->setCropTop( v );
    slotUpdateFinalSize();
}
void K3bDvdCrop::slotSpinLeft( int v){
    m_preview->setLeftLine( v );
    m_data->setCropLeft( v );
    slotUpdateFinalSize();
}
void K3bDvdCrop::slotSpinBottom( int v){
    m_preview->setBottomLine( v );
    m_data->setCropBottom( v );
    slotUpdateFinalSize();
}
void K3bDvdCrop::slotSpinRight( int v){
    m_preview->setRightLine( v );
    m_data->setCropRight( v );
    slotUpdateFinalSize();
}

void K3bDvdCrop::slotResizeMode( int id ){
     switch( id ){
         case 0:{
             setSpinBoxMode( 8 );
             updateSpinBox( 8 );
             break;
         }
         case 1:{
             setSpinBoxMode( 1 );
             break;
         }
         default:
         break;
     }
}


#include "k3bdvdcrop.moc"
