/* 
 *
 * $Id$
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_SMART_INSTANCE_REUSER_H_
#define _K3B_SMART_INSTANCE_REUSER_H_

#include <dcopclient.h>

class KCmdLineArgs;
class DCOPRef;

class K3bSmartInstanceReuser : public DCOPClient
{
 public:
  ~K3bSmartInstanceReuser();

  static bool reuseInstance( KCmdLineArgs* );

 private:
  DCOPRef findInstance();
  void reuseInstance( DCOPRef& );
  K3bSmartInstanceReuser( KCmdLineArgs* );

  KCmdLineArgs* m_args;
};

#endif
