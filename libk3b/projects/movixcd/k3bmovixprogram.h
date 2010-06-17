/*
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_MOVIX_PROGRAM_H_
#define _K3B_MOVIX_PROGRAM_H_

#include "k3bexternalbinmanager.h"
#include "k3b_export.h"

namespace K3b {
    class LIBK3B_EXPORT MovixBin : public ExternalBin
    {
    public:
        MovixBin( ExternalProgram& program, const QString& path )
            : ExternalBin( program, path ) {
        }

        const QString& movixDataDir() const { return m_movixPath; }

        const QStringList& supportedBootLabels() const { return m_supportedBootLabels; }
        QStringList supportedSubtitleFonts() const;
        QStringList supportedLanguages() const;
        QStringList supportedKbdLayouts() const;
        QStringList supportedBackgrounds() const;
        QStringList supportedCodecs() const;

        /*
         * Unused for eMovix versions 0.9.0 and above
         */
        const QStringList& movixFiles() const { return m_movixFiles; }

        /*
         * Unused for eMovix versions 0.9.0 and above
         */
        const QStringList& isolinuxFiles() const { return m_isolinuxFiles; }

        /**
         * returnes empty string if font was not found
         *
         * Unused for eMovix versions 0.9.0 and above
         */
        QString subtitleFontDir( const QString& font ) const;

        /**
         * returnes empty string if lang was not found
         *
         * Unused for eMovix versions 0.9.0 and above
         */
        QString languageDir( const QString& lang ) const;

        /**
         * Interface for the movix-conf --files interface for
         * versions >= 0.9.0
         */
        QStringList files( const QString& kbd = QString(),
                           const QString& font = QString(),
                           const QString& bg = QString(),
                           const QString& lang = QString(),
                           const QStringList& codecs = QStringList() ) const;

    private:
        QStringList supported( const QString& ) const;

        QString m_movixPath;
        QStringList m_movixFiles;
        QStringList m_isolinuxFiles;
        QStringList m_supportedBootLabels;
        QStringList m_supportedSubtitleFonts;
        QStringList m_supportedLanguages;

        friend class MovixProgram;
    };


    class LIBK3B_EXPORT MovixProgram : public ExternalProgram
    {
    public:
        MovixProgram();

        bool scan( const QString& );

        bool supportsUserParameters() const { return false; }

    private:
        bool scanNewEMovix( MovixBin* bin, const QString& );
        bool scanOldEMovix( MovixBin* bin, const QString& );
        QStringList determineSupportedBootLabels( const QString& ) const;
    };
}



#endif
