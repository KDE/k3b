#include "k3bmixedprojectmodel.h"
#include "k3bdataprojectmodel.h"
#include "k3baudioprojectmodel.h"
#include "k3bmixeddoc.h"

#include <KLocale>
#include <KIcon>

namespace K3b
{
    MixedProjectModel::MixedProjectModel( K3bMixedDoc* doc, QObject* parent )
    : K3bMetaItemModel( parent ), m_doc( doc )
    {
        m_dataModel = new DataProjectModel( doc->dataDoc(), this );
        m_audioModel = new AudioProjectModel( doc->audioDoc(), this );

        addSubModel( i18n("Data Section"), KIcon("media-optical"), m_dataModel, true );
        addSubModel( i18n("Audio Section"), KIcon("media-optical-audio"), m_audioModel);
    }

    MixedProjectModel::~MixedProjectModel()
    {
    }

}
