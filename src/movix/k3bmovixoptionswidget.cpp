/* 
 *
 * $Id: $
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


#include "k3bmovixoptionswidget.h"
#include "k3bmovixdoc.h"

K3bMovixOptionsWidget::K3bMovixOptionsWidget( K3bMovixDoc* doc, QWidget* parent, const char* name )
  : base_K3bMovixOptionsWidget( parent, name ),
    m_doc(doc)
{

}


K3bMovixOptionsWidget::~K3bMovixOptionsWidget()
{
}


#include "k3bmovixoptionswidget.moc"

