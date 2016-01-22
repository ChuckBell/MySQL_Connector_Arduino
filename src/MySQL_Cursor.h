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

  MySQL_Cursor.h - Run queries on a MySQL Server

  This header file defines a cursor class for running queries on a MySQL
  server. You can issue any command using SQL statements for inserting or
  retrieving data.

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
#ifndef MYSQL_QUERY_H
#define MYSQL_QUERY_H

#include <MySQL_Connection.h>

#define MAX_FIELDS    0x20   // Maximum number of fields. Reduce to save memory. Default=32

// Structure for retrieving a field (minimal implementation).
typedef struct {
  char *db;
  char *table;
  char *name;
} field_struct;

// Structure for storing result set metadata.
typedef struct {
  int num_fields;     // actual number of fields
  field_struct *fields[MAX_FIELDS];
} column_names;

// Structure for storing row data.
typedef struct {
  char *values[MAX_FIELDS];
} row_values;

class MySQL_Cursor {
  public:
    MySQL_Cursor(MySQL_Connection *connection);
    ~MySQL_Cursor() { close(); };
    boolean execute(const char *query, boolean progmem=false);
    column_names *get_columns();
    row_values *get_next_row();
    void show_results();
    void close();

  private:
    boolean columns_read;
    MySQL_Connection *conn;
    int num_cols;
    column_names columns;
    row_values row;

    void free_columns_buffer();
    void free_row_buffer();
    bool clear_ok_packet();
    boolean execute_query(int query_len);
    char *read_string(int *offset);
    int get_field(field_struct *fs);
    int get_row();
    boolean get_fields();
    int get_row_values();
    column_names *query_result();
};

#endif
