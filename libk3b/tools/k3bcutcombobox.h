/*
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_CUT_COMBOBOX_H_
#define _K3B_CUT_COMBOBOX_H_

#include "k3b_export.h"
#include <KComboBox>
#include <QPixmap>
#include <QResizeEvent>
class QResizeEvent;


namespace K3b {
    /**
     * Cuts it's text.
     * Since it rebuilds the complete list of strings every time
     * a new string is added or one gets removed it is not a good
     * idea to use this for dynamic lists.
     *
     * Be aware that currently only insertItem works.
     * none of the insertStrList or insertStringList methods are implemented
     * yet and also the removeItem methods does not work.
     */
    class LIBK3B_EXPORT CutComboBox : public KComboBox
    {
        Q_OBJECT

    public:
        explicit CutComboBox( QWidget* parent = 0 );
        explicit CutComboBox( int method, QWidget* parent = 0 );
        virtual ~CutComboBox();

        enum Method {
            CUT,
            SQUEEZE
        };

        /**
         * The method to shorten the text
         * default: CUT
         */
        void setMethod( int );

        /** reimplemented */
        QSize sizeHint() const;

        /** reimplemented */
        QSize minimumSizeHint() const;

        /** reimplemented */
        virtual void setCurrentText( const QString& );

        void	insertStringList( const QStringList &, int index=-1 );
        void	insertStrList( const char **, int numStrings=-1, int index=-1);

        void	insertItem( const QString &text, int index=-1 );
        void	insertItem( const QPixmap &pixmap, int index=-1 );
        void	insertItem( const QPixmap &pixmap, const QString &text, int index=-1 );

        void	removeItem( int index );

        void	changeItem( const QString &text, int index );
        void	changeItem( const QPixmap &pixmap, const QString &text, int index );

        QString text( int ) const;
        QString currentText() const;

        void clear();

    protected:
        void resizeEvent( QResizeEvent* e );
        void cutText();

    private:
        class Private;
        Private* d;
    };
}

#endif
