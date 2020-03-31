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

  MySQL_Packet.cpp - Packet library for communicating with a MySQL Server

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
#include <MySQL_Packet.h>
#include <MySQL_Encrypt_Sha1.h>

#define MYSQL_DATA_TIMEOUT  3000   // Wifi client wait in milliseconds
#define MYSQL_WAIT_INTERVAL 300    // WiFi client wait interval

/*
  Constructor

  Initialize the buffer and store client instance.
*/
MySQL_Packet::MySQL_Packet(Client *client_instance) {
  buffer = NULL;
  client = client_instance;
}

/*
  show_error

  Print the curent error message.

  msg[in]    Message to print
  EOL[in]    True if we print EOLN character
*/
void MySQL_Packet::show_error(const char *msg, bool EOL) {
#ifdef DEBUG
  char pos;
  while ((pos = pgm_read_byte(msg))) {
    Serial.print(pos);
    msg++;
  }
  if (EOL)
    Serial.println();
#endif
}

/*
  send_authentication_packet

  This method builds a response packet used to respond to the server's
  challenge packet (called the handshake packet). It includes the user
  name and password scrambled using the SHA1 seed from the handshake
  packet. It also sets the character set (default is 8 which you can
  change to meet your needs).

  Note: you can also set the default database in this packet. See
        the code before for a comment on where this happens.

  The authentication packet is defined as follows.

  Bytes                        Name
  -----                        ----
  4                            client_flags
  4                            max_packet_size
  1                            charset_number
  23                           (filler) always 0x00...
  n (Null-Terminated String)   user
  n (Length Coded Binary)      scramble_buff (1 + x bytes)
  n (Null-Terminated String)   databasename (optional)

  user[in]        User name
  password[in]    password
  db[in]          default database
*/
void MySQL_Packet::send_authentication_packet(char *user, char *password,
                                              char *db)
{
  if (buffer != NULL)
    free(buffer);

  buffer = (byte *)malloc(256);

  int size_send = 4;

  // client flags
  buffer[size_send] = byte(0x0D);
  buffer[size_send+1] = byte(0xa6);
  buffer[size_send+2] = byte(0x03);
  buffer[size_send+3] = byte(0x00);
  size_send += 4;

  // max_allowed_packet
  buffer[size_send] = 0;
  buffer[size_send+1] = 0;
  buffer[size_send+2] = 0;
  buffer[size_send+3] = 1;
  size_send += 4;

  // charset - default is 8
  buffer[size_send] = byte(0x08);
  size_send += 1;
  for(int i = 0; i < 24; i++)
    buffer[size_send+i] = 0x00;
  size_send += 23;

  // user name
  memcpy((char *)&buffer[size_send], user, strlen(user));
  size_send += strlen(user) + 1;
  buffer[size_send-1] = 0x00;

  // password - see scramble password
  byte *scramble = (uint8_t *)malloc(20);
  if (scramble_password(password, scramble)) {
    buffer[size_send] = 0x14;
    size_send += 1;
    for (int i = 0; i < 20; i++)
      buffer[i+size_send] = scramble[i];
    size_send += 20;
    buffer[size_send] = 0x00;
  }
  free(scramble);

  if (db) {
    memcpy((char *)&buffer[size_send], db, strlen(db));
    size_send += strlen(db) + 1;
    buffer[size_send-1] = 0x00;
  } else {
    buffer[size_send+1] = 0x00;
    size_send += 1;
  }

  // Write packet size
  int p_size = size_send - 4;
  store_int(&buffer[0], p_size, 3);
  buffer[3] = byte(0x01);

  // Write the packet
  client->write((uint8_t*)buffer, size_send);
  client->flush();
}


/*
  scramble_password - Build a SHA1 scramble of the user password

  This method uses the password hash seed sent from the server to
  form a SHA1 hash of the password. This is used to send back to
  the server to complete the challenge and response step in the
  authentication handshake.

  password[in]    User's password in clear text
  pwd_hash[in]    Seed from the server

  Returns boolean - True = scramble succeeded
*/
boolean MySQL_Packet::scramble_password(char *password, byte *pwd_hash) {
  byte *digest;
  byte hash1[20];
  byte hash2[20];
  byte hash3[20];
  byte pwd_buffer[40];

  if (strlen(password) == 0)
    return false;

  // hash1
  Sha1.init();
  Sha1.print(password);
  digest = Sha1.result();
  memcpy(hash1, digest, 20);

  // hash2
  Sha1.init();
  Sha1.write(hash1, 20);
  digest = Sha1.result();
  memcpy(hash2, digest, 20);

  // hash3 of seed + hash2
  Sha1.init();
  memcpy(pwd_buffer, &seed, 20);
  memcpy(pwd_buffer+20, hash2, 20);
  Sha1.write(pwd_buffer, 40);
  digest = Sha1.result();
  memcpy(hash3, digest, 20);

  // XOR for hash4
  for (int i = 0; i < 20; i++)
    pwd_hash[i] = hash1[i] ^ hash3[i];

  return true;
}

/*
  wait_for_bytes - Wait until data is available for reading

  This method is used to permit the connector to respond to servers
  that have high latency or execute long queries. The timeout is
  set by MYSQL_DATA_TIMEOUT. Adjust this value to match the performance of
  your server and network.

  It is also used to read how many bytes in total are available from the
  server. Thus, it can be used to know how large a data burst is from
  the server.

  bytes_need[in]    Bytes count to wait for

  Returns integer - Number of bytes available to read.
*/
int MySQL_Packet::wait_for_bytes(int bytes_need)
{
  const long wait_till = millis() + MYSQL_DATA_TIMEOUT;
  int num = 0;
  long now = 0;

  do
  {
    now = millis();
    num = client->available();
    if (num < bytes_need)
      delay(MYSQL_WAIT_INTERVAL);
    else
      break;
  } while (now < wait_till);

  if (num == 0 && now >= wait_till)
    client->stop();

  return num;
}

/*
  read_packet - Read a packet from the server and store it in the buffer

  This method reads the bytes sent by the server as a packet. All packets
  have a packet header defined as follows.

  Bytes                 Name
  -----                 ----
  3                     Packet Length
  1                     Packet Number

  Thus, the length of the packet (not including the packet header) can
  be found by reading the first 4 bytes from the server then reading
  N bytes for the packet payload.
*/
void MySQL_Packet::read_packet() {
  byte local[4];

  if (buffer) {
    free(buffer);
    buffer = NULL;
  }

  // Read packet header
  if (wait_for_bytes(4) < 4) {
    show_error(READ_TIMEOUT, true);
    return;
  }
  for (int i = 0; i < 4; i++)
    local[i] = client->read();

  // Get packet length
  packet_len = local[0];
  packet_len += (local[1] << 8);
  packet_len += ((uint32_t)local[2] << 16);

  // We must wait for slow arriving packets for Ethernet shields only.
/*
  if (wait_for_bytes(packet_len) < packet_len) {
    show_error(READ_TIMEOUT, true);
    return;
  }
*/
  // Check for valid packet.
  if (packet_len < 0) {
    show_error(PACKET_ERROR, true);
    packet_len = 0;
  }
  buffer = (byte *)malloc(packet_len+4);
  if (buffer == NULL) {
    show_error(MEMORY_ERROR, true);
    return;
  }
  for (int i = 0; i < 4; i++)
    buffer[i] = local[i];

  for (int i = 4; i < packet_len+4; i++)
    buffer[i] = client->read();
}


/*
  parse_handshake_packet - Decipher the server's challenge data

  This method reads the server version string and the seed from the
  server. The handshake packet is defined as follows.

   Bytes                        Name
   -----                        ----
   1                            protocol_version
   n (Null-Terminated String)   server_version
   4                            thread_id
   8                            scramble_buff
   1                            (filler) always 0x00
   2                            server_capabilities
   1                            server_language
   2                            server_status
   2                            server capabilities (two upper bytes)
   1                            length of the scramble seed
  10                            (filler)  always 0
   n                            rest of the plugin provided data
                                (at least 12 bytes)
   1                            \0 byte, terminating the second part of
                                 a scramble seed
*/
void MySQL_Packet::parse_handshake_packet() {
  if (!buffer)
    return;

  int i = 5;
  do {
    i++;
  } while (buffer[i-1] != 0x00);

  server_version = (char *)malloc(i-5);
  strncpy(server_version, (char *)&buffer[5], i-5);

  // Capture the first 8 characters of seed
  i += 4; // Skip thread id
  for (int j = 0; j < 8; j++) {
    seed[j] = buffer[i+j];
  }

  // Capture rest of seed
  i += 27; // skip ahead
  for (int j = 0; j < 12; j++) {
    seed[j+8] = buffer[i+j];
  }
}

/*
  parse_error_packet - Display the error returned from the server

  This method parses an error packet from the server and displays the
  error code and text via Serial.print. The error packet is defined
  as follows.

  Note: the error packet is already stored in the buffer since this
        packet is not an expected response.

  Bytes                       Name
  -----                       ----
  1                           field_count, always = 0xff
  2                           errno
  1                           (sqlstate marker), always '#'
  5                           sqlstate (5 characters)
  n                           message
*/
void MySQL_Packet::parse_error_packet() {
#ifdef DEBUG
  Serial.print("Error: ");
  Serial.print(read_int(5, 2));
  Serial.print(" = ");

  if (!buffer)
    return;

  for (int i = 0; i < packet_len-9; i++)
    Serial.print((char)buffer[i+13]);
  Serial.println(".");
#endif
}


/*
  get_packet_type - Returns the packet type received from the server.

   Bytes                       Name
   -----                       ----
   1   (Length Coded Binary)   field_count, always = 0
   1-9 (Length Coded Binary)   affected_rows
   1-9 (Length Coded Binary)   insert_id
   2                           server_status
   2                           warning_count
   n   (until end of packet)   message

  Returns integer - 0 = successful parse, packet type if not an Ok packet
*/
int MySQL_Packet::get_packet_type() {
  if (!buffer)
    return -1;

  int type = buffer[4];
  return type;
}


/*
  get_lcb_len - Retrieves the length of a length coded binary value

  This reads the first byte from the offset into the buffer and returns
  the number of bytes (size) that the integer consumes. It is used in
  conjunction with read_int() to read length coded binary integers
  from the buffer.

  Returns integer - number of bytes integer consumes
*/
int MySQL_Packet::get_lcb_len(int offset) {
  if (!buffer)
    return 0;

  int read_len = buffer[offset];
  if (read_len > 250) {
    // read type:
    byte type = buffer[offset+1];
    if (type == 0xfc)
      read_len = 2;
    else if (type == 0xfd)
      read_len = 3;
    else if (type == 0xfe)
      read_len = 8;
  } else {
    read_len = 1;
  }
  return read_len;
}

/*
  read_int - Retrieve an integer from the buffer in size bytes.

  This reads an integer from the buffer at offset position indicated for
  the number of bytes specified (size).

  offset[in]      offset from start of buffer
  size[in]        number of bytes to use to store the integer

  Returns integer - integer from the buffer
*/
int MySQL_Packet::read_int(int offset, int size) {
  int value = 0;
  int new_size = 0;
  if (!buffer)
    return -1;
  if (size == 0)
     new_size = get_lcb_len(offset);
  if (size == 1)
     return buffer[offset];
  new_size = size;
  int shifter = (new_size - 1) * 8;
  for (int i = new_size; i > 0; i--) {
    value += (buffer[i-1] << shifter);
    shifter -= 8;
  }
  return value;
}


/*
  store_int - Store an integer value into a byte array of size bytes.

  This writes an integer into the buffer at the current position of the
  buffer. It will transform an integer of size to a length coded binary
  form where 1-3 bytes are used to store the value (set by size).

  buff[in]        pointer to location in internal buffer where the
                  integer will be stored
  value[in]       integer value to be stored
  size[in]        number of bytes to use to store the integer
*/
void MySQL_Packet::store_int(byte *buff, long value, int size) {
  memset(buff, 0, size);
  if (value < 0xff)
    buff[0] = (byte)value;
  else if (value < 0xffff) {
    buff[0] = (byte)value;
    buff[1] = (byte)(value >> 8);
  } else if (value < 0xffffff) {
    buff[0] = (byte)value;
    buff[1] = (byte)(value >> 8);
    buff[2] = (byte)(value >> 16);
  } else if (value < 0xffffff) {
    buff[0] = (byte)value;
    buff[1] = (byte)(value >> 8);
    buff[2] = (byte)(value >> 16);
    buff[3] = (byte)(value >> 24);
  }
}

/*
  read_lcb_int - Read an integer with len encoded byte

  This reads an integer from the buffer looking at the first byte in the offset
  as the encoded length of the integer.

  offset[in]      offset from start of buffer

  Returns integer - integer from the buffer
*/
int MySQL_Packet::read_lcb_int(int offset) {
  int len_size = 0;
  int size = 0;
  int value = 0;
  if (!buffer)
      return -1;
  len_size = buffer[offset];
  if (len_size < 252) {
    return buffer[offset];
  } else if (len_size == 252) {
    len_size = 2;
  } else if (len_size == 253) {
    len_size = 3;
  } else {
    len_size = 8;
  }
  int shifter = (len_size-1) * 8;
  for (int i = len_size; i > 0; i--) {
    value += (buffer[offset+i] << shifter);
    shifter -= 8;
  }
  return value;
}

/*
  print_packet - Print the contents of a packet via Serial.print

  This method is a diagnostic method. It is best used to decipher a
  packet from the server (or before being sent to the server). If you
  are looking for additional program memory space, you can safely
  delete this method.
*/
void MySQL_Packet::print_packet() {
  if (!buffer)
    return;

  Serial.print("Packet: ");
  Serial.print(buffer[3]);
  Serial.print(" contains ");
  Serial.print(packet_len+3);
  Serial.println(" bytes.");

  Serial.print("  HEX: ");
  for (int i = 0; i < packet_len+3; i++) {
    Serial.print(buffer[i], HEX);
    Serial.print(' ');
  }
  Serial.println();
  Serial.print("ASCII: ");
  for (int i = 0; i < packet_len+3; i++)
    Serial.print((char)buffer[i]);
  Serial.println();
}
