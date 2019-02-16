/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
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



#ifndef _K3B_DEFAULT_EXTERNAL_PROGRAMS_H_
#define _K3B_DEFAULT_EXTERNAL_PROGRAMS_H_

#include "k3bexternalbinmanager.h"
#include "k3b_export.h"

namespace K3b {
    class ExternalBinManager;

    LIBK3B_EXPORT void addDefaultPrograms( ExternalBinManager* );
    LIBK3B_EXPORT void addTranscodePrograms( ExternalBinManager* );
    LIBK3B_EXPORT void addVcdimagerPrograms( ExternalBinManager* );

    class LIBK3B_EXPORT AbstractCdrtoolsProgram : public SimpleExternalProgram
    {
    public:
        AbstractCdrtoolsProgram( const QString& program, const QString& cdrkitAlternative );
        ~AbstractCdrtoolsProgram() override;

    protected:
        bool usingCdrkit( const ExternalBin& bin ) const;
        QString getProgramPath( const QString& dir ) const override;
        QString versionIdentifier( const ExternalBin& bin ) const override;
        
    private:
        class Private;
        Private* d;
    };

    class LIBK3B_EXPORT CdrecordProgram : public AbstractCdrtoolsProgram
    {
    public:
        CdrecordProgram();

    protected:
        void parseFeatures( const QString& output, ExternalBin& bin ) const override;
    };


    class LIBK3B_EXPORT MkisofsProgram : public AbstractCdrtoolsProgram
    {
    public:
        MkisofsProgram();

    protected:
        void parseFeatures( const QString& output, ExternalBin& bin ) const override;
    };


    class LIBK3B_EXPORT ReadcdProgram : public AbstractCdrtoolsProgram
    {
    public:
        ReadcdProgram();

    protected:
        void parseFeatures( const QString& output, ExternalBin& bin ) const override;
    };


    class LIBK3B_EXPORT Cdda2wavProgram : public AbstractCdrtoolsProgram
    {
    public:
        Cdda2wavProgram();

    protected:
        void parseFeatures( const QString& output, ExternalBin& bin ) const override;
    };


    class LIBK3B_EXPORT CdrdaoProgram : public SimpleExternalProgram
    {
    public:
        CdrdaoProgram();

    protected:
        QString versionIdentifier( const ExternalBin& bin ) const override;
        bool scanFeatures( ExternalBin& bin ) const override;
    };


    class LIBK3B_EXPORT TranscodeProgram : public SimpleExternalProgram
    {
    public:
        explicit TranscodeProgram( const QString& transcodeProgram );

        // no user parameters (yet)
        bool supportsUserParameters() const override { return false; }

    protected:
        QString versionIdentifier( const ExternalBin& bin ) const override;
        bool scanFeatures( ExternalBin& bin ) const override;
    };


    class LIBK3B_EXPORT VcdbuilderProgram : public SimpleExternalProgram
    {
    public:
        explicit VcdbuilderProgram( const QString& );
        
    protected:
        QString versionIdentifier( const ExternalBin& bin ) const override;
    };


    class LIBK3B_EXPORT NormalizeProgram : public SimpleExternalProgram
    {
    public:
        NormalizeProgram();
    };


    class LIBK3B_EXPORT GrowisofsProgram : public SimpleExternalProgram
    {
    public:
        GrowisofsProgram();

    protected:
        bool scanFeatures( ExternalBin& bin ) const override;
    };


    class LIBK3B_EXPORT DvdformatProgram : public SimpleExternalProgram
    {
    public:
        DvdformatProgram();

    protected:
        Version parseVersion( const QString& output, const ExternalBin& bin ) const override;
        QString parseCopyright( const QString& output, const ExternalBin& bin ) const override;
    };


    class LIBK3B_EXPORT DvdBooktypeProgram : public SimpleExternalProgram
    {
    public:
        DvdBooktypeProgram();

    protected:
        Version parseVersion( const QString& output, const ExternalBin& bin ) const override;
        QString parseCopyright( const QString& output, const ExternalBin& bin ) const override;
    };

    class LIBK3B_EXPORT CdrskinProgram : public SimpleExternalProgram
    {
    public:
        CdrskinProgram();

    protected:
        bool scanFeatures(ExternalBin& bin) const override;
    };
}

#endif
