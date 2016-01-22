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

  MySQL_Cursor.cpp - Run queries on a MySQL Server

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
#include <MySQL_Cursor.h>

const char BAD_MOJO[] PROGMEM = "Bad mojo. EOF found reading column header.";
const char ROWS[] PROGMEM = " rows in result.";
const char READ_COLS[] PROGMEM = "ERROR: You must read the columns first!";
const char NOT_CONNECTED[] PROGMEM = "ERROR: Class requires connected server.";

/*
  Constructor

  Requires an instance of the MySQL_Connection class to communicate with a
  MySQL server.

  connection[in]  Connection to a MySQL server - must be connected.
*/
MySQL_Cursor::MySQL_Cursor(MySQL_Connection *connection) {
  conn = connection;
  if (!conn->connected()) {
    Serial.println(NOT_CONNECTED);
  }
  columns.num_fields = 0;
  for (int f = 0; f < MAX_FIELDS; f++) {
    columns.fields[f] = NULL;
    row.values[f] = NULL;
  }
  columns_read = false;
}


/*
  execute - Execute a SQL statement

  This method executes the query specified as a character array. It copies
  the query to the local buffer then calls the execute_query() method to
  execute the query.

  If a result set is available after the query executes, the field
  packets and rows can be read separately using the get_field() and
  get_row() methods.

  query[in]       SQL statement (using normal memory access)
  progmem[in]     True if string is in program memory

  Returns boolean - True = a result set is available for reading
*/
boolean MySQL_Cursor::execute(const char *query, boolean progmem)
{
  int query_len;   // length of query

  if (progmem) {
    query_len = (int)strlen_P(query);
  } else {
    query_len = (int)strlen(query);
  }
  if (conn->buffer != NULL)
    free(conn->buffer);

  conn->buffer = (byte *)malloc(query_len+5);

  // Write query to packet
  if (progmem) {
    for (int c = 0; c < query_len; c++)
      conn->buffer[c+5] = pgm_read_byte_near(query+c);
  } else {
    memcpy(&conn->buffer[5], query, query_len);
  }

  // Send the query
  return execute_query(query_len);
}


/*
  get_columns - Get a list of the columns (fields)

  This method returns an instance of the column_names structure
  that contains an array of fields.

  Note: you should call free_columns_buffer() after consuming
        the field data to free memory.
*/
column_names *MySQL_Cursor::get_columns() {
  free_columns_buffer();
  free_row_buffer();
  num_cols = 0;
  if (get_fields()) {
    columns_read = true;
    return &columns;
  }
  else {
    return NULL;
  }
}


/*
  get_next_row - Iterator for reading rows from a result set

  This method returns an instance of a structure (row_values)
  that contains an array of strings representing the row
  values returned from the server.

  The caller can use the values however needed - by first
  converting them to a specific type or as a string.
*/
row_values *MySQL_Cursor::get_next_row() {
  int res = 0;

  free_row_buffer();

  // Read the rows
  res = get_row_values();
  if (res != MYSQL_EOF_PACKET) {
    return &row;
  }
  return NULL;
}


/*
  show_results - Show a result set from the server via Serial.print

  This method reads a result from the server and displays it via the
  via the Serial.print methods. It can be used in cases where
  you may want to issue a SELECT or SHOW and see the results on your
  computer from the Arduino.

  It is also a good example of how to read a result set from the
  because it uses the public methods designed to return result
  sets from the server.
*/
void MySQL_Cursor::show_results() {
  column_names *cols;
  int rows = 0;

  // Get the columns
  cols = get_columns();
  if (cols == NULL) {
    return;
  }

  for (int f = 0; f < columns.num_fields; f++) {
    Serial.print(columns.fields[f]->name);
    if (f < columns.num_fields-1)
      Serial.print(',');
  }
  Serial.println();

  // Read the rows
  while (get_next_row()) {
    rows++;
    for (int f = 0; f < columns.num_fields; f++) {
      Serial.print(row.values[f]);
      if (f < columns.num_fields-1)
        Serial.print(',');
    }
    free_row_buffer();
    Serial.println();
  }

  // Report how many rows were read
  Serial.print(rows);
  conn->show_error(ROWS, true);
  free_columns_buffer();

  // Free any post-query messages in queue for stored procedures
  clear_ok_packet();
}


/*
  Close

  Takes care of removing allocated memory.
*/
void MySQL_Cursor::close() {
  free_columns_buffer();
  free_row_buffer();
}


// Begin private methods

/*
  execute_query - execute a query

  This method sends the query string to the server and waits for a
  response. If the result is a result set, it returns true, if it is
  an error, it processes the error packet and prints the error via
  Serial.print(). If it is an Ok packet, it parses the packet and
  returns false.

  query_len[in]   Number of bytes in the query string

  Returns boolean - true = result set available,
                    false = no result set returned.
*/
boolean MySQL_Cursor::execute_query(int query_len)
{
  conn->store_int(&conn->buffer[0], query_len+1, 3);
  conn->buffer[3] = byte(0x00);
  conn->buffer[4] = byte(0x03);  // command packet

  // Send the query
  for (int c = 0; c < query_len+5; c++)
    conn->client->write(conn->buffer[c]);

  // Read a response packet and check it for Ok or Error.
  conn->read_packet();
  int res = conn->check_ok_packet();
  if (res == MYSQL_ERROR_PACKET) {
    conn->parse_error_packet();
    return false;
  } else if (!res) {
    return false;
  }
  // Not an Ok packet, so we now have the result set to process.
  columns_read = false;
  return true;
}


/*
  clear_ok_packet - clear last Ok packet (if present)

  This method reads the header and status to see if this is an Ok packet.
  If it is, it reads the packet and discards it. This is useful for
  processing result sets from stored procedures.

  Returns False if the packet was not an Ok packet.
*/
bool MySQL_Cursor::clear_ok_packet() {
  int num = 0;

  do {
    num = conn->client->available();
    if (num > 0) {
      conn->read_packet();
      if (conn->check_ok_packet() != 0) {
        conn->parse_error_packet();
        return false;
      }
    }
  } while (num > 0);
  return true;
}


/*
  free_columns_buffer - Free memory allocated for column names

  This method frees the memory allocated during the get_columns()
  method.

  NOTICE: Failing to call this method after calling get_columns()
          and consuming the column names, types, etc. will result
          in a memory leak. The size of the leak will depend on
          the size of the combined column names (bytes).
*/
void MySQL_Cursor::free_columns_buffer() {
  // clear the columns
  for (int f = 0; f < MAX_FIELDS; f++) {
    if (columns.fields[f] != NULL) {
      free(columns.fields[f]->db);
      free(columns.fields[f]->table);
      free(columns.fields[f]->name);
      free(columns.fields[f]);
    }
    columns.fields[f] = NULL;
  }
  num_cols = 0;
  columns_read = false;
}


/*
  free_row_buffer - Free memory allocated for row values

  This method frees the memory allocated during the get_next_row()
  method.

  NOTICE: You must call this method at least once after you
          have consumed the values you wish to process. Failing
          to do will result in a memory leak equal to the sum
          of the length of values and one byte for each max cols.
*/
void MySQL_Cursor::free_row_buffer() {
  // clear the row
  for (int f = 0; f < MAX_FIELDS; f++) {
    if (row.values[f] != NULL) {
      free(row.values[f]);
    }
    row.values[f] = NULL;
  }
}


/*
  read_string - Retrieve a string from the buffer

  This reads a string from the buffer. It reads the length of the string
  as the first byte.

  offset[in]      offset from start of buffer

  Returns string - String from the buffer
*/
char *MySQL_Cursor::read_string(int *offset) {
  int len_bytes = conn->get_lcb_len(conn->buffer[*offset]);
  int len = conn->read_int(*offset, len_bytes);
  char *str = (char *)malloc(len+1);
  strncpy(str, (char *)&conn->buffer[*offset+len_bytes], len);
  str[len] = 0x00;
  *offset += len_bytes+len;
  return str;
}


/*
  get_field - Read a field from the server

  This method reads a field packet from the server. Field packets are
  defined as:

  Bytes                      Name
  -----                      ----
  n (Length Coded String)    catalog
  n (Length Coded String)    db
  n (Length Coded String)    table
  n (Length Coded String)    org_table
  n (Length Coded String)    name
  n (Length Coded String)    org_name
  1                          (filler)
  2                          charsetnr
  4                          length
  1                          type
  2                          flags
  1                          decimals
  2                          (filler), always 0x00
  n (Length Coded Binary)    default

  Note: the sum of all db, column, and field names must be < 255 in length
*/
int MySQL_Cursor::get_field(field_struct *fs) {
  int len_bytes;
  int len;
  int offset;

  // Read field packets until EOF
  conn->read_packet();
  if (conn->buffer[4] != MYSQL_EOF_PACKET) {
    // calculate location of db
    len_bytes = conn->get_lcb_len(4);
    len = conn->read_int(4, len_bytes);
    offset = 4+len_bytes+len;
    fs->db = read_string(&offset);
    // get table
    fs->table = read_string(&offset);
    // calculate location of name
    len_bytes = conn->get_lcb_len(offset);
    len = conn->read_int(offset, len_bytes);
    offset += len_bytes+len;
    fs->name = read_string(&offset);
    return 0;
  }
  return MYSQL_EOF_PACKET;
}


/*
  get_row - Read a row from the server and store it in the buffer

  This reads a single row and stores it in the buffer. If there are
  no more rows, it returns MYSQL_EOF_PACKET. A row packet is defined as
  follows.

  Bytes                   Name
  -----                   ----
  n (Length Coded String) (column value)
  ...

  Note: each column is store as a length coded string concatenated
        as a single stream

  Returns integer - MYSQL_EOF_PACKET if no more rows, 0 if more rows available
*/
int MySQL_Cursor::get_row() {
  // Read row packets
  conn->read_packet();
  if (conn->buffer[4] != MYSQL_EOF_PACKET)
    return 0;
  return MYSQL_EOF_PACKET;
}


/*
  get_fields - reads the fields from the read buffer

  This method is used to read the field names, types, etc.
  from the read buffer and store them in the columns structure
  in the class.
*/
boolean MySQL_Cursor::get_fields()
{
  int num_fields = 0;
  int res = 0;

  if (conn->buffer == NULL) {
    return false;
  }
  num_fields = conn->buffer[4]; // From result header packet
  columns.num_fields = num_fields;
  num_cols = num_fields; // Save this for later use
  for (int f = 0; f < num_fields; f++) {
    field_struct *field = (field_struct *)malloc(sizeof(field_struct));
    res = get_field(field);
    if (res == MYSQL_EOF_PACKET) {
      conn->show_error(BAD_MOJO, true);
      return false;
    }
    columns.fields[f] = field;
  }
  conn->read_packet(); // EOF packet
  return true;
}


/*
  get_row_values - reads the row values from the read buffer

  This method is used to read the row column values
  from the read buffer and store them in the row structure
  in the class.
*/
int MySQL_Cursor::get_row_values() {
  int res = 0;
  int offset = 0;

  // It is an error to try to read rows before columns
  // are read.
  if (!columns_read) {
    conn->show_error(READ_COLS, true);
    return MYSQL_EOF_PACKET;
  }
  // Drop any row data already read
  free_row_buffer();

  // Read a row
  res = get_row();
  if (res != MYSQL_EOF_PACKET) {
    offset = 4;
    for (int f = 0; f < num_cols; f++) {
      row.values[f] = read_string(&offset);
    }
  }
  return res;
}
