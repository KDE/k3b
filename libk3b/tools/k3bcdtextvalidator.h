/*
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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
