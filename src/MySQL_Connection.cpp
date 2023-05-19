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

  MySQL_Connection.cpp - Library for communicating with a MySQL Server over
                         Ethernet. (formerly mysql.cpp)

  Change History:

  Version 1.0.0a Created by Dr. Charles A. Bell, April 2012.
  Version 1.0.0b Updated by Dr. Charles A. Bell, October 2013.
  Version 1.0.1b Updated by Dr. Charles A. Bell, February 2014.
  Version 1.0.2b Updated by Dr. Charles A. Bell, April 2014.
  Version 1.0.3rc Updated by Dr. Charles A. Bell, March 2015.
  Version 1.0.4ga Updated by Dr. Charles A. Bell, July 2015.
  Version 1.1.0a Created by Dr. Charles A. Bell, January 2016.
  Version 1.1.1a Created by Dr. Charles A. Bell, January 2016.
  Version 1.1.2b Created by Dr. Charles A. Bell, November 2016.
  Version 1.2.0 Created by Dr. Charles A. Bell, March 2020.
*/
#include <Arduino.h>
#include <MySQL_Connection.h>
#include <MySQL_Encrypt_Sha1.h>

#define MAX_CONNECT_ATTEMPTS 3
#define CONNECT_DELAY_MS     500
#define SUCCESS              1

const char CONNECTED[] PROGMEM = "Connected to server version ";
const char DISCONNECTED[] PROGMEM = "Disconnected.";

/*
  connect - Connect to a MySQL server.

  This method is used to connect to a MySQL server. It will attempt to
  connect to the server as a client retrying up to MAX_CONNECT_ATTEMPTS.
  This permits the possibility of longer than normal network lag times
  for wireless networks. You can adjust MAX_CONNECT_ATTEMPTS to suit
  your environment.

  server[in]      IP address of the server as IPAddress type
  port[in]        port number of the server
  user[in]        user name
  password[in]    (optional) user password
  db[in]          (optional) default database

  Returns boolean - True = connection succeeded
*/
boolean MySQL_Connection::connect(IPAddress server, int port, const char *user,
                                  const char *password, const char *db)
{
  int connected = 0;
  int retries = MAX_CONNECT_ATTEMPTS;

  // Retry up to MAX_CONNECT_ATTEMPTS times.
  while (retries--)
  {
    Serial.println("...trying...");
    connected = client->connect(server, port);
    if (connected != SUCCESS) {
      Serial.print("...got: ");
      Serial.print(connected);
      Serial.println(" retrying...");
      delay(CONNECT_DELAY_MS);
    } else {
      break;
    }
  }

  if (connected != SUCCESS)
    return false;

  read_packet();
  parse_handshake_packet();
  send_authentication_packet(user, password, db);
  read_packet();
  if (get_packet_type() != MYSQL_OK_PACKET) {
    parse_error_packet();
    return false;
  }

  show_error(CONNECTED);

  Serial.println(server_version);

  free(server_version); // don't need it anymore
  return true;
}

/*
  close - cancel the connection

  This method closes the connection to the server and frees up any memory
  used in the buffer.
*/
void MySQL_Connection::close()
{
  if (connected())
  {
    client->flush();
    client->stop();
    show_error(DISCONNECTED, true);
  }
}
