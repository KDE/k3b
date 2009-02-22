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



#ifndef _K3B_DEFAULT_EXTERNAL_BIN_PROGRAMS_H_
#define _K3B_DEFAULT_EXTERNAL_BIN_PROGRAMS_H_

#include "k3bexternalbinmanager.h"
#include "k3b_export.h"

namespace K3b {
    class ExternalBinManager;

    LIBK3B_EXPORT void addDefaultPrograms( ExternalBinManager* );
    LIBK3B_EXPORT void addTranscodePrograms( ExternalBinManager* );
    LIBK3B_EXPORT void addVcdimagerPrograms( ExternalBinManager* );

    class LIBK3B_EXPORT CdrecordProgram : public ExternalProgram
    {
    public:
        CdrecordProgram( bool dvdPro );

        bool scan( const QString& );

    private:
        bool m_dvdPro;
    };


    class LIBK3B_EXPORT MkisofsProgram : public ExternalProgram
    {
    public:
        MkisofsProgram();

        bool scan( const QString& );
    };


    class LIBK3B_EXPORT ReadcdProgram : public ExternalProgram
    {
    public:
        ReadcdProgram();

        bool scan( const QString& );
    };


    class LIBK3B_EXPORT CdrdaoProgram : public ExternalProgram
    {
    public:
        CdrdaoProgram();

        bool scan( const QString& );
    };


    class LIBK3B_EXPORT TranscodeProgram : public ExternalProgram
    {
    public:
        TranscodeProgram( const QString& transcodeProgram );

        bool scan( const QString& );

        // no user parameters (yet)
        bool supportsUserParameters() const { return false; }

    private:
        QString m_transcodeProgram;
    };


    class LIBK3B_EXPORT VcdbuilderProgram : public ExternalProgram
    {
    public:
        VcdbuilderProgram( const QString& );

        bool scan( const QString& );

    private:
        QString m_vcdbuilderProgram;
    };


    class LIBK3B_EXPORT NormalizeProgram : public ExternalProgram
    {
    public:
        NormalizeProgram();

        bool scan( const QString& );
    };


    class LIBK3B_EXPORT GrowisofsProgram : public ExternalProgram
    {
    public:
        GrowisofsProgram();

        bool scan( const QString& );
    };


    class LIBK3B_EXPORT DvdformatProgram : public ExternalProgram
    {
    public:
        DvdformatProgram();

        bool scan( const QString& );
    };


    class LIBK3B_EXPORT DvdBooktypeProgram : public ExternalProgram
    {
    public:
        DvdBooktypeProgram();

        bool scan( const QString& );
    };


    class LIBK3B_EXPORT Cdda2wavProgram : public ExternalProgram
    {
    public:
        Cdda2wavProgram();

        bool scan( const QString& );
    };
}

#endif
