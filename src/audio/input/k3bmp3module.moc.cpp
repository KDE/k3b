/****************************************************************************
** K3bMp3Module meta object code from reading C++ file 'k3bmp3module.h'
**
** Created: Sat Oct 20 13:44:46 2001
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#if !defined(Q_MOC_OUTPUT_REVISION)
#define Q_MOC_OUTPUT_REVISION 9
#elif Q_MOC_OUTPUT_REVISION != 9
#error "Moc format conflict - please regenerate all moc files"
#endif

#include "k3bmp3module.h"
#include <qmetaobject.h>
#include <qapplication.h>



const char *K3bMp3Module::className() const
{
    return "K3bMp3Module";
}

QMetaObject *K3bMp3Module::metaObj = 0;

void K3bMp3Module::initMetaObject()
{
    if ( metaObj )
	return;
    if ( qstrcmp(K3bAudioModule::className(), "K3bAudioModule") != 0 )
	badSuperclassWarning("K3bMp3Module","K3bAudioModule");
    (void) staticMetaObject();
}

#ifndef QT_NO_TRANSLATION

QString K3bMp3Module::tr(const char* s)
{
    return qApp->translate( "K3bMp3Module", s, 0 );
}

QString K3bMp3Module::tr(const char* s, const char * c)
{
    return qApp->translate( "K3bMp3Module", s, c );
}

#endif // QT_NO_TRANSLATION

QMetaObject* K3bMp3Module::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void) K3bAudioModule::staticMetaObject();
#ifndef QT_NO_PROPERTIES
#endif // QT_NO_PROPERTIES
    typedef void (K3bMp3Module::*m1_t0)();
    typedef void (QObject::*om1_t0)();
    typedef void (K3bMp3Module::*m1_t1)();
    typedef void (QObject::*om1_t1)();
    typedef void (K3bMp3Module::*m1_t2)(KProcess*,char*,int);
    typedef void (QObject::*om1_t2)(KProcess*,char*,int);
    typedef void (K3bMp3Module::*m1_t3)();
    typedef void (QObject::*om1_t3)();
    typedef void (K3bMp3Module::*m1_t4)(KProcess*,char*,int);
    typedef void (QObject::*om1_t4)(KProcess*,char*,int);
    typedef void (K3bMp3Module::*m1_t5)();
    typedef void (QObject::*om1_t5)();
    typedef void (K3bMp3Module::*m1_t6)(KProcess*,char*,int);
    typedef void (QObject::*om1_t6)(KProcess*,char*,int);
    m1_t0 v1_0 = &K3bMp3Module::slotStartCountRawData;
    om1_t0 ov1_0 = (om1_t0)v1_0;
    m1_t1 v1_1 = &K3bMp3Module::slotGatherInformation;
    om1_t1 ov1_1 = (om1_t1)v1_1;
    m1_t2 v1_2 = &K3bMp3Module::slotCountRawData;
    om1_t2 ov1_2 = (om1_t2)v1_2;
    m1_t3 v1_3 = &K3bMp3Module::slotCountRawDataFinished;
    om1_t3 ov1_3 = (om1_t3)v1_3;
    m1_t4 v1_4 = &K3bMp3Module::slotParseStdErrOutput;
    om1_t4 ov1_4 = (om1_t4)v1_4;
    m1_t5 v1_5 = &K3bMp3Module::slotDecodingFinished;
    om1_t5 ov1_5 = (om1_t5)v1_5;
    m1_t6 v1_6 = &K3bMp3Module::slotOutputData;
    om1_t6 ov1_6 = (om1_t6)v1_6;
    QMetaData *slot_tbl = QMetaObject::new_metadata(7);
    QMetaData::Access *slot_tbl_access = QMetaObject::new_metaaccess(7);
    slot_tbl[0].name = "slotStartCountRawData()";
    slot_tbl[0].ptr = (QMember)ov1_0;
    slot_tbl_access[0] = QMetaData::Private;
    slot_tbl[1].name = "slotGatherInformation()";
    slot_tbl[1].ptr = (QMember)ov1_1;
    slot_tbl_access[1] = QMetaData::Private;
    slot_tbl[2].name = "slotCountRawData(KProcess*,char*,int)";
    slot_tbl[2].ptr = (QMember)ov1_2;
    slot_tbl_access[2] = QMetaData::Private;
    slot_tbl[3].name = "slotCountRawDataFinished()";
    slot_tbl[3].ptr = (QMember)ov1_3;
    slot_tbl_access[3] = QMetaData::Private;
    slot_tbl[4].name = "slotParseStdErrOutput(KProcess*,char*,int)";
    slot_tbl[4].ptr = (QMember)ov1_4;
    slot_tbl_access[4] = QMetaData::Private;
    slot_tbl[5].name = "slotDecodingFinished()";
    slot_tbl[5].ptr = (QMember)ov1_5;
    slot_tbl_access[5] = QMetaData::Private;
    slot_tbl[6].name = "slotOutputData(KProcess*,char*,int)";
    slot_tbl[6].ptr = (QMember)ov1_6;
    slot_tbl_access[6] = QMetaData::Private;
    metaObj = QMetaObject::new_metaobject(
	"K3bMp3Module", "K3bAudioModule",
	slot_tbl, 7,
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
