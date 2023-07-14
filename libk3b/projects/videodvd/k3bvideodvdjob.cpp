/*
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/


#include "k3bvideodvdjob.h"
#include "k3bvideodvddoc.h"
#include "k3bvideodvdimager.h"
#include "k3bcore.h"
#include "k3bisoimager.h"
#include "k3bisooptions.h"
#include "k3bgrowisofswriter.h"
#include "k3bglobals.h"
#include "k3b_i18n.h"

#include <KConfig>
#include <KMessageBox>



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
    return ( i18n("ISO 9660/Udf Filesystem (Size: %1)",KIO::convertSize( doc()->size() ))
             + ( m_doc->copies() > 1
                 ? i18np(" - %1 copy", " - %1 copies", m_doc->copies())
                 : QString() ) );
}

#include "moc_k3bvideodvdjob.cpp"
