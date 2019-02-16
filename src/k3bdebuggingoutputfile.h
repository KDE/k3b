/*
 *
 * Copyright (C) 2005-2008 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_DEBUGGING_OUTPUT_FILE_H_
#define _K3B_DEBUGGING_OUTPUT_FILE_H_

#include <QFile>
#include <QObject>

namespace K3b {
    class DebuggingOutputFile : public QFile
    {
        Q_OBJECT

    public:
        DebuggingOutputFile();

        /**
         * Open the default output file and write some system information.
         */
        bool open( OpenMode mode = WriteOnly ) override;

    public Q_SLOTS:
        void addOutput( const QString&, const QString& );
    };
}


#endif
