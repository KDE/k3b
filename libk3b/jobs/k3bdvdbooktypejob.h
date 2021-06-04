/*
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef _K3B_DVD_BOOKTYPE_JOB_H_
#define _K3B_DVD_BOOKTYPE_JOB_H_


#include "k3bjob.h"
#include <QProcess>


namespace K3b {
    namespace Device {
        class Device;
        class DeviceHandler;
    }


    /**
     * This job can change the compatibility bit of DVD+R(W) media
     * with supported dvd writers.
     */
    class DvdBooktypeJob : public Job
    {
        Q_OBJECT

    public:
        explicit DvdBooktypeJob( JobHandler*, QObject* parent = 0 );
        ~DvdBooktypeJob() override;

        QString jobDescription() const override;
        QString jobDetails() const override;

        /**
         * @list SET_MEDIA_DVD_ROM Change media identification on current media to DVD-ROM.
         * @list SET_MEDIA_DVD_R_W Change media identification on current media to DVD+R or DVD+RW.
         * @list SET_UNIT_DVD_ROM_ON_NEW_DVD_R Set the drive to write DVD-ROM specification on future written DVD+R discs.
         * @list SET_UNIT_DVD_ROM_ON_NEW_DVD_RW Set the drive to write DVD-ROM specification on future written DVD+RW discs.
         * @list SET_UNIT_DVD_R_ON_NEW_DVD_R Set the drive to write DVD+R specification on future written DVD+R discs.
         * @list SET_UNIT_DVD_RW_ON_NEW_DVD_RW Set the drive to write DVD+RW specification on future written DVD+RW discs.
         */
        enum Action {
            SET_MEDIA_DVD_ROM,
            SET_MEDIA_DVD_R_W,
            SET_UNIT_DVD_ROM_ON_NEW_DVD_R,
            SET_UNIT_DVD_ROM_ON_NEW_DVD_RW,
            SET_UNIT_DVD_R_ON_NEW_DVD_R,
            SET_UNIT_DVD_RW_ON_NEW_DVD_RW
        };

    public Q_SLOTS:
        void start() override;

        /**
         * The devicehandler needs to have a valid NgDiskInfo
         * Use this to prevent the job from searching a media.
         */
        void start( K3b::Device::DeviceHandler* );

        void cancel() override;

        void setDevice( K3b::Device::Device* );

        void setAction( int a ) { m_action = a; }

        /**
         * If set true the job ignores the global K3b setting
         * and does not eject the CD-RW after finishing
         */
        void setForceNoEject( bool );

    private Q_SLOTS:
        void slotStderrLine( const QString& );
        void slotProcessFinished( int, QProcess::ExitStatus );
        void slotDeviceHandlerFinished( Device::DeviceHandler* );
        void slotEjectingFinished( Device::DeviceHandler* );

    private:
        void startBooktypeChange();

        int m_action;

        class Private;
        Private* d;
    };
}


#endif
