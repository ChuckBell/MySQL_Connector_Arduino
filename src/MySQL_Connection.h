/*
  Copyright (c) 2012, 2016 Oracle and/or its affiliates. All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA

  MySQL_Connection.h - Library for communicating with a MySQL Server over
                       Ethernet. (formerly mysql.h)

  This header file defines a connection class for connecting to a MySQL
  server.

  Change History:

  Version 1.0.0a Created by Dr. Charles A. Bell, April 2012.
  Version 1.0.0b Updated by Dr. Charles A. Bell, October 2013.
  Version 1.0.1b Updated by Dr. Charles A. Bell, February 2014.
  Version 1.0.2b Updated by Dr. Charles A. Bell, April 2014.
  Version 1.0.3rc Updated by Dr. Charles A. Bell, March 2015.
  Version 1.0.4ga Updated by Dr. Charles A. Bell, July 2015.
  Version 1.1.0a Created by Dr. Charles A. Bell, January 2016.
  Version 1.1.1a Created by Dr. Charles A. Bell, January 2016.
*/
#ifndef MYSQL_CONNECTION_H
#define MYSQL_CONNECTION_H

#include <MySQL_Packet.h>

class MySQL_Connection : public MySQL_Packet {
  public:
    MySQL_Connection(Client *client_instance) :
        MySQL_Packet(client_instance) {}
    boolean connect(IPAddress server, int port, char *user, char *password);
    int connected() { return client->connected(); }
    const char *version() { return MYSQL_VERSION_STR; }
    void close();
};

#endif
