/*

    SPDX-FileCopyrightText: 2003-2008 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/


#ifndef K3B_MSF_EDIT_H
#define K3B_MSF_EDIT_H


#include <QAbstractSpinBox>

#include "k3bmsf.h"
#include "k3b_export.h"

namespace K3b {
    class LIBK3B_EXPORT MsfEdit : public QAbstractSpinBox
    {
        Q_OBJECT

    public:
        explicit MsfEdit( QWidget* parent = 0 );
        ~MsfEdit() override;

        Msf minimum() const;
        Msf maximum() const;
        Msf value() const;

        void stepBy( int steps ) override;
        QSize sizeHint() const override;

    Q_SIGNALS:
        void valueChanged( const K3b::Msf& value );

    public Q_SLOTS:
        void setMinimum( const Msf& min );
        void setMaximum( const Msf& max );
        void setValue( const K3b::Msf& value );
        
    protected:
        StepEnabled stepEnabled () const override;

    private:
        class Private;
        Private* d;

        Q_PRIVATE_SLOT( d, void _k_editingFinished() )
    };
}


#endif
