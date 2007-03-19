/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_DATA_PREPARATION_JOB_H_
#define _K3B_DATA_PREPARATION_JOB_H_

#include <k3bjob.h>


class K3bDataDoc;
class K3bJobHandler;

/**
 * The K3bDataPreparationJob performs some checks on the data in a data project
 * It is used by th K3bIsoImager.
 */
class K3bDataPreparationJob : public K3bJob
{
  Q_OBJECT

 public:
  K3bDataPreparationJob( K3bDataDoc* doc, K3bJobHandler* hdl, QObject* parent );
  ~K3bDataPreparationJob();

  bool hasBeenCanceled() const;

 public slots:
  void start();
  void cancel();

 private slots:
  void slotWorkDone( bool success );

 private:
  class Private;
  Private* d;
};

#endif
