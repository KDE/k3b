/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_EXTERNAL_ENCODER_COMMAND_H_
#define _K3B_EXTERNAL_ENCODER_COMMAND_H_

#include <QtCore/QList>
#include <QtCore/QString>

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
