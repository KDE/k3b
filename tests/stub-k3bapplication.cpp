/*
 * Copyright (C) 2017 Pino Toscano <pino@kde.org>
 *
 * This file is part of the K3b project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
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
