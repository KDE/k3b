/****************************************************************************
** K3bAudioModule meta object code from reading C++ file 'k3baudiomodule.h'
**
** Created: Sat Oct 20 13:45:13 2001
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#if !defined(Q_MOC_OUTPUT_REVISION)
#define Q_MOC_OUTPUT_REVISION 9
#elif Q_MOC_OUTPUT_REVISION != 9
#error "Moc format conflict - please regenerate all moc files"
#endif

#include "k3baudiomodule.h"
#include <qmetaobject.h>
#include <qapplication.h>



const char *K3bAudioModule::className() const
{
    return "K3bAudioModule";
}

QMetaObject *K3bAudioModule::metaObj = 0;

void K3bAudioModule::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(QObject::className(), "QObject") != 0 )
	badSuperclassWarning("K3bAudioModule","QObject");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString K3bAudioModule::tr(const char* s)
{
    return qApp->translate( "K3bAudioModule", s, 0 );
}

QString K3bAudioModule::tr(const char* s, const char * c)
{
    return qApp->translate( "K3bAudioModule", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* K3bAudioModule::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) QObject::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    QMetaData::Access *slot_tbl_access = 0;
    typedef void (K3bAudioModule::*m2_t0)(char*,int);
    typedef void (QObject::*om2_t0)(char*,int);
    typedef void (K3bAudioModule::*m2_t1)(int);
    typedef void (QObject::*om2_t1)(int);
    typedef void (K3bAudioModule::*m2_t2)();
    typedef void (QObject::*om2_t2)();
    m2_t0 v2_0 = &K3bAudioModule::output;
    om2_t0 ov2_0 = (om2_t0)v2_0;
    m2_t1 v2_1 = &K3bAudioModule::percent;
    om2_t1 ov2_1 = (om2_t1)v2_1;
    m2_t2 v2_2 = &K3bAudioModule::finished;
    om2_t2 ov2_2 = (om2_t2)v2_2;
    QMetaData *signal_tbl = QMetaObject::new_metadata(3);
    signal_tbl[0].name = "output(char*,int)";
    signal_tbl[0].ptr = (QMember)ov2_0;
    signal_tbl[1].name = "percent(int)";
    signal_tbl[1].ptr = (QMember)ov2_1;
    signal_tbl[2].name = "finished()";
    signal_tbl[2].ptr = (QMember)ov2_2;
    metaObj = QMetaObject::new_metaobject(
	"K3bAudioModule", "QObject",
	0, 0,
	signal_tbl, 3,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    metaObj->set_slot_access( slot_tbl_access );
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    return metaObj;
}

#include <qobjectdefs.h>
#include <qsignalslotimp.h>

// SIGNAL output
void K3bAudioModule::output( char* t0, int t1 )
{
    // No builtin function for signal parameter type char*,int
    QConnectionList *clist = receivers("output(char*,int)");
    if ( !clist || signalsBlocked() )
	return;
    typedef void (QObject::*RT0)();
    typedef void (QObject::*RT1)(char*);
    typedef void (QObject::*RT2)(char*,int);
    RT0 r0;
    RT1 r1;
    RT2 r2;
    QConnectionListIt it(*clist);
    QConnection   *c;
    QSenderObject *object;
    while ( (c=it.current()) ) {
	++it;
	object = (QSenderObject*)c->object();
	object->setSender( this );
	switch ( c->numArgs() ) {
	    case 0:
#ifdef Q_FP_CCAST_BROKEN
		r0 = reinterpret_cast<RT0>(*(c->member()));
#else
		r0 = (RT0)*(c->member());
#endif
		(object->*r0)();
		break;
	    case 1:
#ifdef Q_FP_CCAST_BROKEN
		r1 = reinterpret_cast<RT1>(*(c->member()));
#else
		r1 = (RT1)*(c->member());
#endif
		(object->*r1)(t0);
		break;
	    case 2:
#ifdef Q_FP_CCAST_BROKEN
		r2 = reinterpret_cast<RT2>(*(c->member()));
#else
		r2 = (RT2)*(c->member());
#endif
		(object->*r2)(t0, t1);
		break;
	}
    }
}

// SIGNAL percent
void K3bAudioModule::percent( int t0 )
{
    activate_signal( "percent(int)", t0 );
}

// SIGNAL finished
void K3bAudioModule::finished()
{
    activate_signal( "finished()" );
}
