/****************************************************************************
** K3bWavModule meta object code from reading C++ file 'k3bwavmodule.h'
**
** Created: Sat Oct 20 15:21:07 2001
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#if !defined(Q_MOC_OUTPUT_REVISION)
#define Q_MOC_OUTPUT_REVISION 9
#elif Q_MOC_OUTPUT_REVISION != 9
#error "Moc format conflict - please regenerate all moc files"
#endif

#include "k3bwavmodule.h"
#include <qmetaobject.h>
#include <qapplication.h>



const char *K3bWavModule::className() const
{
    return "K3bWavModule";
}

QMetaObject *K3bWavModule::metaObj = 0;

void K3bWavModule::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(K3bAudioModule::className(), "K3bAudioModule") != 0 )
	badSuperclassWarning("K3bWavModule","K3bAudioModule");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString K3bWavModule::tr(const char* s)
{
    return qApp->translate( "K3bWavModule", s, 0 );
}

QString K3bWavModule::tr(const char* s, const char * c)
{
    return qApp->translate( "K3bWavModule", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* K3bWavModule::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) K3bAudioModule::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    typedef void (K3bWavModule::*m1_t0)(KProcess*,char*,int);
    typedef void (QObject::*om1_t0)(KProcess*,char*,int);
    typedef void (K3bWavModule::*m1_t1)(KProcess*,char*,int);
    typedef void (QObject::*om1_t1)(KProcess*,char*,int);
    typedef void (K3bWavModule::*m1_t2)();
    typedef void (QObject::*om1_t2)();
    typedef void (K3bWavModule::*m1_t3)(char*,int);
    typedef void (QObject::*om1_t3)(char*,int);
    typedef void (K3bWavModule::*m1_t4)();
    typedef void (QObject::*om1_t4)();
    m1_t0 v1_0 = &K3bWavModule::slotParseStdErrOutput;
    om1_t0 ov1_0 = (om1_t0)v1_0;
    m1_t1 v1_1 = &K3bWavModule::slotOutputData;
    om1_t1 ov1_1 = (om1_t1)v1_1;
    m1_t2 v1_2 = &K3bWavModule::slotConvertingFinished;
    om1_t2 ov1_2 = (om1_t2)v1_2;
    m1_t3 v1_3 = &K3bWavModule::slotTestCountOutput;
    om1_t3 ov1_3 = (om1_t3)v1_3;
    m1_t4 v1_4 = &K3bWavModule::slotTestOutputFinished;
    om1_t4 ov1_4 = (om1_t4)v1_4;
    QMetaData *slot_tbl = QMetaObject::new_metadata(5);
    QMetaData::Access *slot_tbl_access = QMetaObject::new_metaaccess(5);
    slot_tbl[0].name = "slotParseStdErrOutput(KProcess*,char*,int)";
    slot_tbl[0].ptr = (QMember)ov1_0;
    slot_tbl_access[0] = QMetaData::Private;
    slot_tbl[1].name = "slotOutputData(KProcess*,char*,int)";
    slot_tbl[1].ptr = (QMember)ov1_1;
    slot_tbl_access[1] = QMetaData::Private;
    slot_tbl[2].name = "slotConvertingFinished()";
    slot_tbl[2].ptr = (QMember)ov1_2;
    slot_tbl_access[2] = QMetaData::Private;
    slot_tbl[3].name = "slotTestCountOutput(char*,int)";
    slot_tbl[3].ptr = (QMember)ov1_3;
    slot_tbl_access[3] = QMetaData::Private;
    slot_tbl[4].name = "slotTestOutputFinished()";
    slot_tbl[4].ptr = (QMember)ov1_4;
    slot_tbl_access[4] = QMetaData::Private;
    metaObj = QMetaObject::new_metaobject(
	"K3bWavModule", "K3bAudioModule",
	slot_tbl, 5,
	0, 0,
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
