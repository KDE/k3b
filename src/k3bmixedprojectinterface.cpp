/* 
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bmixedprojectinterface.h"
#include "k3bdataprojectinterface.h"
#include "k3baudioprojectinterface.h"

#include <kapplication.h>
#include <dcopclient.h>

#include <k3bmixeddoc.h>
#include <k3bdatadoc.h>
#include <k3baudiodoc.h>


K3b::MixedProjectInterface::MixedProjectInterface( K3b::MixedDoc* doc )
  : K3b::ProjectInterface( doc ),
    m_mixedDoc( doc )
{
  m_dataInterface = new K3b::DataProjectInterface( doc->dataDoc(), objId() + "-datapart" );
  m_audioInterface = new K3b::AudioProjectInterface( doc->audioDoc(), objId() + "-audiopart" );
}


DCOPRef K3b::MixedProjectInterface::dataPart() const
{
  return DCOPRef( kapp->dcopClient()->appId(), m_dataInterface->objId() );
}


DCOPRef K3b::MixedProjectInterface::audioPart() const
{
  return DCOPRef( kapp->dcopClient()->appId(), m_audioInterface->objId() );
}
