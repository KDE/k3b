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

#include "ktrm.h"

#ifdef HAVE_TUNEPIMP

#include <kapplication.h>
#include <kprotocolmanager.h>
#include <kurl.h>
#include <kdebug.h>

#include <qmutex.h>
#include <qevent.h>
#include <qobject.h>
#include <qfile.h>

#include <tunepimp/tp_c.h>
#include <fixx11h.h>

class KTRMLookup;

extern "C"
{
    static void TRMNotifyCallback(tunepimp_t pimp, void *data, TPCallbackEnum type, int fileId);
}

/**
 * This represents the main TunePimp instance and handles incoming requests.
 */

class KTRMRequestHandler
{
public:
    static KTRMRequestHandler *instance()
    {
        static QMutex mutex;
        mutex.lock();
        static KTRMRequestHandler handler;
        mutex.unlock();
        return &handler;
    }

    int KTRMRequestHandler::startLookup(KTRMLookup *lookup)
    {
        int id;

        if(!m_fileMap.contains(lookup->file())) {
            id = tp_AddFile(m_pimp, QFile::encodeName(lookup->file()));
            m_fileMap.insert(lookup->file(), id);
        }
        else {
            id = m_fileMap[lookup->file()];
            tp_IdentifyAgain(m_pimp, id);
        }
        m_lookupMap[id] = lookup;
        return id;
    }

    void endLookup(KTRMLookup *lookup)
    {
        tp_ReleaseTrack(m_pimp, tp_GetTrack(m_pimp, lookup->fileId()));
        tp_Remove(m_pimp, lookup->fileId());
        m_lookupMap.remove(lookup->fileId());
    }

    bool lookupMapContains(int fileId) const
    {
        m_lookupMapMutex.lock();
        bool contains = m_lookupMap.contains(fileId);
        m_lookupMapMutex.unlock();
        return contains;
    }

    KTRMLookup *lookup(int fileId) const
    {
        m_lookupMapMutex.lock();
        KTRMLookup *l = m_lookupMap[fileId];
        m_lookupMapMutex.unlock();
        return l;
    }

    void removeFromLookupMap(int fileId)
    {
        m_lookupMapMutex.lock();
        m_lookupMap.remove(fileId);
        m_lookupMapMutex.unlock();
    }

    const tunepimp_t &tunePimp() const
    {
        return m_pimp;
    }

protected:
    KTRMRequestHandler()
    {
        m_pimp = tp_New("KTRM", "0.1");
        tp_SetTRMCollisionThreshold(m_pimp, 100);
        tp_SetAutoSaveThreshold(m_pimp, -1);
        tp_SetMoveFiles(m_pimp, false);
        tp_SetRenameFiles(m_pimp, false);
        tp_SetUseUTF8(m_pimp, true);
        tp_SetNotifyCallback(m_pimp, TRMNotifyCallback, 0);

        if(KProtocolManager::useProxy()) {
            KURL proxy = KProtocolManager::proxyFor("http");
            tp_SetProxy(m_pimp, proxy.host().latin1(), short(proxy.port()));
        }
    }

    ~KTRMRequestHandler()
    {
        tp_Delete(m_pimp);
    }

private:
    tunepimp_t m_pimp;
    QMap<int, KTRMLookup *> m_lookupMap;
    QMap<QString, int> m_fileMap;
    mutable QMutex m_lookupMapMutex;
};


/**
 * A custom event type used for signalling that a TRM lookup is finished.
 */

class KTRMEvent : public QCustomEvent
{
public:
    enum Status {
        Recognized,
        Unrecognized,
        Collision,
        Error
    };

    KTRMEvent(int fileId, Status status) :
        QCustomEvent(id),
        m_fileId(fileId),
        m_status(status) {}

    int fileId() const
    {
        return m_fileId;
    }

    Status status() const
    {
        return m_status;
    }

    static const int id = User + 1984; // random, unique, event id

private:
    int m_fileId;
    Status m_status;
};

/**
 * A helper class to intercept KTRMQueryEvents and call recognized() (from the GUI
 * thread) for the lookup.
 */

class KTRMEventHandler : public QObject
{
public:
    static void send(int fileId, KTRMEvent::Status status)
    {
        KApplication::postEvent(instance(), new KTRMEvent(fileId, status));
    }

protected:
    KTRMEventHandler() : QObject() {}

    static KTRMEventHandler *instance()
    {
        static QMutex mutex;
        mutex.lock();
        static KTRMEventHandler handler;
        mutex.unlock();
        return &handler;
    }

    virtual void customEvent(QCustomEvent *event)
    {
        if(!event->type() == KTRMEvent::id)
            return;

        KTRMEvent *e = static_cast<KTRMEvent *>(event);

        static QMutex mutex;
        mutex.lock();

        if(!KTRMRequestHandler::instance()->lookupMapContains(e->fileId())) {
            mutex.unlock();
            return;
        }

        KTRMLookup *lookup = KTRMRequestHandler::instance()->lookup(e->fileId());
        KTRMRequestHandler::instance()->removeFromLookupMap(e->fileId());

        mutex.unlock();

        switch(e->status()) {
        case KTRMEvent::Recognized:
            lookup->recognized();
            break;
        case KTRMEvent::Unrecognized:
            lookup->unrecognized();
            break;
        case KTRMEvent::Collision:
            lookup->collision();
            break;
        case KTRMEvent::Error:
            lookup->error();
            break;
        }
	lookup->finished();
    }
};

/**
 * Callback fuction for TunePimp lookup events.
 */

static void TRMNotifyCallback(tunepimp_t pimp, void *, TPCallbackEnum type, int fileId)
{
    if(type != tpFileChanged)
        return;

    track_t track = tp_GetTrack(pimp, fileId);
    TPFileStatus status = tr_GetStatus(track);

    switch(status) {
    case eRecognized:
        KTRMEventHandler::send(fileId, KTRMEvent::Recognized);
        break;
    case eUnrecognized:
        KTRMEventHandler::send(fileId, KTRMEvent::Unrecognized);
        break;
    case eTRMCollision:
        KTRMEventHandler::send(fileId, KTRMEvent::Collision);
        break;
    case eError:
        KTRMEventHandler::send(fileId, KTRMEvent::Error);
        break;
    default:
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////
// KTRMResult implementation
////////////////////////////////////////////////////////////////////////////////

class KTRMResult::KTRMResultPrivate
{
public:
    KTRMResultPrivate() : track(0), year(0), relevance(0) {}
    QString title;
    QString artist;
    QString album;
    int track;
    int year;
    int relevance;
};

////////////////////////////////////////////////////////////////////////////////
// KTRMResult public methods
////////////////////////////////////////////////////////////////////////////////

KTRMResult::KTRMResult()
{
    d = new KTRMResultPrivate;
}

KTRMResult::KTRMResult(const KTRMResult &result)
{
    d = new KTRMResultPrivate(*result.d);
}

KTRMResult::~KTRMResult()
{
    delete d;
}

QString KTRMResult::title() const
{
    return d->title;
}

QString KTRMResult::artist() const
{
    return d->artist;
}

QString KTRMResult::album() const
{
    return d->album;
}

int KTRMResult::track() const
{
    return d->track;
}

int KTRMResult::year() const
{
    return d->year;
}

bool KTRMResult::operator<(const KTRMResult &r) const
{
    return r.d->relevance < d->relevance;
}

bool KTRMResult::operator>(const KTRMResult &r) const
{
    return r.d->relevance > d->relevance;
}

bool KTRMResult::isEmpty() const
{
    return d->title.isEmpty() && d->artist.isEmpty() && d->album.isEmpty() &&
        d->track == 0 && d->year == 0;
}

////////////////////////////////////////////////////////////////////////////////
// KTRMLookup implementation
////////////////////////////////////////////////////////////////////////////////

class KTRMLookup::KTRMLookupPrivate
{
public:
    KTRMLookupPrivate() :
        fileId(-1) {}
    QString file;
    KTRMResultList results;
    int fileId;
    bool autoDelete;
};

////////////////////////////////////////////////////////////////////////////////
// KTRMLookup public methods
////////////////////////////////////////////////////////////////////////////////

KTRMLookup::KTRMLookup(const QString &file, bool autoDelete)
{
    d = new KTRMLookupPrivate;
    d->file = file;
    d->autoDelete = autoDelete;
    d->fileId = KTRMRequestHandler::instance()->startLookup(this);
}

KTRMLookup::~KTRMLookup()
{
    KTRMRequestHandler::instance()->endLookup(this);
    delete d;
}

QString KTRMLookup::file() const
{
    return d->file;
}

int KTRMLookup::fileId() const
{
    return d->fileId;
}

void KTRMLookup::recognized()
{
    kdDebug() << k_funcinfo << d->file << endl;

    d->results.clear();

    metadata_t *metaData = md_New();
    track_t track = tp_GetTrack(KTRMRequestHandler::instance()->tunePimp(), d->fileId);
    tr_GetServerMetadata(track, metaData);

    KTRMResult result;

    result.d->title = QString::fromUtf8(metaData->track);
    result.d->artist = QString::fromUtf8(metaData->artist);
    result.d->album = QString::fromUtf8(metaData->album);
    result.d->track = metaData->trackNum;
    result.d->year = metaData->releaseYear;

    d->results.append(result);

    md_Delete(metaData);
}

void KTRMLookup::unrecognized()
{
    kdDebug() << k_funcinfo << d->file << endl;
    d->results.clear();
}

void KTRMLookup::collision()
{
    kdDebug() << k_funcinfo << d->file << endl;

    track_t track = tp_GetTrack(KTRMRequestHandler::instance()->tunePimp(), d->fileId);

    if(track <= 0) {
        kdDebug() << "invalid track number" << endl;
        return;
    }

    tr_Lock(track);
    int resultCount = tr_GetNumResults(track);

    if(resultCount > 0) {
        TPResultType type;
        result_t *results = new result_t[resultCount];
        tr_GetResults(track, &type, results, &resultCount);

        switch(type) {
        case eNone:
            kdDebug() << k_funcinfo << "eNone" << endl;
            break;
        case eArtistList:
            kdDebug() << "eArtistList" << endl;
            break;
        case eAlbumList:
            kdDebug() << "eAlbumList" << endl;
            break;
        case eTrackList:
        {
            kdDebug() << "eTrackList" << endl;
            albumtrackresult_t **tracks = (albumtrackresult_t **) results;
            d->results.clear();

            for(int i = 0; i < resultCount; i++) {
                KTRMResult result;

                result.d->title = QString::fromUtf8(tracks[i]->name);
                result.d->artist = QString::fromUtf8(tracks[i]->artist->name);
                result.d->album = QString::fromUtf8(tracks[i]->album->name);
                result.d->track = tracks[i]->trackNum;
                result.d->year = tracks[i]->album->releaseYear;
                result.d->relevance = tracks[i]->relevance;

                d->results.append(result);
            }
            break;
        }
        case eMatchedTrack:
            kdDebug() << k_funcinfo << "eMatchedTrack" << endl;
            break;
        }

        delete [] results;
    }

    tr_Unlock(track);
}

void KTRMLookup::error()
{
    kdDebug() << k_funcinfo << d->file << endl;

    d->results.clear();
}

KTRMResultList KTRMLookup::results() const
{
    return d->results;
}

////////////////////////////////////////////////////////////////////////////////
// KTRMLookup protected methods
////////////////////////////////////////////////////////////////////////////////

void KTRMLookup::finished()
{
    if(d->autoDelete)
        delete this;
}

#endif
