/***************************************************************************
    copyright            : (C) 2004 by Scott Wheeler
    email                : wheeler@kde.org
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it  under the terms of the GNU Lesser General Public License version  *
 *   2.1 as published by the Free Software Foundation.                     *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 ***************************************************************************/

/*
 * At some point this will likely be a library class, as such it's been written
 * as such and is LGPL'ed.
 */

#ifndef KTRM_H
#define KTRM_H

#include <config.h>

#if HAVE_MUSICBRAINZ

#include <qstring.h>
#include <qvaluelist.h>
#include <qmap.h>

/**
 * This represents a potential match for a TRM lookup.  KTRMResultList is
 * returned from KTRMLookup and will be sorted by relevance (better matches
 * at the beginning of the list).
 */

class KTRMResult
{
    friend class KTRMLookup;

public:
    KTRMResult();
    KTRMResult(const KTRMResult &result);
    ~KTRMResult();

    /**
     * Returns the title of the track for the potential match.
     */
    QString title() const;

    /**
     * Returns the artist name of the track for the potential match.
     */
    QString artist() const;

    /**
     * Returns the album name of the track for the potential match.
     */
    QString album() const;

    /**
     * Returns the track number of the track for the potential match.
     */
    int track() const;

    /**
     * Returns the original release year of the track for the potential match.
     */
    int year() const;

    /**
     * Returns true if all of the values for the result are empty.
     */
    bool isEmpty() const;

    /**
     * Compares to \a r based on the relevance of the match.  Better matches
     * will be greater than less accurate matches.
     */
    bool operator<(const KTRMResult &r) const;

    /**
     * Compares to \a r based on the relevance of the match.  Better matches
     * will be greater than less accurate matches.
     */
    bool operator>(const KTRMResult &r) const;

private:
    class KTRMResultPrivate;
    KTRMResultPrivate *d;
};

typedef QValueList<KTRMResult> KTRMResultList;

/**
 * An abstraction for libtunepimp's TRM based lookup and file recognition.
 *
 * A lookup is started when the object is created.  One of the virtual methods
 * -- recognized(), unrecognized(), collision() or error().  Those methods
 * should be reimplemented in subclasses to specify what behavior should happen
 * for each result.
 *
 * The lookups themselves happen in a background thread, but the return calls
 * are guaranteed to run in the GUI thread.
 */
class KTRMLookup
{
public:
    /**
     * Creates and starts a lookup for \a file.  If \a autoDelete is set to
     * true the lookup will delete itself when it has finished.
     */
    KTRMLookup(const QString &file, bool autoDelete = false);

    virtual ~KTRMLookup();

    /**
     * Returns the file name that was specified in the constructor.
     */
    QString file() const;

    /**
     * Returns the TunePimp file ID for the file.  This is of no use to the
     * public API.
     * 
     * @internal
     */
    int fileId() const;

    /**
     * This method is called if the track was recognized by the TRM server.
     * results() will return just one value.  This may be reimplemented to
     * provide specific behavion in the case of the track being recognized.
     *
     * \note The base class call should happen at the beginning of the subclass
     * reimplementation; it populates the values of results().
     */
    virtual void recognized();

    /**
     * This method is called if the track was not recognized by the TRM server.
     * results() will return an empty set.  This may be reimplemented to provide
     * specific behavion in the case of the track not being recognized.
     */
    virtual void unrecognized();

    /**
     * This method is called if there are multiple potential matches for the TRM
     * value.  results() will return a list of the potential matches, sorted by
     * liklihood.  This may be reimplemented to provide
     * specific behavion in the case of the track not being recognized.
     *
     * \note The base class call should happen at the beginning of the subclass
     * reimplementation; it populates the values of results().
     */
    virtual void collision();

    /**
     * This method is called if the track was not recognized by the TRM server.
     * results() will return an empty set.  This may be reimplemented to provide
     * specific behavion in the case of the track not being recognized.
     */
    virtual void error();

    /**
     * Returns the list of matches found by the lookup.  In the case that there
     * was a TRM collision this list will contain multiple entries.  In the case
     * that it was recognized this will only contain one entry.  Otherwise it
     * will remain empty.
     */
    KTRMResultList results() const;

    /**
     * This method is called when any of terminal states (recognized,
     * unrecognized, collision or error) has been reached after the specifc
     * method for the result has been called.
     *
     * This should be reimplemented in the case that there is some general
     * processing to be done for all terminal states.
     */
    virtual void finished();

private:
    class KTRMLookupPrivate;
    KTRMLookupPrivate *d;
};

#endif
#endif
