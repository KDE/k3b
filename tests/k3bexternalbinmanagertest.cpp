/*
    SPDX-FileCopyrightText: 2016-2017 Leslie Zhai <lesliezhai@llvm.org.cn>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QTest>

#include "k3bexternalbinmanagertest.h"
#include "k3bcore.h"
#include "k3bjob.h"
#include "k3bexternalbinmanager.h"
#include "k3bburnprogressdialog.h"
#include "k3bdefaultexternalprograms.h"

class MyBurnJob : public K3b::BurnJob 
{
    Q_OBJECT

public:
    MyBurnJob(K3b::JobHandler* hdl, QObject* parent = Q_NULLPTR)
        : K3b::BurnJob(hdl, parent) 
    {
        qDebug() << "DEBUG:" << __PRETTY_FUNCTION__;
        const K3b::ExternalBin* cdrecordBin = k3bcore->externalBinManager()->binObject("cdrecord");
        qDebug() << "DEBUG:" << __PRETTY_FUNCTION__ << cdrecordBin;
    }
    ~MyBurnJob() override
    {
        qDebug() << "DEBUG:" << __PRETTY_FUNCTION__;
    }

public Q_SLOTS:
    void start() override { jobStarted(); jobFinished(true); }
    void cancel() override { emit canceled(); }

private:
    bool prepareWriter() 
    {
        qDebug() << "DEBUG:" << __PRETTY_FUNCTION__;
        const K3b::ExternalBin* cdrecordBin = k3bcore->externalBinManager()->binObject("cdrecord");
        if (!cdrecordBin)
            qWarning() << "ERROR:" << __PRETTY_FUNCTION__ << cdrecordBin;
        return true;
    }
};

QTEST_GUILESS_MAIN(ExternalBinManagerTest)

ExternalBinManagerTest::ExternalBinManagerTest()
    : QObject()
    , m_core(new K3b::Application::Core(this))
{
}

void ExternalBinManagerTest::testBinObject()
{
    K3b::ExternalBinManager* binManager = new K3b::ExternalBinManager;
    K3b::addDefaultPrograms(binManager);
    binManager->search();
    qDebug() << "DEBUG:" << __PRETTY_FUNCTION__ << binManager->foundBin("cdrecord");
    if (binManager->binObject("ooo") && binManager->binObject("ooo")->hasFeature("fff")) {
        qDebug() << __PRETTY_FUNCTION__ << "it *NEVER* happened!";
    }
    // ooo binObject directly return 0
    // then hasFeature will segfault!
    // there are a lot of *unchecking* binObject is nullptr issue in k3b-2.0.3!!!
    //if (binManager->binObject("ooo")->hasFeature("fff")) {
    //}
}

void ExternalBinManagerTest::testMyBurnJob() 
{
    QSKIP("currently segfaulting");
    K3b::BurnProgressDialog* dlg = new K3b::BurnProgressDialog;
    MyBurnJob* job = new MyBurnJob(dlg, this);
    dlg->setJob(job);
    // TODO: it needs CdrskinWritter for job->setWritingApp
    dlg->startJob(job);
}

#include "k3bexternalbinmanagertest.moc"

#include "moc_k3bexternalbinmanagertest.cpp"
