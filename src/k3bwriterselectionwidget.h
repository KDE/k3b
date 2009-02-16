/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
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


#ifndef K3BWRITERSELECTIONWIDGET_H
#define K3BWRITERSELECTIONWIDGET_H

#include <qwidget.h>
#include <QLabel>

#include <kconfiggroup.h>

#include <k3bglobals.h>

class K3bIntMapComboBox;
class QLabel;
class K3bMediaSelectionComboBox;
namespace K3bDevice {
    class Device;
}
namespace K3b {
    class Msf;
}


/**
 *@author Sebastian Trueg
 */
class K3bWriterSelectionWidget : public QWidget
{
    Q_OBJECT

public: 
    /**
     * Creates a writerselectionwidget
     */
    K3bWriterSelectionWidget( QWidget* parent = 0 );
    ~K3bWriterSelectionWidget();

    int writerSpeed() const;
    K3bDevice::Device* writerDevice() const;

    QList<K3bDevice::Device*> allDevices() const;

    /**
     * returns K3b::WritingApp
     */
    K3b::WritingApp writingApp() const;

    int wantedMediumType() const;
    int wantedMediumState() const;
    K3b::Msf wantedMediumSize() const;

    void loadDefaults();
    void loadConfig( const KConfigGroup& );
    void saveConfig( KConfigGroup& );

public Q_SLOTS:
    void setWriterDevice( K3bDevice::Device* );
    void setSpeed( int );
    void setWritingApp( K3b::WritingApp app );

    /**
     * K3b::WritingApp or'ed together
     *
     * Defaults to cdrecord and cdrdao for CD and growisofs for DVD
     */
    void setSupportedWritingApps( K3b::WritingApps apps );

    /**
     * A simple hack to disable the speed selection for DVD formatting
     */
    void setForceAutoSpeed( bool );

    /**
     * Set the wanted medium type. Defaults to writable CD.
     * 
     * \param type a bitwise combination of the K3bDevice::MediaType enum
     */
    void setWantedMediumType( int type );

    /**
     * Set the wanted medium state. Defaults to empty media.
     *
     * \param state a bitwise combination of the K3bDevice::State enum
     */
    void setWantedMediumState( int state );

    /**
     * Set the wanted medium size. Defaults to 0 which means
     * that the size should be irgnored.
     */
    void setWantedMediumSize( const K3b::Msf& minSize );

    /**
     * This is a hack to allow the copy dialogs to use the same device for reading
     * and writing without having the user to choose the same medium.
     *
     * \param overrideString A string which will be shown in place of the medium string.
     *                       For example: "Burn to the same device". Set it to 0 in order
     *                       to disable the feature.
     */
    void setOverrideDevice( K3bDevice::Device* dev, const QString& overrideString = QString(), const QString& tooltip = QString() );

    /**
     * Compare K3bMediaSelectionComboBox::setIgnoreDevice
     */
    void setIgnoreDevice( K3bDevice::Device* dev );

Q_SIGNALS:
    void writerChanged();
    void writerChanged( K3bDevice::Device* );
    void writingAppChanged( K3b::WritingApp app );

    /**
     * \see K3bMediaSelectionComboBox
     */
    void newMedia();
    void newMedium( K3bDevice::Device* dev );

private Q_SLOTS:
    void slotRefreshWriterSpeeds();
    void slotRefreshWritingApps();
    void slotWritingAppSelected( int id );
    void slotConfigChanged( KConfigBase* c );
    void slotSpeedChanged( int index );
    void slotWriterChanged();
    void slotNewBurnMedium( K3bDevice::Device* dev );
    void slotManualSpeed();

private:
    void clearSpeedCombo();
    void insertSpeedItem( int );
    K3b::WritingApp selectedWritingApp() const;

    class MediaSelectionComboBox;

    K3bIntMapComboBox* m_comboSpeed;
    MediaSelectionComboBox* m_comboMedium;
    K3bIntMapComboBox* m_comboWritingApp;
    QLabel* m_writingAppLabel;

    class Private;
    Private* d;
};

#endif
