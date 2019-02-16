/*
 *
 * Copyright (C) 2005-2009 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_THREAD_WIDGET_H_
#define _K3B_THREAD_WIDGET_H_

#include <QObject>
#include <QHash>

namespace K3b {
    namespace Device {
        class Device;
    }

    /**
     * This class allows a thread other than the GUI thread to perform simple GUI
     * operations. Mainly creating some simple K3b Dialogs like Device selection.
     *
     * Since the calling thread cannot create the ThreadWidget by himself there exists
     * exactly one instance created by Core which is used by all threads.
     */
    class ThreadWidget : public QObject
    {
        Q_OBJECT

    public:
        ~ThreadWidget() override;

        static ThreadWidget* instance();

        /**
         * Call this from a thread to show a device selection dialog.
         */
        static Device::Device* selectDevice( QWidget* parent,
                                                  const QString& text = QString() );

    protected:
        /**
         * communication between the threads
         */
        void customEvent( QEvent* ) override;

    private:
        /**
         * used internally
         */
        class DeviceSelectionEvent;
        class Data;

        ThreadWidget();

        /**
         * Get unique id
         */
        int getNewId();
        void clearId( int id );
        Data* data( int id );

        int m_idCounter;
        QHash<int, Data*> m_dataMap;

        static ThreadWidget* s_instance;
    };
}

#endif
