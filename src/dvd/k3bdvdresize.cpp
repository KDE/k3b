/***************************************************************************
                          k3bdvdresize.cpp  -  description
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

#include "k3bdvdresize.h"

#include <qlayout.h>
#include <qlabel.h>
#include <qhgroupbox.h>
#include <qframe.h>
#include <qslider.h>
#include <qstring.h>
#include <qstringlist.h>

#include <kdialog.h>
#include <klocale.h>
#include <kcombobox.h>

K3bDvdResize::K3bDvdResize(QWidget *parent, const char *name ) : QGroupBox(parent,name) {
    setupGui();
}

K3bDvdResize::~K3bDvdResize(){
}

void K3bDvdResize::setupGui(){
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
    m_sliderResize->setPageStep( 32 );

    QHGroupBox *groupSize = new QHGroupBox( this );
    groupSize->setFrameStyle( Plain | NoFrame );
    groupSize->layout()->setSpacing( 0 );
    groupSize->layout()->setMargin( 0 );
    //cropLayout->setMargin( KDialog::marginHint() );
    QLabel *width = new QLabel( i18n("Width:"), groupSize );
    m_labelWidth = new QLabel( "", groupSize );
    QLabel *height = new QLabel( i18n("Height:"), groupSize );
    m_comboHeight = new KComboBox( groupSize );
    QStringList heights;
    for( int i=576; i > 32; i=i-32){
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
}


#include "k3bdvdresize.moc"
