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

#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>
#include "k3b_export.h"
#include "k3bversion.h"

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
        ExternalProgram( const QString& name );
        virtual ~ExternalProgram();

        const ExternalBin* defaultBin() const;
        const ExternalBin* mostRecentBin() const;

        void addUserParameter( const QString& );
        void setUserParameters( const QStringList& list ) { m_userParameters = list; }

        QStringList userParameters() const { return m_userParameters; }
        QString name() const { return m_name; }

        void addBin( ExternalBin* );
        void clear() { m_bins.clear(); }
        void setDefault( const ExternalBin* );
        void setDefault( const QString& path );

        QList<const ExternalBin*> bins() const { return m_bins; }

        /**
         * this scans for the program in the given path,
         * adds the found bin object to the list and returnes true.
         * if nothing could be found false is returned.
         */
        virtual bool scan( const QString& ) {return false;}//= 0;

        /**
         * reimplement this if it does not make sense to have the user be able
         * to specify additional parameters
         */
        virtual bool supportsUserParameters() const { return true; }

        /**
         * Builds the path to the program from the \p dir and the \p programName.
         * On Windows the .exe extension is added autoamtically.
         */
        static QString buildProgramPath( const QString& dir, const QString& programName );

    private:
        QString m_name;
        QStringList m_userParameters;
        QList<const ExternalBin*> m_bins;
        QString m_defaultBin;
    };


    /**
     * Simple implementation of the ExternalProgram scan functionality based on calling the
     * program twice: once for the version and once for the features.
     */
    class LIBK3B_EXPORT SimpleExternalProgram : public ExternalProgram
    {
    public:
        SimpleExternalProgram( const QString& name );
        virtual ~SimpleExternalProgram();

        virtual bool scan( const QString& path );

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
         * @return the string preceeding the actual version string, used by
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
        ExternalBinManager( QObject* parent = 0 );
        ~ExternalBinManager();

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
        const ExternalBin* mostRecentBinObject( const QString& name );

        ExternalProgram* program( const QString& ) const;
        QMap<QString, ExternalProgram*> programs() const { return m_programs; }

        /** always extends the default searchpath */
        void setSearchPath( const QStringList& );
        void addSearchPath( const QString& );
        void loadDefaultSearchPath();

        QStringList searchPath() const { return m_searchPath; }

        void addProgram( ExternalProgram* );
        void clear();

    private:
        QMap<QString, ExternalProgram*> m_programs;
        QStringList m_searchPath;

        static QString m_noPath;  // used for binPath() to return const string

        QString m_gatheredOutput;
    };
}

#endif
