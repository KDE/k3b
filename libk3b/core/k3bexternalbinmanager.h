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

#ifndef K3B_EXTERNAL_BIN_MANAGER_H
#define K3B_EXTERNAL_BIN_MANAGER_H

#include "k3b_export.h"
#include "k3bversion.h"

#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>

class KConfigGroup;


namespace K3b {
    class ExternalProgram;

    /**
     * A ExternalBin represents an installed version of a program.
     * All ExternalBin objects are managed by ExternalPrograms.
     *
     * A bin may have certain features that are represented by a string.
     */
    class LIBK3B_EXPORT ExternalBin
    {
    public:
        ExternalBin( ExternalProgram& program, const QString& path );
        virtual ~ExternalBin();

        void setNeedGroup( const QString& name );
        const QString& needGroup() const;

        void setVersion( const Version& version );
        const Version& version() const;

        void setCopyright( const QString& copyright );
        const QString& copyright() const;

        const QString& path() const;
        QString name() const;
        bool isEmpty() const;
        QStringList userParameters() const;
        QStringList features() const;

        bool hasFeature( const QString& ) const;
        void addFeature( const QString& );

        ExternalProgram& program() const;

    private:
        class Private;
        Private* const d;
    };


    /**
     * This is the main class that represents a program
     * It's scan method has to be reimplemented for every program
     * It manages a list of ExternalBin-objects that each represent
     * one installed version of the program.
     */
    class LIBK3B_EXPORT ExternalProgram
    {
    public:
        explicit ExternalProgram( const QString& name );
        virtual ~ExternalProgram();

        const ExternalBin* defaultBin() const;
        const ExternalBin* mostRecentBin() const;

        void addUserParameter( const QString& );
        void setUserParameters( const QStringList& list );

        QStringList userParameters() const;
        QString name() const;

        void addBin( ExternalBin* );
        void clear();
        void setDefault( const ExternalBin* );
        void setDefault( const QString& path );

        QList<const ExternalBin*> bins() const;

        /**
         * this scans for the program in the given path,
         * adds the found bin object to the list and returns true.
         * if nothing could be found false is returned.
         */
        virtual bool scan( const QString& ) = 0;

        /**
         * reimplement this if it does not make sense to have the user be able
         * to specify additional parameters
         */
        virtual bool supportsUserParameters() const;

        /**
         * Builds the path to the program from the \p dir and the \p programName.
         * On Windows the .exe extension is added automatically.
         */
        static QString buildProgramPath( const QString& dir, const QString& programName );

    private:
        class Private;
        Private* const d;
    };


    /**
     * Simple implementation of the ExternalProgram scan functionality based on calling the
     * program twice: once for the version and once for the features.
     */
    class LIBK3B_EXPORT SimpleExternalProgram : public ExternalProgram
    {
    public:
        explicit SimpleExternalProgram( const QString& name );
        ~SimpleExternalProgram() override;

        bool scan( const QString& path ) override;

        /**
         * Parses a version starting at \p pos by looking for the first digit
         * followed by the first space char.
         */
        static Version parseVersionAt( const QString& data, int pos );

    protected:
        /**
         * Build the program's path. The default implementation simply calls
         * buildProgramPath on the program's name and the given path.
         */
        virtual QString getProgramPath( const QString& dir ) const;

        /**
         * Scan the version. The default implementation calls the program with
         * parameter --version and then calls parseVersion and parseCopyright.
         */
        virtual bool scanVersion( ExternalBin& bin ) const;

        /**
         * Scan for features. The default implementation checks for suidroot and
         * calls the program with parameter --help and then calls parseFeatures.
         */
        virtual bool scanFeatures( ExternalBin& bin ) const;

        /**
         * Determine the version from the program's version output.
         * The default implementation searches for the program's name
         * and parses the version from there.
         */
        virtual Version parseVersion( const QString& output, const ExternalBin& bin ) const;

        /**
         * Determine the copyright statement from the program's version output.
         * The default implementation looks for "(C)" and uses the rest of the line
         * from there.
         */
        virtual QString parseCopyright( const QString& output, const ExternalBin& bin ) const;

        /**
         * Parse the features from the \p output of --help and add them to the \p bin.
         * The default implementation does nothing.
         */
        virtual void parseFeatures( const QString& output, ExternalBin& bin ) const;

        /**
         * @return the string preceding the actual version string, used by
         *         parseVersion() method. Default implementation returns name().
         */
        virtual QString versionIdentifier( const ExternalBin& bin ) const;

    private:
        class Private;
        Private* const d;
    };


    class LIBK3B_EXPORT ExternalBinManager : public QObject
    {
        Q_OBJECT

    public:
        explicit ExternalBinManager( QObject* parent = 0 );
        ~ExternalBinManager() override;

        void search();

        /**
         * read config and add changes to current map.
         * Takes care of setting the config group
         */
        bool readConfig( const KConfigGroup& );

        /**
         * Takes care of setting the config group
         */
        bool saveConfig( KConfigGroup );

        bool foundBin( const QString& name );
        QString binPath( const QString& name );
        const ExternalBin* binObject( const QString& name );
        QString binNeedGroup( const QString& name );
        const ExternalBin* mostRecentBinObject( const QString& name );

        ExternalProgram* program( const QString& ) const;
        QMap<QString, ExternalProgram*> programs() const;

        /** always extends the default searchpath */
        void setSearchPath( const QStringList& );
        void addSearchPath( const QString& );
        void loadDefaultSearchPath();

        QStringList searchPath() const;

        void addProgram( ExternalProgram* );
        void clear();

    private:
        class Private;
        Private* const d;
    };
}

#endif
