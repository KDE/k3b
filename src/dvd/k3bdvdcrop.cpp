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


#include <qlabel.h>
#include <qslider.h>
#include <qcheckbox.h>
#include <qvbuttongroup.h>
#include <qradiobutton.h>
#include <qlayout.h>
#include <qframe.h>

#include <qcanvas.h>

#include <kdialog.h>
#include <klocale.h>
#include <klineedit.h>
#include <kcombobox.h>
#include <knuminput.h>

K3bDvdCrop::K3bDvdCrop(QWidget *parent, const char *name ) : QGroupBox(parent,name) {
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

    QCanvas *c = new QCanvas(300, 150);
    c->setAdvancePeriod(30);
    K3bDvdPreview *pre = new K3bDvdPreview( c, this );

    m_sliderPreview = new QSlider( Horizontal,  this );

    QVButtonGroup *modeGroup = new QVButtonGroup( i18n("Resize mode") );
    m_buttonFast = new QRadioButton( i18n("Fast (-B)"), modeGroup );
    m_buttonExactly = new QRadioButton( i18n("Exactly (-Z)"), modeGroup );
    modeGroup->insert( m_buttonFast  );
    modeGroup->insert( m_buttonExactly  );

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

    mainLayout->addMultiCellWidget( pre, 0, 0, 0, 3);
    mainLayout->addMultiCellWidget( m_sliderPreview, 1, 1, 0, 3);
    mainLayout->addMultiCellWidget( modeGroup, 2, 2, 0, 1);
    mainLayout->addMultiCellWidget( groupCrop, 2, 2, 2, 3);

}

#include "k3bdvdcrop.moc"
