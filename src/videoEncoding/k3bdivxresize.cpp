/***************************************************************************
                          k3bdivxresize.cpp  -  description
                             -------------------
    begin                : Sat Apr 6 2002
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

#include "k3bdivxresize.h"
#include "k3bdivxcodecdata.h"

#include <qlayout.h>
#include <qlabel.h>
#include <qhgroupbox.h>
#include <qframe.h>
#include <qslider.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qwhatsthis.h>

#include <kdialog.h>
#include <klocale.h>
#include <kcombobox.h>
#include <kdebug.h>

K3bDivxResize::K3bDivxResize(K3bDivxCodecData *data, QWidget *parent, const char *name ) : QGroupBox(parent,name) {
    m_data = data;
    setupGui();
}

K3bDivxResize::~K3bDivxResize(){
}

void K3bDivxResize::setupGui(){
    setColumnLayout(0, Qt::Vertical );
    setTitle( i18n( "Resizing" ) );
    QGridLayout *mainLayout = new QGridLayout( layout() );
    mainLayout->setSpacing( KDialog::spacingHint() );
    //mainLayout->setMargin( KDialog::marginHint() );

    QLabel *resize = new QLabel( i18n("Resize:"), this );
    m_sliderResize = new QSlider( 0, 704, 16, 0, Horizontal,  this );
    m_sliderResize->setTickmarks( QSlider::Below );
    m_sliderResize->setTracking( true );
    m_sliderResize->setLineStep( 32 );
    m_sliderResize->setPageStep( 16 );
    m_sliderResize->setValue( 704 );
    QWhatsThis::add( m_sliderResize, i18n("Resizes the final output video depending on the aspect ratio of the movie. Before resizing the video \
the image should already be cropped, so it can be used to detect the real aspect ratio of the movie. If the aspect ratio is not properly detected you must \
correct the height of the video to fit it."));
    QHGroupBox *groupSize = new QHGroupBox( this );
    groupSize->setFrameStyle( Plain | NoFrame );
    groupSize->layout()->setSpacing( 0 );
    groupSize->layout()->setMargin( 0 );
    QWhatsThis::add( groupSize, i18n("The aspect ratio error shows the difference from the original aspect ratio. If cropping is used, the apsect \
ratio error shows the difference from the \"best match\" aspect ratio (i.e. 4:3, 16:9 or letterbox 1:2.35). To correct the aspect ratio manually change \
the height."));
    QLabel *width = new QLabel( i18n("Width:"), groupSize );
    m_labelWidth = new QLabel( "", groupSize );
    QLabel *height = new QLabel( i18n("Height:"), groupSize );
    m_comboHeight = new KComboBox( groupSize );
    QStringList heights;
    for( int i=576; i > 63; i=i-8){
        heights << QString::number( i );
    }
    m_comboHeight->insertStringList( heights );
    QLabel *aspectRatio = new QLabel( i18n("Aspect ratio:"), groupSize);
    m_labelAspectRatio = new QLabel( "", groupSize );

    QLabel *aspectError = new QLabel( i18n("Aspect ratio error:"), groupSize );
    m_labelAspectError = new QLabel( "", groupSize );

    QFrame* line = new QFrame( this, "line" );
    line->setFrameStyle( QFrame::HLine | QFrame::Sunken );

    QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );

    mainLayout->addItem( spacer, 0, 0);
    mainLayout->addMultiCellWidget( groupSize, 0, 0, 2, 3);
    mainLayout->addMultiCellWidget( line, 1, 1, 0, 3);
    mainLayout->addMultiCellWidget( resize, 2, 2, 0, 0);
    mainLayout->addMultiCellWidget( m_sliderResize, 2, 2, 1, 3);
    mainLayout->setColStretch( 1, 20);

    connect( m_sliderResize, SIGNAL( valueChanged( int )), this, SLOT( slotResizeChanged( int )) );
    connect( m_comboHeight, SIGNAL( activated( int )), this, SLOT( slotHeightChanged( int )) );
}

void K3bDivxResize::initView(){
    m_orginalAspect = m_data->getAspectRatioValue();
    m_realAspect = m_orginalAspect;
    slotResizeChanged( 704 );
    //updateView(); // done in slotResizeChanged
}

void K3bDivxResize::resetView(){
    kdDebug() << "(K3bDivxResize::resetView) Reset slider" << endl;
    m_sliderResize->setValue(704);
}

void K3bDivxResize::slotUpdateView(){
     kdDebug() << "(K3bDivxResize::slotUpdateView)" << endl;
     m_labelWidth->setText( QString::number( m_data->getWidthValue() - (m_data->getResizeWidth()*8) ) );
     int currentHeight = m_data->getHeightValue() -( m_data->getResizeHeight() *8 );
     int currentWidth = m_data->getWidthValue() - ( m_data->getResizeWidth() *8 );
     // estimate real aspect
     int cropWidth = m_data->getCropLeft() + m_data->getCropRight();
     int cropHeight = m_data->getCropTop() + m_data->getCropBottom();
     if( cropWidth > 0 || cropHeight > 0){
         m_realAspect = (( float ) (m_data->getWidthValue() - cropWidth)) / ((float) (m_data->getHeightValue() - cropHeight));
         // check for anamorph
         if( m_data->isAnamorph() ){
             if( m_realAspect < 1.5 ){
                 // true 16:9 anamorph
                 m_realAspect = 1.778;
             } else if ( m_realAspect < 2.05 ){
                 // widescreen 1:2.35 (16:9) anamorph
                 m_realAspect = 2.35;
             }
             // check for cropped aspect ratio
         } else if( m_realAspect > 2.05 ){
             m_realAspect = 2.35;
         } else if ( m_realAspect > 1.55 ){
             m_realAspect = 1.778;
         } else {
             m_realAspect = 1.333;
         }
     } else {
         m_realAspect = m_data->getAspectRatioValue();
     }
     m_currentAspect = ( (float)(currentWidth-m_data->getCropLeft()-m_data->getCropRight() ) ) /
     ( (float) (currentHeight-m_data->getCropTop() - m_data->getCropBottom() ) );

     m_labelAspectRatio->setText( "1:" + QString::number( m_currentAspect, 'f', 2) );

     float error = ( m_realAspect / m_currentAspect -1) *100;
     m_labelAspectError->setText( QString::number( error, 'f', 2) + " %" );
}

void K3bDivxResize::slotResizeChanged( int value ){
     kdDebug() << "(K3bDivxResize::slotResizeChanged)" << endl;
     m_data->setResizeWidth( (704-value) / 8 );
     int h = (int) ((value-m_data->getCropLeft() - m_data->getCropRight() ) / m_realAspect);
     h = h + m_data->getCropBottom() + m_data->getCropTop();
     h = h + ( (8- (h % 8)) % 8) + 8; // last +8 is experience, most times one step to small
     int comboIndex = 72 - (h / 8);
     m_data->setResizeHeight( comboIndex );
     m_comboHeight->setCurrentItem( comboIndex );
     //slotUpdateView();
     emit sizeChanged();
}

void K3bDivxResize::slotHeightChanged( int index ){
    m_data->setResizeHeight( index );
    //slotUpdateView();
    emit sizeChanged();
}
#include "k3bdivxresize.moc"
