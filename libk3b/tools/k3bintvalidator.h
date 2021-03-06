/*
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/


#ifndef _K3B_INT_VALIDATOR_H_
#define _K3B_INT_VALIDATOR_H_

#include "k3b_export.h"
#include <QValidator>
class QWidget;
class QString;

namespace K3b {
    /**
     * QValidator for integers.
     *
     * It differs from QIntValidator and KIntValidator in the fact that
     * it also accepts hex numbers prefixed with 0x.
     */
    class LIBK3B_EXPORT IntValidator : public QValidator
    {
    public:
        /**
         * Constructor.  Also sets the base value.
         */
        explicit IntValidator ( QWidget * parent );

        /**
         * Constructor.  Also sets the minimum, maximum, and numeric base values.
         */
        IntValidator ( int bottom, int top, QWidget * parent );

        /**
         * Destructs the validator.
         */
        ~IntValidator () override;

        /**
         * Validates the text, and return the result.  Does not modify the parameters.
         */
        State validate ( QString &, int & ) const override;

        /**
         * Fixes the text if possible, providing a valid string.  The parameter may be modified.
         */
        void fixup ( QString & ) const override;

        /**
         * Sets the minimum and maximum values allowed.
         */
        virtual void setRange ( int bottom, int top );

        /**
         * Returns the current minimum value allowed.
         */
        virtual int bottom () const;

        /**
         * Returns the current maximum value allowed.
         */
        virtual int top () const;

        /**
         * If the string starts with 0x it's assumed to be a hex number.
         */
        static int toInt( const QString&, bool* ok = 0 );

    private:
        int m_min;
        int m_max;
    };
}

#endif
