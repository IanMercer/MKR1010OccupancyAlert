/*
 *  Crowding alert
*/

#include <SPI.h>
#include <WiFiNINA.h>
#include <ArduinoBLE.h>
#include "Arduino.h"

// Maximum number of concurrent devices
#define N 100

#define UNKNOWNTYPE 0
#define IPHONETYPE 1
#define IWATCHTYPE 2
#define ANDROIDTYPE 3

#define RED_LED_PIN        6
#define GREEN_LED_PIN      7

// Define the ranges for the colors based on how many devices are detected in range
// If the store has many static bluetooth devices, increase these numbers
// If the store is larger, increase these numbers

#define GREEN_RANGE_TOP    4
#define YELLOW_RANGE_TOP   8


void blePeripheralConnectHandler(BLEDevice central) {
  Serial.print("Connected: ");
  Serial.println(central.address());
}

void blePeripheralDisconnectHandler(BLEDevice central) {
  Serial.print("Disconnected: ");
  Serial.println(central.address());
}

void blePeripheralDiscoveredHandler(BLEDevice device) {
  Serial.print("Discovered: ");
  Serial.println(device.address());
}

struct Device {
  unsigned int id;   // Sequential id (for debugging)
  byte address[6];   // 12:45:78:9A:CD:F0
  unsigned int device_type;
  unsigned long first_seen;
  unsigned long last_seen;
  unsigned int count;
  int32_t rssi;
  unsigned int distance;
  unsigned int column;
};

struct Device devices[N];

// Current number of devices < N being tracked
int n = 0;


/* 
    SETUP
*/
void setup() {

  Serial.begin(115200);
  
  Serial.println("================== STARTING ========================");

  // The MKR1010 RGB LED seems to be incompatible with Bluetooth so using two external LEDs

  pinMode( RED_LED_PIN, OUTPUT);
  pinMode( GREEN_LED_PIN, OUTPUT);
  analogWrite( GREEN_LED_PIN, 0);
  analogWrite( RED_LED_PIN, 255);

  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");
    while (1);
  }

  Serial.println("BLE Started");
  BLE.setLocalName("MKR1010");
  
  // start scanning for BLE advertisements
  BLE.scan(true);

  Serial.println("Scanning ...");

  analogWrite( RED_LED_PIN, 0);
  analogWrite( GREEN_LED_PIN, 255);
}

byte hexFromChars(unsigned char one, unsigned char two)
{
   int digitOne = (one >= 'a') ? (one - 'a' + 10) :(one >= 'A') ? (one - 'A' + 10) : (one - '0');
   int digitTwo = (two >= 'a') ? (two - 'a' + 10) :(two >= 'A') ? (two - 'A' + 10) : (two - '0');
   byte result = (byte)(digitOne*16 + digitTwo);
   return result;
}

char charFromHex(byte b) {
  return (b & 0xf) < 10 ? ('0' + (b & 0xf)) : ('A' + (b & 0xf) - 10);
}

void sprintAddress(char* pointer, byte* address) {
  for (int i = 0; i < 6; i++){
    *pointer++ = charFromHex(address[i] / 16);
    *pointer++ = charFromHex(address[i]);
    if (i < 5) *pointer++ = ':';
  }
}

/* 
   Get the BLE address as a byte array (less storage than a String)
*/
void get_address_as_bytes(BLEDevice device, byte* buff) {
  String a = device.address();
  buff[0] = hexFromChars(a.charAt(0), a.charAt(1));
  buff[1] = hexFromChars(a.charAt(3), a.charAt(4));
  buff[2] = hexFromChars(a.charAt(6), a.charAt(7));
  buff[3] = hexFromChars(a.charAt(9), a.charAt(10));
  buff[4] = hexFromChars(a.charAt(12), a.charAt(13));
  buff[5] = hexFromChars(a.charAt(15), a.charAt(16));
}

/* 
    Do two devices overlap in time? If so they must be distinct devices so place on separate columns
*/
bool overlaps (struct Device*a, struct Device*b) {
  if (a->column != b-> column) return false;       // already in different columns
  if (a->first_seen > b->last_seen) return false;  // a entirely after b
  if (b->first_seen > a->last_seen) return false;  // b entirely after a
  return true;
}

/* 
    Compute the minimum number of devices present by assigning each in a non-overlapping manner to columns
*/
void pack_columns()
{
  for (int i = 0; i < n; i++) {
    for (int j = i+1; j < n; j++) {
      if (overlaps(&devices[i], &devices[j])) {
        devices[j].column++;
      }
    }
  }
}

/* 
   Remove a device from array and move all later devices up one spot
*/
void remove_device(int index) {
  for (int i = index; i < n-1; i++) {
    devices[i] = devices[i+1];
    // decrease column count, may create clashes, will fix these up next
    devices[i].column = devices[i].column > 0 ? devices[i].column - 1 : 0;       
  }
  n--;
  pack_columns();
}

void print_device(struct Device d) {
  char* device_type = (char*)((d.device_type == IPHONETYPE) ? F("iPhone") :
    (d.device_type == IWATCHTYPE) ? F("iWatch") :
    (d.device_type == ANDROIDTYPE) ? F("Android") :
    F("")); 

  char text[80]; 
  sprintf(text, "%4i 01:34:67:9A:CD:F0 %3i %5i %10i-%10i %4i %s  ", d.id, d.column, d.count, d.first_seen, d.last_seen, d.rssi, device_type);
  sprintAddress(text+5, d.address);  // overwrite 17 characters starting at position 5 (after id)
  Serial.print(text);
}

/* 
   Red and green colors are determined by how many devices are present within range
*/
uint8_t red = 0;
uint8_t green = 255;

void print_table()
{
  Serial.println();
  Serial.println("-------------------------------------------------------------------");
  Serial.println("  Id Device            Col   Cnt            Time range RSSI Type");
  Serial.println("-------------------------------------------------------------------");
  unsigned int max_col = 0;
  for (int i = 0; i < n; i++) {
    Device d = devices[i];
    print_device(d);
    if (d.column > max_col) max_col = d.column;
    Serial.println();
  }
  char text[80]; 
  sprintf(text, "     Devices present: %4i                   at %6i", max_col, millis());
  Serial.println(text);
  Serial.println();

  // Compute colors
  
  if (max_col <= GREEN_RANGE_TOP) {
    red = 0; green = 255;
  } else if (max_col <= YELLOW_RANGE_TOP) {
    red = 200; green = 200;
  } else {
    red = 255; green = 0;
  }
}

void remove_expired() 
{
  for (int i = 0; i < n; i++) {
    Device d = devices[i];

    // Allow 1 minute for 1 count, 2 for 2, ... max 5 minute since seen
    // This removes spurious single-count devices quickly (sometimes bluetooth signals can reflect and go a long way!)

    int allowed_seconds = d.count > 5 ? 5*60 : d.count * 60;
    
    if (d.last_seen + 1000*allowed_seconds < millis()) {
      remove_device(i);
      Serial.print("Removed: ");
      print_device(d);
      Serial.println();
    }
  }
}

unsigned long last_report = 0;
unsigned int id_gen = 0;

/* 
   MAIN LOOP
*/
void loop() {

    unsigned long tick = millis();

    // pulse the LEDs the appropriate color
    float y = 192 + 64 * sin((float)tick / 1024);
    
    analogWrite( RED_LED_PIN, (int)(red * y / 256));
    analogWrite( GREEN_LED_PIN, (int)(green * y / 256));

    BLEDevice peripheral = BLE.available();

    if (peripheral) {

      byte buff[6];
      get_address_as_bytes(peripheral, buff);

      int index = -1;
      for (int i = 0; i < n; i++) {
        if (memcmp(devices[i].address, buff, 6) == 0) {
           // found 
           index = i;
           break;
        }
      }

      if (index == -1 && (n == N)) {
        Serial.println("No room in array, should have cleared an old one out ealier");
        return;
      } else if (index == -1) {
        index = n++;
        devices[index].id = ++id_gen;
        memcpy(devices[index].address, buff, sizeof(buff));
        devices[index].device_type = UNKNOWNTYPE;
        devices[index].first_seen = tick;
        devices[index].last_seen = tick;
        devices[index].count = 1;
        devices[index].column = 0;
        devices[index].rssi = peripheral.rssi();
        // Can calculate an approximate distance from this
        // Could filter based on distance to ignore devices further away

        Serial.print(peripheral.address());
        Serial.print("  Connecting ...");

        // Have to stop scanning to attempt a connection
        BLE.stopScan();

        if (peripheral.connect()) {
          if (peripheral.discoverAttributes()) {

            // read and print device name of peripheral
            Serial.print(" Device name: ");
            Serial.println(peripheral.deviceName());

            if (peripheral.deviceName() == F("iPhone")) {
              devices[index].device_type = IPHONETYPE;  
            } else if (peripheral.deviceName() == F("iWatch")) {
              devices[index].device_type = IPHONETYPE;  
            }

          } else {
            Serial.println(" Attribute discovery failed (not unusual)");
          }
      
          peripheral.disconnect();
      
        } else {
          Serial.println(" Unable to connect (not unusual)");
        }
        // and resume scanning
        BLE.scan(true);
        
      } else {
        devices[index].last_seen = tick;
        devices[index].count++;
        devices[index].rssi = peripheral.rssi();
      }

    }    

    // Every 30s dump the array to console
    if (tick > last_report + 30*1000) {

      // Pack columns (Calendar-like arrangement algorithm to find all Devices that could be the same device as a result of MAC address randomization)
      pack_columns();

      // Remove any devices from array that have expired (not seen in last 5 minutes)
      remove_expired();

      print_table();

      last_report = tick;
    }
}
