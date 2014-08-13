/*
 *
 * Copyright (C) 2005-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bmediaselectiondialog.h"
#include "k3bmediaselectioncombobox.h"
#include "k3bmediacache.h"
#include "k3bapplication.h"

#include <KI18n/KLocalizedString>

#include <qlabel.h>
#include <QGridLayout>


K3b::MediaSelectionDialog::MediaSelectionDialog( QWidget* parent,
                                                 const QString& title,
                                                 const QString& text,
                                                 bool modal )
    : KDialog( parent)
{
    QWidget *widget = new QWidget();
    setMainWidget(widget);
    setWindowTitle(title.isEmpty() ? i18n("Medium Selection") : title);
    setButtons (Ok|Cancel);
    setModal(modal);
    setDefaultButton(Ok);
    QGridLayout* lay = new QGridLayout( widget );

    QLabel* label = new QLabel( text.isEmpty() ? i18n("Please select a medium:") : text, widget );
    m_combo = new K3b::MediaSelectionComboBox( widget );

    lay->addWidget( label, 0, 0 );
    lay->addWidget( m_combo, 1, 0 );
    lay->setRowStretch( 2, 1 );

    connect( m_combo, SIGNAL(selectionChanged(K3b::Device::Device*)),
             this, SLOT(slotSelectionChanged(K3b::Device::Device*)) );

    slotSelectionChanged( m_combo->selectedDevice() );
}


K3b::MediaSelectionDialog::~MediaSelectionDialog()
{
}


void K3b::MediaSelectionDialog::setWantedMediumType( Device::MediaTypes type )
{
    m_combo->setWantedMediumType( type );
}


void K3b::MediaSelectionDialog::setWantedMediumState( Device::MediaStates state )
{
    m_combo->setWantedMediumState( state );
}


void K3b::MediaSelectionDialog::setWantedMediumContent( Medium::MediumContents content )
{
    m_combo->setWantedMediumContent( content );
}


K3b::Device::Device* K3b::MediaSelectionDialog::selectedDevice() const
{
    return m_combo->selectedDevice();
}


void K3b::MediaSelectionDialog::slotSelectionChanged( K3b::Device::Device* dev )
{
    enableButtonOk( dev != 0 );
}


K3b::Device::Device* K3b::MediaSelectionDialog::selectMedium( Device::MediaTypes type,
                                                              Device::MediaStates state,
                                                              Medium::MediumContents content,
                                                              QWidget* parent,
                                                              const QString& title, const QString& text,
                                                              bool* canceled )
{
    K3b::MediaSelectionDialog dlg( parent, title, text );
    dlg.setWantedMediumType( type );
    dlg.setWantedMediumState( state );
    dlg.setWantedMediumContent( content );

    // even if no usable medium is inserted the combobox shows the "insert one" message
    // so it's not sufficient to check for just one entry to check if there only is a
    // single useable medium
    if( ( dlg.selectedDevice() && dlg.m_combo->count() == 1 )
        || dlg.exec() == Accepted ) {
        if( canceled )
            *canceled = false;
        return dlg.selectedDevice();
    }
    else {
        if( canceled )
            *canceled = true;
        return 0;
    }
}


