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

#include "k3bdivxcrop.h"
#include "k3bdivxpreview.h"
#include "k3bdivxcodecdata.h"
#include "../k3b.h"
#include "../tools/k3bexternalbinmanager.h"

#include <qlabel.h>
#include <qslider.h>
#include <qcheckbox.h>
#include <qvbuttongroup.h>
#include <qradiobutton.h>
#include <qlayout.h>
#include <qframe.h>
#include <qimage.h>
#include <qcanvas.h>
#include <qwhatsthis.h>

#include <kdialog.h>
#include <klocale.h>
#include <klineedit.h>
#include <kcombobox.h>
#include <knuminput.h>
#include <kprocess.h>
#include <kdirsize.h>
#include <kdebug.h>

K3bDivxCrop::K3bDivxCrop(K3bDivxCodecData *data, QWidget *parent, const char *name ) : QGroupBox(parent,name) {
     m_data = data;
     m_previewOffset = 0;
     setupGui();
}

K3bDivxCrop::~K3bDivxCrop(){
}

void K3bDivxCrop::setupGui(){
    setColumnLayout(0, Qt::Vertical );
    setTitle( i18n( "Cropping Settings" ) );
    QGridLayout *mainLayout = new QGridLayout( layout() );
    mainLayout->setSpacing( KDialog::spacingHint() );
    mainLayout->setMargin( 0 );// KDialog::marginHint() );

    QCanvas *c = new QCanvas(380, 300);  //image 360x288
    //c->setAdvancePeriod(30);
    m_preview = new K3bDivxPreview( c, this );
    QWhatsThis::add( m_preview, i18n("Preview picture off the video to crop it properly. The colors and the quality are NOT the final state, so use this \
picture only to cut of black borders"));
    m_sliderPreview = new QSlider( Horizontal,  this );
    QWhatsThis::add( m_sliderPreview, i18n("Move through to video to find a light picture to do a proper cut of the black bars"));
    QVButtonGroup *modeGroup = new QVButtonGroup( i18n("Resize mode"), this );
    modeGroup->layout()->setMargin( KDialog::marginHint() );
    m_buttonFast = new QRadioButton( i18n("Fast (-B)"), modeGroup );
    m_buttonExactly = new QRadioButton( i18n("Exact (-Z)"), modeGroup );
    QWhatsThis::add( modeGroup, i18n("\"Fast\" resizing is much faster than the \"Exact\" resizing but is limit to a resize step multiple of 8 pixels."));

    QGroupBox *groupCrop = new QGroupBox( this );
    groupCrop->setColumnLayout(0, Qt::Vertical );
    groupCrop->layout()->setMargin( 0 );//KDialog::marginHint() );
    QGridLayout *cropLayout = new QGridLayout( groupCrop->layout() );
    cropLayout->setSpacing( KDialog::spacingHint() );
    cropLayout->setMargin( KDialog::marginHint() );
    groupCrop->setTitle( i18n( "Crop parameters" ) );
    m_spinBottom = new KIntSpinBox( 0, 100, 8, 0, 10, groupCrop );
    m_spinTop = new KIntSpinBox( 0, 100, 8, 0, 10, groupCrop );
    m_spinLeft = new KIntSpinBox( 0, 100, 8, 0, 10, groupCrop );
    m_spinRight = new KIntSpinBox( 0, 100, 8, 0, 10, groupCrop );
    QWhatsThis::add( groupCrop, i18n("Cut of the black borders of the movie"));
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
    groupFinal->layout()->setMargin( 0 );
    QGridLayout *finalLayout = new QGridLayout( groupFinal->layout() );
    finalLayout->setSpacing( KDialog::spacingHint() );
    finalLayout->setMargin( KDialog::marginHint() );
    groupFinal->setTitle( i18n( "Final video" ) );
    QLabel *size = new QLabel( i18n("Size:"), groupFinal );
    QLabel *aspect = new QLabel( i18n("Aspect ratio:"), groupFinal );
    QLabel *quality = new QLabel( i18n("Video quality:"), groupFinal );
    m_finalSize = new QLabel( "", groupFinal );
    m_finalAspect = new QLabel( "", groupFinal );
    m_finalQuality = new QLabel( "", groupFinal );
    finalLayout->addWidget( size, 0, 0 );
    finalLayout->addWidget( aspect, 1, 0);
    finalLayout->addWidget( quality, 2, 0 );
    finalLayout->addWidget( m_finalSize, 0, 1);
    finalLayout->addWidget( m_finalAspect, 1, 1);
    finalLayout->addWidget( m_finalQuality, 2, 1 );
    QWhatsThis::add( groupFinal, i18n("This is the final size of the movie. Verify that the final size of your movie have the right aspect ratio \
1:1.333 for a 4:3 movie, 1:1.77 for a 16:9 movie, and 1:2.35 for a letterbox movie. If you are using cropping you can ignore the aspect ratio error shown \
in the resizing box. The \"Video quality\" is an value to ESTIMATE the final video quality. (The value is calculate as \"<video bitrate>/(<framerate>*<height>*<width>)\", \
you may know this from GordianKnot (www.doom9.net)). The higher the value the better the quality.\nThe following values are ok if you look the movies on TV. \
Values > 0.15 are ok for letterbox movie, for 16:9 you should use better values > 0.16 and for 4:3 > 0.17-0.18, but that depends on you, try it."));

    QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );

    mainLayout->addMultiCellWidget( m_preview, 0, 3, 0, 1);
    mainLayout->addMultiCellWidget( m_sliderPreview, 4, 4, 0, 1);
    mainLayout->addMultiCellWidget( modeGroup, 0, 0, 2, 2);
    mainLayout->addMultiCellWidget( groupCrop, 1, 1, 2, 2);
    mainLayout->addMultiCellWidget( groupFinal, 2, 2, 2, 2);
    mainLayout->addItem( spacer, 3, 2);
    mainLayout->setColStretch( 0, 20 );

    // cropping
    connect( m_spinTop, SIGNAL( valueChanged( int ) ), this, SLOT( slotSpinTop( int )) );
    connect( m_spinBottom, SIGNAL( valueChanged( int ) ), this, SLOT( slotSpinBottom( int )) );
    connect( m_spinLeft, SIGNAL( valueChanged( int ) ), this, SLOT( slotSpinLeft( int )) );
    connect( m_spinRight, SIGNAL( valueChanged( int ) ), this, SLOT( slotSpinRight( int )) );
    // resize mode
    connect( modeGroup, SIGNAL( clicked( int )), this, SLOT( slotResizeMode( int )) );
    connect( m_sliderPreview, SIGNAL( valueChanged( int )), this, SLOT( slotPreviewChanged( int ) ));
    // autocrop
    connect( m_autoCrop, SIGNAL( stateChanged( int )), this, SLOT( slotAutoCropMode( int ) ));
    // some init stuff
    modeGroup->setButton( 0  );
}

void K3bDivxCrop::updateView(){
    m_spinBottom->setMaxValue( m_data->getHeightValue()/2 - 50 );
    m_spinTop->setMaxValue( m_data->getHeightValue()/2 - 50 );
    m_spinLeft->setMaxValue( m_data->getWidthValue()/2 - 100 );
    m_spinRight->setMaxValue( m_data->getWidthValue()/2 - 100 );
}

void K3bDivxCrop::resetView(){
    m_preview->resetView();
    m_spinTop->setValue( 0 );
    m_spinLeft->setValue( 0 );
    m_spinBottom->setValue( 0 );
    m_spinRight->setValue( 0 );
}
void K3bDivxCrop::initPreview( ){
    KURL url(m_data->getProjectDir() + "/vob" );
    m_maxDirSize = KDirSize::dirSize( url )/2048;      // typedef unsigned long long int
    kdDebug()  << "Dirsize: (2KBytes)"  <<  KIO::number( m_maxDirSize ) << endl;
    encodePreview();
    m_preview->setPreviewPicture( m_data->getProjectDir() + "/preview00000.ppm" );
}

void K3bDivxCrop::encodePreview( ){
    previewProcess = new KShellProcess(); // = new KShellProcess;
     *previewProcess << k3bMain()->externalBinManager()->binObject("transcode")->path;
     *previewProcess << " -i " + m_data->getProjectDir() + "/vob";
     *previewProcess << " -x vob -y ppm -V -w 1200 -a 0 -c 4-5"; // -V
     *previewProcess << " -L " + QString::number( m_previewOffset );
     *previewProcess << "-o " + m_data->getProjectDir() + "/preview";
     connect( previewProcess, SIGNAL(receivedStdout(KProcess*, char*, int)),
	       this, SLOT(slotParseProcess(KProcess*, char*, int)) );
     connect( previewProcess, SIGNAL(receivedStderr(KProcess*, char*, int)),
	       this, SLOT(slotParseProcess(KProcess*, char*, int)) );
     if( !previewProcess->start( KProcess::Block, KProcess::AllOutput ) ){
         kdDebug() << "Error process starting" << endl;
     }
     delete previewProcess;
}
void K3bDivxCrop::setSpinBoxMode( int step ){
    m_spinTop->setLineStep( step );
    m_spinLeft->setLineStep( step );
    m_spinBottom->setLineStep( step );
    m_spinRight->setLineStep( step );
}
void K3bDivxCrop::updateSpinBox( int step ){
    setCorrectedSpinValue( m_spinTop, step );
    setCorrectedSpinValue( m_spinLeft, step );
    setCorrectedSpinValue( m_spinBottom, step );
    setCorrectedSpinValue( m_spinRight, step );
}
void K3bDivxCrop::setCorrectedSpinValue( KIntSpinBox *box, int step ){
    int value = box->value();
    int error =  value % step;
    box->setValue( value + (( 8-error )% 8 ) );
}
int K3bDivxCrop::checkMaxValidSpinValue( KIntSpinBox *box, int v ){
    if( m_buttonFast->isChecked() ){
        if( v > box->maxValue() ){
            v =  box->maxValue() - (box->maxValue() % 8);
            box->setValue( v );
        }
    }
    return v;
}

void K3bDivxCrop::enableManuelCrop( bool state ){
      m_spinBottom->setEnabled( state );
      m_spinLeft->setEnabled( state );
      m_spinTop->setEnabled( state );
      m_spinRight->setEnabled( state );
}
void K3bDivxCrop::slotAutoCropMode( int buttonStatus ){
      if( buttonStatus == 0 ){
          enableManuelCrop( true );
      } else if( buttonStatus == 2 ){
          enableManuelCrop( false );
      }
}

void K3bDivxCrop::slotUpdateFinalSize(){
    int h = m_data->getHeightValue() - 8* m_data->getResizeHeight();
    h = h - m_data->getCropTop() - m_data->getCropBottom();
    int w = m_data->getWidthValue() - 8* m_data->getResizeWidth();
    w = w - m_data->getCropLeft() - m_data->getCropRight();
    float a = (float) w / (float) h;
    m_finalSize->setText( QString::number(w) + " x " + QString::number(h) );
    m_finalAspect->setText( "1:" + QString::number( a, 'f', 2 ) );
    // quality videobitrate / ( width *height *fps ), used by GordianKnot
    float q = m_data->getVideoBitrateValue() * 1000 / ( w * h * m_data->getFramerateValue() );
    m_finalQuality->setText( QString::number( q, 'f', 3 ) );
}

void K3bDivxCrop::slotPreviewChanged( int v ){
    m_previewOffset = v*m_maxDirSize/100;
    encodePreview();
    m_preview->updatePreviewPicture( m_data->getProjectDir() + "/preview00000.ppm" );
}

void K3bDivxCrop::slotParseProcess( KProcess* p, char *data, int len){
    QString tmp = QString::fromLatin1( data, len );
    kdDebug() << tmp << endl;
}

void K3bDivxCrop::slotSpinTop( int value){
    int v = checkMaxValidSpinValue( m_spinTop, value );
    m_preview->setTopLine( v );
    m_data->setCropTop( v );
    slotUpdateFinalSize();
}
void K3bDivxCrop::slotSpinLeft( int value){
    int v = checkMaxValidSpinValue( m_spinLeft, value );
    m_preview->setLeftLine( v );
    m_data->setCropLeft( v );
    slotUpdateFinalSize();
}
void K3bDivxCrop::slotSpinBottom( int value){
    int v = checkMaxValidSpinValue( m_spinBottom, value );
    m_preview->setBottomLine( v );
    m_data->setCropBottom( v );
    slotUpdateFinalSize();
}
void K3bDivxCrop::slotSpinRight( int value){
    int v = checkMaxValidSpinValue( m_spinRight, value );
    m_preview->setRightLine( v );
    m_data->setCropRight( v );
    slotUpdateFinalSize();
}

void K3bDivxCrop::slotResizeMode( int id ){
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


#include "k3bdivxcrop.moc"
