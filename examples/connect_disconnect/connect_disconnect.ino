/*
  MySQL Connector/Arduino Example : connect and disconnect (close)

  This example demonstrates how to use the connection to open at the start
  of a loop, perform some query, then close the connection. Use this technique
  for solutions that must sleep for a long period or otherwise require
  additional processing or delays. The connect/close pair allow you to
  control how long the connection is open and thus reduce the amount of
  time a connection is held open. It also helps for lossy connections.

  INSTRUCTIONS FOR USE

  1) Change the address of the server to the IP address of the MySQL server
  2) Change the user and password to a valid MySQL user and password
  3) Connect a USB cable to your Arduino
  4) Select the correct board and port
  5) Compile and upload the sketch to your Arduino
  6) Once uploaded, open Serial Monitor (use 115200 speed) and observe

  Created by: Dr. Charles A. Bell
*/
#include <Ethernet.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>

byte mac_addr[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

IPAddress server_addr(10,0,1,35);  // IP of the MySQL *server* here
char user[] = "root";              // MySQL user login username
char password[] = "secret";        // MySQL user login password

EthernetClient client;
MySQL_Connection conn((Client *)&client);
MySQL_Cursor cur = MySQL_Cursor(&conn);

void setup() {
  Serial.begin(115200);
  while (!Serial); // wait for serial port to connect
  Ethernet.begin(mac_addr);
}

void loop() {
  Serial.println("Sleeping...");
  delay(2000);
  Serial.println("Connecting...");
  if (conn.connect(server_addr, 3306, user, password)) {
    delay(500);
    Serial.println("Running a query");
    cur.execute("SHOW DATABASES"); // execute a query
    cur.show_results();            // show the results
    cur.close();                   // close the cursor
    conn.close();                  // close the connection
  } else {
    Serial.println("Connect failed. Trying again on next iteration.");
  }
}
