/***************************************************************************
                          k3brippingpatternoptiontab.cpp  -  description
                             -------------------
    begin                : Fri Nov 23 2001
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

#include "k3brippingpatternoptiontab.h"
//#include "../kiotree/kiotree.h"
#include "../rip/k3bpatternwidget.h"
#include "../rip/k3bcdview.h"

#include <qcheckbox.h>
#include <qlayout.h>
#include <qframe.h>
#include <qvgroupbox.h>

#include <kdialog.h>
#include <klocale.h>
#include <kapp.h>
#include <kconfig.h>

K3bRippingPatternOptionTab::K3bRippingPatternOptionTab(QWidget *parent, const char *name ) : QWidget(parent,name) {
    setup();
}
K3bRippingPatternOptionTab::~K3bRippingPatternOptionTab(){
}

void K3bRippingPatternOptionTab::setup(){

    QGridLayout *frameLayout = new QGridLayout( this );
    frameLayout->setSpacing( KDialog::spacingHint() );
    frameLayout->setMargin( 0 );

    QVGroupBox *groupPattern = new QVGroupBox( this, "use_pattern" );
    groupPattern->setTitle( i18n( "General" ) );
    m_usePattern = new QCheckBox(i18n("Use pattern per default."), groupPattern, "filename_patternDir_box");

    m_frame = new K3bPatternWidget( this );

    frameLayout->addWidget( groupPattern, 0, 0);
    frameLayout->addWidget( m_frame, 1, 0);
}

void K3bRippingPatternOptionTab::init( QString& album ){
    m_frame->init( album, DEFAULT_ARTIST, DEFAULT_TITLE, "01" );
}

void K3bRippingPatternOptionTab::apply(){
    m_frame->apply();
    KConfig* c = kapp->config();
    c->setGroup("Ripping");
    c->writeEntry( "usePattern", m_usePattern->isChecked() );
}

void K3bRippingPatternOptionTab::readSettings(){
    m_frame->readSettings();
    KConfig* c = kapp->config();
    c->setGroup("Ripping");
    m_usePattern->setChecked( c->readBoolEntry("usePattern", true) );
}


#include "k3brippingpatternoptiontab.moc"
