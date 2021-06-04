/*
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef _K3B_EXTERNAL_ENCODER_COMMAND_H_
#define _K3B_EXTERNAL_ENCODER_COMMAND_H_

#include <QList>
#include <QString>

class K3bExternalEncoderCommand
{
public:
    K3bExternalEncoderCommand()
        : swapByteOrder(false),
          writeWaveHeader(false) {
    }

    QString name;
    QString extension;
    QString command;

    bool swapByteOrder;
    bool writeWaveHeader;

    static QList<K3bExternalEncoderCommand> defaultCommands();
    static QList<K3bExternalEncoderCommand> readCommands();
    static void saveCommands( const QList<K3bExternalEncoderCommand>& );
};

#endif
