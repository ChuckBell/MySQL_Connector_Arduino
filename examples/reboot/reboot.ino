/*
  MySQL Connector/Arduino Example : reboot if connection lost

  This example demonstrates how to reboot an Arduino if connection to the
  server is lost for a period of time.

  INSTRUCTIONS FOR USE

  1) Change the address of the server to the IP address of the MySQL server
  2) Change the user and password to a valid MySQL user and password
  3) Connect a USB cable to your Arduino
  4) Select the correct board and port
  5) Compile and upload the sketch to your Arduino
  6) Once uploaded, open Serial Monitor (use 115200 speed) and observe

  To test the reboot, unplug your Ethernet cable once you see "disconnected"
  then wait for the timeout. Once the Arduino reboots, plug the cable in again
  and you should see the query processing resume.

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

// Begin reboot code
int num_fails;                  // variable for number of failure attempts
#define MAX_FAILED_CONNECTS 5   // maximum number of failed connects to MySQL

void soft_reset() {
  asm volatile("jmp 0");
}
// End reboot code

void loop() {
  Serial.println("Sleeping...");
  delay(1000);
  if (conn.connected()) {
    Serial.println("Running a query");
    cur.execute("SHOW DATABASES"); // execute a query
    cur.show_results();            // show the results
    cur.close();                   // close the cursor
    conn.close();                  // close the connection
    num_fails = 0;                 // reset failures
    delay(3000);
  } else {
    Serial.println("Connecting...");
    if (conn.connect(server_addr, 3306, user, password)) {
      delay(500);
    } else {
      num_fails++;
      Serial.println("Connect failed!");
      if (num_fails == MAX_FAILED_CONNECTS) {
        Serial.println("Ok, that's it. I'm outta here. Rebooting...");
        delay(2000);
        // Here we tell the Arduino to reboot by redirecting the instruction
        // pointer to the "top" or position 0. This is a soft reset and may
        // not solve all hardware-related lockups.
        soft_reset();
      }
    }
  }
}
