/*
 *
 * $Id$
 * Copyright (C) 2003 Thomas Froescher <tfroescher@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bdivxcrop.h"
#include "k3bdivxpreview.h"
#include "k3bdivxcodecdata.h"
#include <k3bcore.h>
#include <k3bexternalbinmanager.h>

#include <stdlib.h>

#include <qlabel.h>
#include <qslider.h>
#include <qcheckbox.h>
#include <qvbuttongroup.h>
#include <qradiobutton.h>
#include <qlayout.h>
#include <qframe.h>
#include <qimage.h>
#include <qcolor.h>
#include <qcanvas.h>
#include <qwhatsthis.h>
#include <qdir.h>

#include <kdialog.h>
#include <klocale.h>
#include <klineedit.h>
#include <kcombobox.h>
#include <knuminput.h>
#include <kprocess.h>
#include <kdirsize.h>
#include <kdebug.h>

K3bDivxCrop::K3bDivxCrop( K3bDivxCodecData *data, QWidget *parent, const char *name )
  : QGroupBox( parent, name ) {
    m_data = data;
    m_previewOffset = 0;
    setupGui();
}

K3bDivxCrop::~K3bDivxCrop() {}

void K3bDivxCrop::setupGui() {
    setColumnLayout( 0, Qt::Vertical );
    setTitle( i18n( "Cropping Settings" ) );
    QGridLayout *mainLayout = new QGridLayout( layout() );
    mainLayout->setSpacing( KDialog::spacingHint() );
    mainLayout->setMargin( 0 ); // KDialog::marginHint() );

    QCanvas *c = new QCanvas( 380, 300 );  //image 360x288
    //c->setAdvancePeriod(30);
    m_preview = new K3bDivxPreview( c, this );
    QWhatsThis::add( m_preview, i18n( "Preview picture of the video to crop it properly. The colors and quality are not the final state, so use this \
picture only to cut off black borders." ) );
    m_sliderPreview = new QSlider( Horizontal, this );
    initSlider();
    QWhatsThis::add( m_sliderPreview, i18n( "Step through the video to find a light picture to do a proper cut of the black bars." ) );

    QGroupBox *rightColGroup = new QGroupBox( this );
    rightColGroup->setColumnLayout( 0, Qt::Vertical );
    rightColGroup->layout() ->setMargin( 0 );
    QGridLayout *rightLayout = new QGridLayout( rightColGroup->layout() );
    rightLayout->setSpacing( 0 );
    rightLayout->setMargin( 0 );
    rightColGroup->setFrameStyle( Plain | NoFrame );


    QVButtonGroup *modeGroup = new QVButtonGroup( i18n( "Resize Mode" ), rightColGroup );
    modeGroup->layout() ->setMargin( KDialog::marginHint() );
    m_buttonFast = new QRadioButton( i18n( "Fast (-B)" ), modeGroup );
    m_buttonExactly = new QRadioButton( i18n( "Exact (-Z)" ), modeGroup );
    QWhatsThis::add( modeGroup, i18n( "\"Fast\" resizing is much faster than \"Exact\" resizing, but is limited to a resize step multiple of 8 pixels." ) );

    QGroupBox *groupCrop = new QGroupBox( rightColGroup );
    groupCrop->setColumnLayout( 0, Qt::Vertical );
    groupCrop->layout() ->setMargin( 0 ); //KDialog::marginHint() );
    
    QGridLayout *cropLayout = new QGridLayout( groupCrop->layout() );
    cropLayout->setSpacing( KDialog::spacingHint() );
    cropLayout->setMargin( KDialog::marginHint() );
    groupCrop->setTitle( i18n( "Crop Parameters" ) );
    m_spinBottom = new KIntSpinBox( 0, 100, 8, 0, 10, groupCrop );
    m_spinTop = new KIntSpinBox( 0, 100, 8, 0, 10, groupCrop );
    m_spinLeft = new KIntSpinBox( 0, 100, 8, 0, 10, groupCrop );
    m_spinRight = new KIntSpinBox( 0, 100, 8, 0, 10, groupCrop );
    QWhatsThis::add( groupCrop, i18n( "Cut off the black borders of the movie" ) );
    QFrame* line = new QFrame( groupCrop, "line" );
    line->setFrameStyle( QFrame::HLine | QFrame::Sunken );
    m_autoCrop = new QCheckBox( i18n( "Automatically crop" ), groupCrop );

    // transcode DVD - VOB mode
    QVButtonGroup *tcDvdModeGroup = new QVButtonGroup( i18n( "Transcode's Input Mode" ), rightColGroup );
    //m_tcDvdModeGroup->layout() ->setSpacing( KDialog::spacingHint() );
    tcDvdModeGroup->layout() ->setMargin( KDialog::marginHint() );
    //m_tcDvdModeGroup->setFrameStyle( Plain | NoFrame );
    (void)new QRadioButton( i18n( "Read data from VOB-files", "VOB" ), tcDvdModeGroup );
    (void)new QRadioButton( i18n( "Read data in form of DVD", "DVD" ), tcDvdModeGroup );
    tcDvdModeGroup->setButton( 0 );

    QString wt_transcodemode = i18n( "Input mode of transcode given by parameter -x." );
    QWhatsThis::add( tcDvdModeGroup, wt_transcodemode );

    cropLayout->addMultiCellWidget( m_autoCrop, 0, 0, 0, 5 );
    cropLayout->addMultiCellWidget( line, 1, 1, 0, 5 );
    cropLayout->addMultiCellWidget( m_spinTop, 3, 3, 2, 2 );
    cropLayout->addMultiCellWidget( m_spinLeft, 4, 4, 1, 1 );
    cropLayout->addMultiCellWidget( m_spinBottom, 5, 5, 2, 2 );
    cropLayout->addMultiCellWidget( m_spinRight, 4, 4, 3, 3 );

    QGroupBox *groupFinal = new QGroupBox( rightColGroup );
    groupFinal->setColumnLayout( 0, Qt::Vertical );
    groupFinal->layout() ->setMargin( 0 );
    QGridLayout *finalLayout = new QGridLayout( groupFinal->layout() );
    finalLayout->setSpacing( KDialog::spacingHint() );
    finalLayout->setMargin( KDialog::marginHint() );
    groupFinal->setTitle( i18n( "Final Video" ) );
    QLabel *size = new QLabel( i18n( "Size:" ), groupFinal );
    QLabel *aspect = new QLabel( i18n( "Aspect ratio:" ), groupFinal );
    QLabel *quality = new QLabel( i18n( "Video quality:" ), groupFinal );
    m_finalSize = new QLabel( "", groupFinal );
    m_finalAspect = new QLabel( "", groupFinal );
    m_finalQuality = new QLabel( "", groupFinal );
    finalLayout->addWidget( size, 0, 0 );
    finalLayout->addWidget( aspect, 1, 0 );
    finalLayout->addWidget( quality, 2, 0 );
    finalLayout->addWidget( m_finalSize, 0, 1 );
    finalLayout->addWidget( m_finalAspect, 1, 1 );
    finalLayout->addWidget( m_finalQuality, 2, 1 );
    QWhatsThis::add( groupFinal, i18n( "This is the final size of the movie. Verify that the final size of your movie has the right aspect ratio: \
1:1.333 for a 4:3 movie, 1:1.77 for a 16:9 movie, and 1:2.35 for a letterbox movie. If you are using cropping, you can ignore the aspect ratio error shown \
in the resizing box. The \"Video quality\" is a value to estimate the final video quality. (The value is calculated as \"<video bitrate>/(<framerate>*<height>*<width>)\". \
You may know this from GordianKnot (www.doom9.net)). The higher the value the better the quality.\nThe following values are adequate if the movies on viewed on a television. \
Values greater than 0.15 are adequate for letterbox movies. For 16:9 you should use values greater than 0.16 and for 4:3, use greater than 0.17-0.18. Experiment with different values to see what you prefer." ) );

    rightLayout->addMultiCellWidget( modeGroup, 0, 0, 0, 0 );
    rightLayout->addMultiCellWidget( tcDvdModeGroup, 1, 1, 0, 0 );
    rightLayout->addMultiCellWidget( groupCrop, 2, 2, 0, 0 );
    rightLayout->addMultiCellWidget( groupFinal, 3, 3, 0, 0 );
    rightLayout->setRowStretch( 4, 20 );

    (void)new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
    mainLayout->addMultiCellWidget( m_preview, 0, 3, 0, 1 );
    mainLayout->addMultiCellWidget( m_sliderPreview, 4, 4, 0, 1 );
    mainLayout->addMultiCellWidget( rightColGroup, 0, 4, 2, 2 );
    //mainLayout->addItem( spacer, 4, 2 );
    mainLayout->setColStretch( 0, 20 );

    // cropping
    connect( m_spinTop, SIGNAL( valueChanged( int ) ), this, SLOT( slotSpinTop( int ) ) );
    connect( m_spinBottom, SIGNAL( valueChanged( int ) ), this, SLOT( slotSpinBottom( int ) ) );
    connect( m_spinLeft, SIGNAL( valueChanged( int ) ), this, SLOT( slotSpinLeft( int ) ) );
    connect( m_spinRight, SIGNAL( valueChanged( int ) ), this, SLOT( slotSpinRight( int ) ) );
    // resize mode
    connect( modeGroup, SIGNAL( clicked( int ) ), this, SLOT( slotResizeMode( int ) ) );
    connect( m_sliderPreview, SIGNAL( valueChanged( int ) ), this, SLOT( slotPreviewChanged( int ) ) );
    // autocrop
    connect( m_autoCrop, SIGNAL( stateChanged( int ) ), this, SLOT( slotAutoCropMode( int ) ) );
    connect( tcDvdModeGroup, SIGNAL( clicked( int ) ), SLOT( slotTcDvdModeGroup( int ) ) );

    // some init stuff
    modeGroup->setButton( 0 );
}

void K3bDivxCrop::slotUpdateView() {
    kdDebug( ) << "(K3bDivxCrop::slotUpdateView)" << endl;
    m_spinBottom->setMaxValue( m_data->getHeightValue() / 2 - 50 );
    m_spinTop->setMaxValue( m_data->getHeightValue() / 2 - 50 );
    m_spinLeft->setMaxValue( m_data->getWidthValue() / 2 - 100 );
    m_spinRight->setMaxValue( m_data->getWidthValue() / 2 - 100 );
    slotAutoCropMode( m_autoCrop->state() );
}

void K3bDivxCrop::resetView() {
    kdDebug( ) << "(K3bDivxCrop::resetView)" << endl;
    m_preview->resetView();
    resetCrop();
}
void K3bDivxCrop::resetCrop() {
    m_spinTop->setValue( 0 );
    m_spinLeft->setValue( 0 );
    m_spinBottom->setValue( 0 );
    m_spinRight->setValue( 0 );
}
void K3bDivxCrop::initPreview( ) {
    kdDebug( ) << "(K3bDivxCrop::initPreview)" << endl;
    KURL url( m_data->getProjectDir() + "/vob" );
    m_maxDirSize = KDirSize::dirSize( url ) / 2048;      // typedef unsigned long long int
    kdDebug() << "Dirsize: (2KBytes)" << KIO::number( m_maxDirSize ) << endl;
    m_previewOffset = ( int ) m_maxDirSize / 20;
    m_preview->resetView();
    encodePreview();
    //done after slotEncodePreview to verify image exists
    //m_preview->setPreviewPicture( m_data->getProjectDir() + "/preview000000.ppm" );
}

void K3bDivxCrop::initSlider() {
    if ( m_data->isTcDvdMode() ) {
        m_sliderPreview->setRange( 0, m_data->getChapters() );
        if ( m_data->getChapters() > 1 ) {
            m_sliderPreview->setValue( 2 );
        } else {
            m_sliderPreview->setValue( 1 );
        }
        m_sliderPreview->setPageStep( 2 );
        m_sliderPreview->setLineStep( 1 );
    } else {
        m_sliderPreview->setValue( 5 );
        m_sliderPreview->setPageStep( 10 );
        m_sliderPreview->setLineStep( 1 );
        m_sliderPreview->setRange( 0, 100 );
    }
}

void K3bDivxCrop::encodePreview( ) {
    kdDebug( ) << "(K3bDivxCrop::encodePreview)" << endl;
    if ( m_data->isTcDvdMode() ) {
        copyIfos();
    } else {
        // delete ifos
        QDir vobs( m_data->getProjectDir() + "/vob" );
        if ( vobs.exists() ) {
            QStringList ifos = vobs.entryList( "*.ifo" );
            for ( QStringList::Iterator it = ifos.begin(); it != ifos.end(); ++it ) {
                ( *it ) = m_data->getProjectDir() + "/vob/" + ( *it );
            }
            KURL::List ifoList( ifos );
            //        KURL dest( m_dirvob );
            connect( KIO::del( ifoList, false, false ), SIGNAL( result( KIO::Job * ) ), this, SLOT( slotEncodePreview( ) ) );
            kdDebug() << "(K3bDivxCrop) Delete IFO files in " << vobs.path() << endl;
        }
    }
}

void K3bDivxCrop::copyIfos() {
    QDir vobs( m_data->getProjectDir() + "/tmp" );
    if ( vobs.exists() ) {
        QStringList ifos = vobs.entryList( "*.ifo" );
        for ( QStringList::Iterator it = ifos.begin(); it != ifos.end(); ++it ) {
            ( *it ) = m_data->getProjectDir() + "/tmp/" + ( *it );
        }
        KURL::List ifoList( ifos );
        KURL dest( m_data->getProjectDir() + "/vob" );
        // old slot: slotStartAudioProcessing
        connect( KIO::copy( ifoList, dest, false ), SIGNAL( result( KIO::Job * ) ), this, SLOT( slotEncodePreview() ));
        kdDebug() << "(K3bDivxCrop) Copy IFO files to " << dest.path() << endl;
    }
}

void K3bDivxCrop::slotEncodePreview( ) {
    kdDebug( ) << "(K3bDivxCrop::slotEncodePreview)" << endl;
    QString debugString( "" );
    previewProcess = new KShellProcess(); // = new KShellProcess;
    *previewProcess << k3bcore->externalBinManager() ->binObject( "transcode" ) ->path;
    *previewProcess << " -i " + m_data->getProjectDir() + "/vob";
    if ( m_data->isTcDvdMode() ) {
        *previewProcess << " -x dvd -T " + m_data->getTitle() + "," + QString::number( m_sliderPreview->value() ) + ",1";
        debugString += " -x dvd -T " + m_data->getTitle() + "," + QString::number( m_sliderPreview->value() ) + ",1";
    } else {
        m_previewOffset = m_sliderPreview->value() * m_maxDirSize / 100;
        *previewProcess << " -x vob " << " -L " + QString::number( m_previewOffset );
        debugString += " -x vob -L " + QString::number( m_previewOffset );
    }
    *previewProcess << " -y ppm -V -w 1200 -a 0 -c 4-5 "; // -V
    *previewProcess << " -o " + m_data->getProjectDir() + "/preview";
    kdDebug() << k3bcore->externalBinManager() ->binObject( "transcode" ) ->path
    << " -i " + m_data->getProjectDir() + "/vob -y ppm -V -w 1200 -a 0 -c 4-5 "
    << "-o " + m_data->getProjectDir() + "/preview" << debugString << endl;
    connect( previewProcess, SIGNAL( receivedStdout( KProcess*, char*, int ) ),
             this, SLOT( slotParseProcess( KProcess*, char*, int ) ) );
    connect( previewProcess, SIGNAL( receivedStderr( KProcess*, char*, int ) ),
             this, SLOT( slotParseProcess( KProcess*, char*, int ) ) );
    if ( !previewProcess->start( KProcess::Block, KProcess::AllOutput ) ) {
        kdDebug() << "(K3bDivxCrop::slotEncodePreview) Error process starting" << endl;
    }
    delete previewProcess;
    kdDebug() << "(K3bDivxCrop::slotEncodePreview) Set image first time" << endl;
    m_preview->setPreviewPicture( m_data->getProjectDir() + "/preview000000.ppm" );
    m_sliderPreview->setEnabled( true );
    slotAutoCropMode( m_autoCrop->state() );  // update auto cropping
    //emit previewEncoded();
}
void K3bDivxCrop::setSpinBoxMode( int step ) {
    m_spinTop->setLineStep( step );
    m_spinLeft->setLineStep( step );
    m_spinBottom->setLineStep( step );
    m_spinRight->setLineStep( step );
}
void K3bDivxCrop::updateSpinBox( int step ) {
    setCorrectedSpinValue( m_spinTop, step );
    setCorrectedSpinValue( m_spinLeft, step );
    setCorrectedSpinValue( m_spinBottom, step );
    setCorrectedSpinValue( m_spinRight, step );
}
void K3bDivxCrop::setCorrectedSpinValue( KIntSpinBox *box, int step ) {
    int value = box->value();
    int error = value % step;
    box->setValue( value + ( ( 8 - error ) % 8 ) );
}
int K3bDivxCrop::checkMaxValidSpinValue( KIntSpinBox *box, int v ) {
    if ( m_buttonFast->isChecked() ) {
        if ( v > box->maxValue() ) {
            v = box->maxValue() - ( box->maxValue() % 8 );
            box->setValue( v );
        }
    }
    return v;
}

void K3bDivxCrop::enableManuelCrop( bool state ) {
    m_spinBottom->setEnabled( state );
    m_spinLeft->setEnabled( state );
    m_spinTop->setEnabled( state );
    m_spinRight->setEnabled( state );
}
void K3bDivxCrop::slotAutoCropMode( int buttonStatus ) {
    if ( buttonStatus == 0 ) {
        enableManuelCrop( true );
    } else if ( buttonStatus == 2 ) {
        enableManuelCrop( false );
        autoCrop();
    }
}

void K3bDivxCrop::slotUpdateFinalSize() {
    int h = m_data->getHeightValue() - 8 * m_data->getResizeHeight();
    h = h - m_data->getCropTop() - m_data->getCropBottom();
    int w = m_data->getWidthValue() - 8 * m_data->getResizeWidth();
    w = w - m_data->getCropLeft() - m_data->getCropRight();
    float a = ( float ) w / ( float ) h;
    m_finalSize->setText( QString::number( w ) + " x " + QString::number( h ) );
    m_finalAspect->setText( "1:" + QString::number( a, 'f', 2 ) );
    // quality videobitrate / ( width *height *fps ), used by GordianKnot
    float q = m_data->getVideoBitrateValue() * 1000 / ( w * h * m_data->getFramerateValue() );
    m_finalQuality->setText( QString::number( q, 'f', 3 ) );
    emit cropChanged();
}

void K3bDivxCrop::slotPreviewChanged( int ) {
    kdDebug( ) << "(K3bDivxCrop::slotPreviewChanged)" << endl;
    if ( m_tcDvdModeChanged ) {
        m_tcDvdModeChanged = false;
        if( m_data->isTcDvdMode() ){
            m_sliderPreview->setEnabled( false );
        }
    } else {
        kdDebug( ) << "(K3bDivxCrop::slotPreviewChanged) call slotEncodePreview()" << endl;
        slotEncodePreview();
    }
    //m_preview->setPreviewPicture( m_data->getProjectDir() + "/preview000000.ppm" );
    
}

void K3bDivxCrop::slotParseProcess( KProcess*, char *data, int len ) {
    QString tmp = QString::fromLocal8Bit( data, len );
    kdDebug() << tmp << endl;
}

void K3bDivxCrop::slotSpinTop( int value ) {
    int v = checkMaxValidSpinValue( m_spinTop, value );
    m_preview->setTopLine( v );
    m_data->setCropTop( v );
    slotUpdateFinalSize();
}
void K3bDivxCrop::slotSpinLeft( int value ) {
    int v = checkMaxValidSpinValue( m_spinLeft, value );
    m_preview->setLeftLine( v );
    m_data->setCropLeft( v );
    slotUpdateFinalSize();
}
void K3bDivxCrop::slotSpinBottom( int value ) {
    int v = checkMaxValidSpinValue( m_spinBottom, value );
    m_preview->setBottomLine( v );
    m_data->setCropBottom( v );
    slotUpdateFinalSize();
}
void K3bDivxCrop::slotSpinRight( int value ) {
    int v = checkMaxValidSpinValue( m_spinRight, value );
    m_preview->setRightLine( v );
    m_data->setCropRight( v );
    slotUpdateFinalSize();
}

void K3bDivxCrop::slotResizeMode( int id ) {
    switch ( id ) {
            case 0: {
                setSpinBoxMode( 8 );
                updateSpinBox( 8 );
                break;
            }
            case 1: {
                setSpinBoxMode( 1 );
                break;
            }
            default:
            break;
    }
}

void K3bDivxCrop::autoCrop() {
    resetCrop();
    QImage i( m_data->getProjectDir() + "/preview000000.ppm" );
    kdDebug() << "(K3bDivxCrop::autoCrop) Image: " << QString::number( i.width() ) << "x" << QString::number( i.height() ) << endl;
    int difArray[ 25 ];
    int topCrop = 0;
    int bottomCrop = 0;
    // top line
    for ( unsigned int y = 0; y < 200; y++ ) {
        int index = 0;
        m_firstPictureLine = ( y == 0 ) ? true : false;
        for ( unsigned int x = 50; x < 654; x = x + 25 ) {
            difArray[ index ] = checkPixels( &i, x, y, 0, 1 );
            index++;
        }
        if ( checkLine( difArray ) ) {
            //kdDebug() << "Found line: " << QString::number( y ) << endl;
            int correct = y - ( y % m_spinTop->lineStep() );
            //kdDebug() << "Corrected line: " << QString::number( correct ) << endl;
            m_spinTop->setValue( correct );
            slotSpinTop( correct );
            topCrop = correct;
            break;
        }
    }
    // bottom line
    for ( unsigned int y = 575; y > 300; y-- ) {
        m_firstPictureLine = ( y == 575 ) ? true : false;
        int index = 0;
        for ( unsigned int x = 50; x < 654; x = x + 25 ) {
            difArray[ index ] = checkPixels( &i, x, y, 0, -1 );
            index++;
        }
        if ( checkLine( difArray ) ) {
            bottomCrop = 576 - y;
            //kdDebug() << "Found line: " << QString::number( y ) << endl;
            int correct = bottomCrop - ( bottomCrop % m_spinBottom->lineStep() );
            //kdDebug() << "Corrected line: " << QString::number( correct ) << endl;
            m_spinBottom->setValue( correct );
            slotSpinBottom( correct );
            break;
        }
    }

    // left line
    for ( unsigned int x = 0; x < 200; x++ ) {
        m_firstPictureLine = ( x == 0 ) ? true : false;
        int index = 0;

        for ( unsigned int y = 140; y < 430; y = y + 12 ) {
            difArray[ index ] = checkPixels( &i, x, y, 1, 0 );
            index++;
        }
        if ( checkLine( difArray ) ) {
            //kdDebug() << "Found line: " << QString::number( x ) << endl;
            int correct = x - ( x % m_spinLeft->lineStep() );
            //kdDebug() << "Corrected line: " << QString::number( correct ) << endl;
            m_spinLeft->setValue( correct );
            slotSpinLeft( correct );
            break;
        }
    }
    // right line
    for ( unsigned int x = 719; x > 520; x-- ) {
        m_firstPictureLine = ( x == 719 ) ? true : false;
        int index = 0;
        for ( unsigned int y = 140; y < 430; y = y + 12 ) {
            difArray[ index ] = checkPixels( &i, x, y, -1, 0 );
            index++;
        }
        if ( checkLine( difArray ) ) {
            int r = 720 - x;
            //kdDebug() << "Found line: " << QString::number( r ) << endl;
            int correct = r - ( r % m_spinRight->lineStep() );
            //kdDebug() << "Corrected line: " << QString::number( correct ) << endl;
            m_spinRight->setValue( correct );
            slotSpinRight( correct );
            break;
        }
    }
    emit cropChanged();
}
int K3bDivxCrop::checkPixels( QImage *i, int x, int y, int xoffset, int yoffset ) {
    int result = 0;
    int grey = 0;
    if ( !m_firstPictureLine ) {
        grey = qGray( i->pixel( x, y ) );
    }
    int grey2 = qGray( i->pixel( x + xoffset, y + yoffset ) );
    int difference = grey2 - grey;
    if ( difference > 0 ) {
        //kdDebug() << "( " + QString::number(x) + "," + QString::number(y) + "): " << QString::number( difference )
        //<< "," << QString::number( grey ) << "," << QString::number( grey2 ) << endl;
        result = difference;
    }
    return result;
}
bool K3bDivxCrop::checkLine( int difArray[ 25 ] ) {
    int cropOk = 0;
    int cropOk2 = 0;
    int cropOk3 = 0;
    for ( int a = 0; a < 25; a++ ) {
        if ( difArray[ a ] > 10 ) {
            cropOk++;
        } else if ( difArray[ a ] > 2 ) {
            cropOk2++;
        } else if ( difArray[ a ] > 20 ) {
            cropOk3++;
        }
    }
    if ( cropOk > 5 ||
         cropOk2 > 9 ||
         cropOk3 > 1 ) {
        return true;
    }
    return false;
}

void K3bDivxCrop::slotTcDvdModeGroup( int id ) {
    // state = 0 is vob, 1 dvd
    kdDebug() << "(K3bDivxCrop) Transcode mode is: " << id << endl;
    // avoid preview encoding due to slider value changes
    m_tcDvdModeChanged = true;
    if ( id == 0 ) {
        m_data->setTcDvdMode( false );
        initSlider();
        encodePreview();
    } else if ( id == 1 ) {
        m_data->setTcDvdMode( true );
        m_sliderPreview->setEnabled( false );
        initSlider();
        copyIfos();
    }
}


#include "k3bdivxcrop.moc"
