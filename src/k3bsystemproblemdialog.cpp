/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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


#include "k3bsystemproblemdialog.h"

K3bSystemProblem::K3bSystemProblem( int t,
				    const QString& p,
				    const QString& d,
				    const QString& s,
				    bool k )
  : type(t),
    problem(p),
    details(d),
    solution(s),
    solvableByK3bSetup(k)
{
}


K3bSystemProblemDialog::K3bSystemProblemDialog( const QValueList<K3bSystemProblem>&,
						QWidget* parent, 
						const char* name )
  : KDialog( parent, name )
{

}

#include "k3bsystemproblemdialog.moc"
