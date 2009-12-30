/*
 *
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_SOX_ENCODER_H_
#define _K3B_SOX_ENCODER_H_

#include "k3bprocess.h"

#include "k3baudioencoder.h"
#include "k3bpluginconfigwidget.h"
#include "ui_base_k3bsoxencoderconfigwidget.h"


class K3bSoxEncoder : public K3b::AudioEncoder
{
    Q_OBJECT

public:
    K3bSoxEncoder( QObject* parent, const QVariantList& );
    ~K3bSoxEncoder();

    virtual QStringList extensions() const;

    virtual QString fileTypeComment( const QString& ) const;

    virtual long long fileSize( const QString&, const K3b::Msf& msf ) const;

    virtual int pluginSystemVersion() const { return K3B_PLUGIN_SYSTEM_VERSION; }

    virtual K3b::PluginConfigWidget* createConfigWidget( QWidget* parent = 0) const;

    /**
     * reimplemented since sox writes the file itself
     */
    virtual bool openFile( const QString& ext, const QString& filename, const K3b::Msf& );
    virtual bool isOpen() const;
    virtual void closeFile();

private Q_SLOTS:
    void slotSoxFinished( int, QProcess::ExitStatus );
    void slotSoxOutputLine( const QString& );

private:
    virtual bool initEncoderInternal( const QString& extension, const K3b::Msf& length );
    virtual void finishEncoderInternal();
    virtual long encodeInternal( const char* data, Q_ULONG len );

    class Private;
    Private* d;
};


class K3bSoxEncoderSettingsWidget : public K3b::PluginConfigWidget, public Ui::base_K3bSoxEncoderConfigWidget
{
    Q_OBJECT

public:
    K3bSoxEncoderSettingsWidget( QWidget* parent = 0 );
    ~K3bSoxEncoderSettingsWidget();

    public Q_SLOTS:
    void loadConfig();
    void saveConfig();
};

#endif
