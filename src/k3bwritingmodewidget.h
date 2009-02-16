/* 
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef _K3B_WRITING_MODE_WIDGET_H_
#define _K3B_WRITING_MODE_WIDGET_H_

#include <k3bintmapcombobox.h>
#include <k3bmedium.h>
#include "k3bglobals.h"

#include <kconfiggroup.h>


/**
 * Allows selection of K3b::WritingMode
 */
class K3bWritingModeWidget : public K3bIntMapComboBox
{
    Q_OBJECT

public:
    K3bWritingModeWidget( QWidget* parent = 0 );
    K3bWritingModeWidget( K3b::WritingModes modes, QWidget* parent = 0 );
    ~K3bWritingModeWidget();

    K3b::WritingMode writingMode() const;

    /**
     * \return All supported writing modes. The list
     * is optionally filtered according to the device
     * set via setDevice.
     */
    K3b::WritingModes supportedWritingModes() const;

    void saveConfig( KConfigGroup& );

    /**
     * This will not emit the writingModeChanged signal
     */
    void loadConfig( const KConfigGroup& );

public Q_SLOTS:
    /**
     * This will not emit the writingModeChanged signal
     */
    void setWritingMode( K3b::WritingMode m );
    void setSupportedModes( K3b::WritingModes modes );

    /**
     * If the device is set the supported writing modes
     * will be filtered by the ones supported by the drive.
     */
    void setDevice( K3bDevice::Device* );

    /**
     * Set the writing modes which make sense with the provided medium.
     * This will also reset the device from the medium.
     *
     * \param m The medium. May even be non-writable or no medium at all
     *          in which case only the auto mode will be selected.
     *
     * \sa setDevice
     */
    void determineSupportedModesFromMedium( const K3bMedium& m );

    /**
     * Convenience method. Does the same as the one above.
     *
     * \param dev The device which contains the medium. May even be 0 in
     *            which case only the auto mode will be selected.
     */
    void determineSupportedModesFromMedium( K3bDevice::Device* dev );

 Q_SIGNALS:
    void writingModeChanged( K3b::WritingMode );

private:
    void init();
    void updateModes();

    class Private;
    Private* d;

    Q_PRIVATE_SLOT( d, void _k_writingModeChanged(int) )
};

#endif
