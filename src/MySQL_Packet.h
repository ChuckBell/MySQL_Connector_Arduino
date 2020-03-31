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

  MySQL_Packet.h - Packet library for communicating with a MySQL Server

  This header file defines the base packet handling code for connecting
  to a MySQL server and executing queries.

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
#ifndef MYSQL_PACKET_H
#define MYSQL_PACKET_H

#ifdef ARDUINO_ARCH_ESP32 
    #include <Arduino.h> 
#elif ARDUINO_ARCH_ESP8266
    #include <ESP8266WiFi.h>
#else
//    #include <WiFi.h>
    #include <Ethernet.h>
#endif

#define MYSQL_OK_PACKET     0x00
#define MYSQL_EOF_PACKET    0xfe
#define MYSQL_ERROR_PACKET  0xff
#define MYSQL_VERSION_STR   "1.2.0"
#define DEBUG

const char MEMORY_ERROR[] PROGMEM = "Memory error.";
const char PACKET_ERROR[] PROGMEM = "Packet error.";
const char READ_TIMEOUT[] PROGMEM = "ERROR: Timeout waiting for client.";

class MySQL_Packet {
  public:
    byte *buffer;           // buffer for reading packets
    int packet_len;         // length of current packet
    Client *client;         // instance of client class (e.g. EthernetClient)
    char *server_version;   // save server version from handshake

    MySQL_Packet(Client *client_instance);
    boolean complete_handshake(char *user, char *password);
    void send_authentication_packet(char *user, char *password,
                                    char *db=NULL);
    void parse_handshake_packet();
    boolean scramble_password(char *password, byte *pwd_hash);
    void read_packet();
    int get_packet_type();
    void parse_error_packet();
    int get_lcb_len(int offset);
    int read_int(int offset, int size=0);
    void store_int(byte *buff, long value, int size);
    int read_lcb_int(int offset);
    int wait_for_bytes(int bytes_count);
    void show_error(const char *msg, bool EOL = false);
    void print_packet();

  private:
    byte seed[20];
};

#endif
