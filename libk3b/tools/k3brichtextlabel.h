/*
 *
 * Copyright (C) 2005 Waldo Bastian <bastian@kde.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef K3BRICHTEXTLABEL_H
#define K3BRICHTEXTLABEL_H

#include <qlabel.h>

#include "k3b_export.h"

namespace K3b {

    /**
     * @short A replacement for QLabel that supports richtext and proper layout management
     *
     * @author Waldo Bastian <bastian@kde.org>
     */
    class LIBK3B_EXPORT RichTextLabel : public QLabel {
        Q_OBJECT

    public:
        /**
         * Default constructor.
         */
        RichTextLabel( QWidget *parent );
        RichTextLabel( const QString &text, QWidget *parent );

        int defaultWidth() const { return m_defaultWidth; }
        void setDefaultWidth(int defaultWidth);

        virtual QSize minimumSizeHint() const;
        virtual QSize sizeHint() const;
        QSizePolicy sizePolicy() const;

    public Q_SLOTS:
        void setText( const QString & );

    protected:
        int m_defaultWidth;

    protected:
        virtual void virtual_hook( int id, void* data );
    private:
        class RichTextLabelPrivate;
        RichTextLabelPrivate *d;
    };
}

#endif // K3BRICHTEXTLABEL_H
