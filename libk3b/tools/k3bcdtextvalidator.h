/*

    SPDX-FileCopyrightText: 2003 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/


#ifndef _K3BCDTEXTVALIDATOR_H_
#define _K3BCDTEXTVALIDATOR_H_


#include "k3bvalidators.h"
#include "k3b_export.h"

namespace K3b {
    class LIBK3B_EXPORT CdTextValidator : public Latin1Validator
    {
    public:
        explicit CdTextValidator(QObject *parent = 0);
        ~CdTextValidator() override;

        State validate( QString& input, int& pos ) const override;
    };
}

#endif
