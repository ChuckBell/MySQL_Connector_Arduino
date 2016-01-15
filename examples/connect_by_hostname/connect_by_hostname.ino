/*
  MySQL Connector/Arduino Example : connect by hostname

  This example demonstrates how to connect to a MySQL server resolving the
  hostname for cases when you do not know the IP address of the server or
  it changes because it is in the cloud.

  INSTRUCTIONS FOR USE

  1) Change the hostname variable to the hostname of the MySQL server
  2) Change the user and password to a valid MySQL user and password
  3) Connect a USB cable to your Arduino
  4) Select the correct board and port
  5) Compile and upload the sketch to your Arduino
  6) Once uploaded, open Serial Monitor (use 115200 speed) and observe

  If you do not see messages indicating you have a connection, refer to the
  manual for troubleshooting tips. The most common issues are the server is
  not accessible from the network or the user name and password is incorrect.

  Note: The MAC address can be anything so long as it is unique on your network.

  Created by: Dr. Charles A. Bell
*/
#include <Ethernet.h>
#include <MySQL_Connection.h>
#include <Dns.h>

byte mac_addr[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

char hostname[] = "www.google.com"; // change to your server's hostname/URL
char user[] = "root";               // MySQL user login username
char password[] = "secret";         // MySQL user login password

IPAddress server_ip;
EthernetClient client;
MySQL_Connection conn((Client *)&client);
DNSClient dns_client;   // DNS instance

void setup() {
  Serial.begin(115200);
  while (!Serial); // wait for serial port to connect
  Ethernet.begin(mac_addr);
  // Begin DNS lookup
  dns_client.begin(Ethernet.dnsServerIP());
  dns_client.getHostByName(hostname, server_ip);
  Serial.println(server_ip);
  // End DNS lookup
  Serial.println("Connecting...");
  if (conn.connect(server_ip, 3306, user, password)) {
    delay(1000);
    // You would add your code here to run a query once on startup.
  }
  else
    Serial.println("Connection failed.");
  conn.close();
}

void loop() {
}
