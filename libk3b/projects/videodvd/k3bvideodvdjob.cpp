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


#include "k3bvideodvdjob.h"
#include "k3bvideodvddoc.h"
#include "k3bvideodvdimager.h"

#include "k3bcore.h"
#include "k3bisoimager.h"
#include "k3bisooptions.h"
#include "k3bgrowisofswriter.h"
#include "k3bglobals.h"

#include <KConfigCore/KConfig>
#include <KI18n/KLocalizedString>
#include <KWidgetsAddons/KMessageBox>



K3b::VideoDvdJob::VideoDvdJob( K3b::VideoDvdDoc* doc, K3b::JobHandler* jh, QObject* parent )
    : K3b::DataJob( doc, jh, parent ),
      m_doc(doc)
{
}


K3b::VideoDvdJob::~VideoDvdJob()
{
}


void K3b::VideoDvdJob::prepareImager()
{
    setImager( new K3b::VideoDvdImager( m_doc, this ) );
}


QString K3b::VideoDvdJob::jobDescription() const
{
    if( m_doc->onlyCreateImages() ) {
        return i18n("Creating Video DVD Image File");
    }
    else {
        return i18n("Writing Video DVD")
            + ( m_doc->isoOptions().volumeID().isEmpty()
                ? QString()
                : QString( " (%1)" ).arg(m_doc->isoOptions().volumeID()) );
    }
}


QString K3b::VideoDvdJob::jobDetails() const
{
    return ( i18n("ISO9660/Udf Filesystem (Size: %1)",KIO::convertSize( doc()->size() ))
             + ( m_doc->copies() > 1
                 ? i18np(" - %1 copy", " - %1 copies", m_doc->copies())
                 : QString() ) );
}


