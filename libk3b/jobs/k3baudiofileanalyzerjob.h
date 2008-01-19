/*
 *
 * Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_AUDIO_FILE_ANALYZER_JOB_H_
#define _K3B_AUDIO_FILE_ANALYZER_JOB_H_

#include "k3bthreadjob.h"
#include "k3b_export.h"

class K3bAudioDecoder;

/**
 * A simple convinience class that runs K3bAudioDecoder::analyseFile
 * in a different thread.
 */
class LIBK3B_EXPORT K3bAudioFileAnalyzerJob : public K3bThreadJob
{
    Q_OBJECT

public:
    K3bAudioFileAnalyzerJob( K3bJobHandler* hdl, QObject* parent );
    ~K3bAudioFileAnalyzerJob();

    /**
     * Set the decoder that does the analyzation.
     */
    void setDecoder( K3bAudioDecoder* decoder );
    K3bAudioDecoder* decoder() const;

private:
    bool run();

    class Private;
    Private* const d;
};

#endif
