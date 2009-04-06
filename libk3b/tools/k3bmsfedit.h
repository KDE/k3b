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


#ifndef K3B_MSF_EDIT_H
#define K3B_MSF_EDIT_H


#include <QtGui/QSpinBox>
#include <qvalidator.h>

#include "k3bmsf.h"
#include "k3b_export.h"

namespace K3b {
    class MsfValidator : public QRegExpValidator
    {
    public:
        MsfValidator( QObject* parent = 0 );
    };
}


namespace K3b {
    class LIBK3B_EXPORT MsfEdit : public QSpinBox
    {
        Q_OBJECT

    public:
        MsfEdit( QWidget* parent = 0 );
        ~MsfEdit();

        Msf msfValue() const;

        void stepBy( int steps );

    Q_SIGNALS:
        void valueChanged( const K3b::Msf& );

    public Q_SLOTS:
        void setMsfValue( const K3b::Msf& );

    private:
        QString textFromValue( int value ) const ;
        int valueFromText( const QString & text ) const;

        class Private;
        Private* d;

        Q_PRIVATE_SLOT( d, void _k_valueChanged(int) )
    };
}


#endif
