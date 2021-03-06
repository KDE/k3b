/*
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_<name>_ENCODER_H_
#define _K3B_<name>_ENCODER_H_


#include "k3baudioencoder.h"
#include "k3bpluginconfigwidget.h"


class K3b<name>Encoder : public K3b::AudioEncoder
{
    Q_OBJECT

public:
    K3b<name>Encoder( QObject* parent, const QVariantList& );
    ~K3b<name>Encoder();

    QStringList extensions() const;
  
    QString fileTypeComment( const QString& ) const;

    long long fileSize( const QString&, const K3b::Msf& msf ) const;

    int pluginSystemVersion() const { return K3B_PLUGIN_SYSTEM_VERSION; }

private:
    void finishEncoderInternal();
    bool initEncoderInternal( const QString& extension, const Msf& length, const MetaData& metaData );
    long encodeInternal( const char* data, Q_ULONG len );
};


class K3b<name>EncoderConfigWidget : public K3b::PluginConfigWidget
{
    Q_OBJECT

public:
    explicit K3b<name>EncoderConfigWidget( QWidget* parent = 0 );
    ~K3b<name>EncoderConfigWidget();

    public Q_SLOTS:
    virtual void load();
    virtual void save();
    virtual void defaults();
};

#endif
