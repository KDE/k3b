/*
    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_AUDIO_PROJECT_CDDB_PLUGIN_H_
#define _K3B_AUDIO_PROJECT_CDDB_PLUGIN_H_

#include "k3bprojectplugin.h"

#include <QPointer>
#include <QScopedPointer>

namespace K3b {
    class Cddb;
    class AudioDoc;
    class ProgressDialog;
}

class QWidget;
class QProgressDialog;
class KJob;

class K3bAudioProjectCddbPlugin : public K3b::ProjectPlugin
{
    Q_OBJECT

public:
    K3bAudioProjectCddbPlugin( QObject* parent, const QVariantList& );
    ~K3bAudioProjectCddbPlugin() override;

    int pluginSystemVersion() const override { return K3B_PLUGIN_SYSTEM_VERSION; }

    void activate( K3b::Doc* doc, QWidget* parent ) override;

private Q_SLOTS:
    void slotCddbQueryFinished( KJob* );

private:
    QScopedPointer<QProgressDialog> m_progress;
    QPointer<K3b::AudioDoc> m_doc;
    QPointer<QWidget> m_parentWidget;

    bool m_canceled;
};

#endif
