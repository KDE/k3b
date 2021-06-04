/*
    SPDX-FileCopyrightText: 2017 Pino Toscano <pino@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bcore.h"
#include "k3bexternalbinmanager.h"
#include "k3bthememanager.h"

namespace K3b {

    class Application
    {
    public:
        class Core;
    };

    class Application::Core : public K3b::Core
    {
    public:
        Core( QObject* parent );

        ThemeManager* themeManager() const { return m_themeManager; }

        static Core* k3bAppCore() { return s_k3bAppCore; }

    private:
        ThemeManager* m_themeManager;

        static Core* s_k3bAppCore;
    };
}

K3b::Application::Core* K3b::Application::Core::s_k3bAppCore = 0;

K3b::Application::Core::Core( QObject* parent )
    : K3b::Core( parent )
{
    s_k3bAppCore = this;
    m_themeManager = new ThemeManager( this );

    externalBinManager()->search();
}
