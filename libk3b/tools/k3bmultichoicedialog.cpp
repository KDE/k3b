/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bmultichoicedialog.h"
#include "k3bstdguiitems.h"

#include <KApplication>
#include <KDialog>
#include <KIconLoader>
#include <KPushButton>

#include <QCloseEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QSignalMapper>


class K3b::MultiChoiceDialog::Private
{
public:
    Private()
        : mapper(0),
          buttonLayout(0) {
    }

    QSignalMapper* mapper;
    QList<KPushButton*> buttons;
    QHBoxLayout* buttonLayout;

    bool buttonClicked;
};


// from kmessagebox.cpp
static QIcon themedMessageBoxIcon(QMessageBox::Icon icon)
{
    QString icon_name;

    switch (icon) {
    case QMessageBox::NoIcon:
        return QIcon();
        break;
    case QMessageBox::Information:
        icon_name = "dialog-information";
        break;
    case QMessageBox::Warning:
        icon_name = "dialog-warning";
        break;
    case QMessageBox::Critical:
        icon_name = "dialog-error";
        break;
    default:
        break;
    }

    QIcon ret = KIconLoader::global()->loadIcon(icon_name, KIconLoader::NoGroup, KIconLoader::SizeLarge, KIconLoader::DefaultState, QStringList(), 0, true);

    if (ret.isNull()) {
        return QMessageBox::standardIcon(icon);
    } else {
        return ret;
    }
}


K3b::MultiChoiceDialog::MultiChoiceDialog( const QString& caption,
                                            const QString& text,
                                            QMessageBox::Icon icon,
                                            QWidget* parent )
    : QDialog( parent )
{
    d = new Private();
    d->mapper = new QSignalMapper( this );
    connect( d->mapper, SIGNAL(mapped(int)), this, SLOT(done(int)) );

    setWindowTitle( caption );

    QGridLayout* mainGrid = new QGridLayout( this );

    QHBoxLayout* contents = new QHBoxLayout;
    contents->setSpacing( KDialog::spacingHint()*2 );
    contents->setMargin( 0 );

    QLabel* pixLabel = new QLabel( this );
    int size = IconSize(KIconLoader::Dialog);
    pixLabel->setPixmap( themedMessageBoxIcon( icon ).pixmap( size, size ) );
    pixLabel->setScaledContents( false );
    QLabel* label = new QLabel( text, this );
    label->setWordWrap( true );
    contents->addWidget( pixLabel, 0 );
    contents->addWidget( label, 1 );

    d->buttonLayout = new QHBoxLayout;
    d->buttonLayout->setMargin( 0 );

    mainGrid->addLayout( contents, 0, 0, 1, 3 );
    mainGrid->addWidget( K3b::StdGuiItems::horizontalLine( this ), 1, 0, 1, 3 );
    mainGrid->addLayout( d->buttonLayout, 2, 1 );

    mainGrid->setColumnStretch( 0, 1 );
    mainGrid->setColumnStretch( 2, 1 );
    mainGrid->setRowStretch( 0, 1 );
}


K3b::MultiChoiceDialog::~MultiChoiceDialog()
{
    delete d;
}


int K3b::MultiChoiceDialog::addButton( const KGuiItem& b )
{
    KPushButton* button = new KPushButton( b, this );
    d->buttonLayout->addWidget( button );
    d->buttons.append(button);
    d->mapper->setMapping( button, d->buttons.count() );
    connect( button, SIGNAL(clicked()), d->mapper, SLOT(map()) );
    return d->buttons.count();
}


void K3b::MultiChoiceDialog::slotButtonClicked( int code )
{
    d->buttonClicked = true;
    done( code );
}


int K3b::MultiChoiceDialog::exec()
{
    d->buttonClicked = false;
    return QDialog::exec();
}


void K3b::MultiChoiceDialog::closeEvent( QCloseEvent* e )
{
    // make sure the dialog can only be closed by the buttons
    // otherwise we may get an undefined return value in exec

    if( d->buttonClicked )
        QDialog::closeEvent( e );
    else
        e->ignore();
}


int K3b::MultiChoiceDialog::choose( const QString& caption,
                                  const QString& text,
                                  QMessageBox::Icon icon,
                                  QWidget* parent,
                                  int buttonCount,
                                  const KGuiItem& b1,
                                  const KGuiItem& b2,
                                  const KGuiItem& b3,
                                  const KGuiItem& b4,
                                  const KGuiItem& b5,
                                  const KGuiItem& b6 )
{
    K3b::MultiChoiceDialog dlg( caption, text, icon, parent );
    dlg.addButton( b1 );
    if( buttonCount > 1 )
        dlg.addButton( b2 );
    if( buttonCount > 2 )
        dlg.addButton( b3 );
    if( buttonCount > 3 )
        dlg.addButton( b4 );
    if( buttonCount > 4 )
        dlg.addButton( b5 );
    if( buttonCount > 5 )
        dlg.addButton( b6 );

    return dlg.exec();
}


#include "k3bmultichoicedialog.moc"
