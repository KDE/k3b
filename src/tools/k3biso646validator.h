/*
 *
 * $Id: $
 * Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef K3BISO646VALIDATOR_H
#define K3BISO646VALIDATOR_H


/**
  *@author Christian Kvasny
  */

#include <qvalidator.h>

class K3bIso646Validator : public QRegExpValidator
{
    Q_OBJECT

    public: 
        K3bIso646Validator(QObject *parent, const char *name = 0 );
        K3bIso646Validator( int , QObject *parent, const char *name = 0 );
        K3bIso646Validator( int ,  bool , QObject *parent, const char *name = 0 );

        ~K3bIso646Validator();

        /*
        Notes.
        (1) d-characters are: A-Z, 0-9, _ (see ISO-9660:1988, Annex A, Table 15)
        (2) a-characters are: A-Z, 0-9, _, space, !, ", %, &, ', (, ), *, +, ,, -, ., /, :, ;, <, =, >, ? (see ISO-9660:1988, Annex A, Table 14)
        */
        enum Type { Iso646_a, Iso646_d };

        const int isoType() const { return m_type; }
        const bool allowLowerCase() const { return m_allowlowercase; }
        QRegExp setIsoType( int , bool AllowLowerCase = false );

        private:
            int m_type;
            bool m_allowlowercase;
        
};

#endif
