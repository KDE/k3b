/*
    SPDX-FileCopyrightText: 2009 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bvolumenamewidget.h"
#include "k3bdatadoc.h"
#include "k3bisooptions.h"
#include "k3bvalidators.h"

#include <KLineEdit>
#include <KLocalizedString>

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>

namespace K3b {

class VolumeNameWidget::Private
{
public:
    DataDoc* doc;
    KLineEdit* volumeNameEdit;

    void fontChanged( const QFontMetrics& fontMetrics );
};


void VolumeNameWidget::Private::fontChanged( const QFontMetrics& fontMetrics )
{
    volumeNameEdit->setMaximumWidth( fontMetrics.boundingRect('A').width()*50 );
}


VolumeNameWidget::VolumeNameWidget( DataDoc* doc, QWidget* parent )
    : QWidget( parent ),
      d( new Private )
{
    d->doc = doc;

    d->volumeNameEdit = new KLineEdit( doc->isoOptions().volumeID(), this );
    d->volumeNameEdit->setValidator( new Latin1Validator( d->volumeNameEdit ) );
    d->volumeNameEdit->setClearButtonEnabled( true );
    d->fontChanged( fontMetrics() );

    QHBoxLayout* layout = new QHBoxLayout( this );
    layout->addWidget( new QLabel( i18n("Volume Name:"), this ), 1, Qt::AlignRight );
    layout->addWidget( d->volumeNameEdit, 2 );
    layout->setContentsMargins( 0, 0, 0, 0 );

    connect( d->volumeNameEdit, SIGNAL(textChanged(QString)),
             d->doc, SLOT(setVolumeID(QString)) );
    connect( d->doc, SIGNAL(changed()),
             this, SLOT(slotDocChanged()) );
}


VolumeNameWidget::~VolumeNameWidget()
{
    delete d;
}


void VolumeNameWidget::changeEvent( QEvent* event )
{
    if( event->type() == QEvent::FontChange ) {
        d->fontChanged( fontMetrics() );
    }
    QWidget::changeEvent( event );
}


void VolumeNameWidget::slotDocChanged()
{
    // do not update the editor in case it changed the volume id itself
    if( d->doc->isoOptions().volumeID() != d->volumeNameEdit->text() )
        d->volumeNameEdit->setText( d->doc->isoOptions().volumeID() );
}

} // namespace K3b

#include "moc_k3bvolumenamewidget.cpp"
