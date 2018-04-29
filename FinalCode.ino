#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include "DHT.h"
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <UbidotsMicroESP8266.h>
#include <FS.h> //spiff file system

int16_t accelX, accelY, accelZ;
float gForceX, gForceY, gForceZ, AccMag =0;
int lux = 0;
float h, f;
String line;
unsigned long timestamp;
static const char ntpServerName[] = "us.pool.ntp.org";
const int timeZone = -4;     
WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets
time_t getNtpTime();

#define HumidityRH "5a8b1bc8c03f977da6a45ef1"
#define TemperatureF "5a8b1bc4c03f977de3327aa8" 
#define TOKEN "A1E-QrVm0RfSxsS8QSKFRT6BcdRgWsJaBN"
#define AccelerometerX "5a90e2e7c03f971788be2913" // Replace it with your Ubidots' variable ID
#define AccelerometerY "5a91e062c03f9733092222c9"
#define AccelerometerZ "5a91e069c03f9733092222cb"
#define ShockMagnitude "5a91f6fec03f97492ca09019"
#define Light "5a8b1b1dc03f977c85f928a8"
#define TSLAddr 0x39
#define MPUAddr 0x68

const char* ssid     = "iPhone";         // The SSID (name) of the Wi-Fi network you want to connect to
const char* password = "11223344";     // The password of the Wi-Fi network
char filename [] = "/datalog.txt";                     // Assign a filename or use the format e.g. SD.open("datalog.txt",...);
File myDataFile;
#define DHTPIN 10     // what digital pin the DHT22 is conected to
#define DHTTYPE DHT22   
 Ubidots client(TOKEN);
 DHT dht(DHTPIN, DHTTYPE);
void setup() {

  wifiConnection();
  Wire.begin(4,5);
  setupTSL();
  timeSetup();
  creatTxt();
  setupMPU();
}

void loop() {
  recordAccelRegisters();
  recordTempAndHumidity();
  readTSL();
  printData();
  sendToUbidots();
  Serial.println("Wifi is connected. This is the IP address:\t");     // Write some data to it (26-characters)
  Serial.println(WiFi.localIP());         // Send the IP address of the ESP8266 to the computer

  if(WiFi.status() != WL_CONNECTED) { 
      LED_OFF();
      while(WiFi.status() != WL_CONNECTED){
            Serial.println("Wifi is disconnected! Looking for a network!");     // Write some data to it (26-characters)
            wifiReconnect();
            recordAccelRegisters();
            readTSL();
            recordTempAndHumidity();
            printData();
            logTxt(AccMag, gForceX, gForceY, gForceZ, h, f, lux);
            readTxt();
      }
      LED_ON();
      parsing();
 }

}

//================ Functions =======================
void logTxt(float magshock, float valueX, float valueY, float valueZ, float humidity, float temperature, int luxes){
  Serial.println("entering logTxt");
  myDataFile = SPIFFS.open(filename, "a");        // Open a file for reading and writing (appending)
  myDataFile.print("Shock:");     // Write some data to it (26-characters)
  myDataFile.print(magshock);     // Write some data to it (26-characters)
  myDataFile.print(" X:");     // Write some data to it (26-characters)
  myDataFile.print(valueX);     // Write some data to it (26-characters)
  myDataFile.print(" Y:");     // Write some data to it (26-characters)
  myDataFile.print(valueY);     // Write some data to it (26-characters)
  myDataFile.print(" Z:");     // Write some data to it (26-characters)
  myDataFile.print(valueZ);     // Write some data to it (26-characters)
  myDataFile.print(" H:");     // Write some data to it (26-characters)
  myDataFile.print(humidity);     // Write some data to it (26-characters)
  myDataFile.print(" F:");     // Write some data to it (26-characters)
  myDataFile.print(temperature);     // Write some data to it (26-characters)
  myDataFile.print(" L:");
  myDataFile.print(luxes);
  myDataFile.print(" Time: ");
  myDataFile.println(now());
Serial.println("exiting logTxt");
  
//  Serial.println(myDataFile.size());                    // Display the file size (26 characters + 4-byte floating point number + 6 termination bytes (2/line) = 34 bytes)
}
void readTxt(){
   myDataFile = SPIFFS.open(filename, "r");              // Open the file again, this time for reading
  if (!myDataFile) Serial.println("file open failed");  // Check for errors
  while (myDataFile.available()) {
    Serial.write(myDataFile.read());                    // Read all the data from the file and display it
  }
  myDataFile.close();                                   // Close the file
}
void creatTxt(){
  Serial.println("entering creatTxt");
  SPIFFS.begin();
  if (SPIFFS.exists(filename)) SPIFFS.remove(filename); // First in this example check to see if a file already exists, if so delete it
  if (!myDataFile)Serial.println("file open failed");   // Check for errors 
  Serial.println("exiting creatTxt");
}
void setupMPU(){
 // Wire.begin(4,5);
  Wire.beginTransmission(0b1101000); //This is the I2C address of the MPU (b1101000/b1101001 for AC0 low/high datasheet sec. 9.2)
  Wire.write(0x6B); //Accessing the register 6B - Power Management (Sec. 4.28)
  Wire.write(0b00000000); //Setting SLEEP register to 0. (Required; see Note on p. 9)
  Wire.endTransmission();  
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x1C); //Accessing the register 1C - Acccelerometer Configuration (Sec. 4.5) 
  Wire.write(0b00011000); //Setting the accel to +/- 16g
  Wire.endTransmission(); 
}
void setupTSL(){
  
  Wire.beginTransmission(TSLAddr);  // Configure TSL
  Wire.write(0x00 | 0x80);          // Select control register
  Wire.write(0x03);                 // Power ON mode
  Wire.endTransmission();           
  Wire.beginTransmission(TSLAddr);  // Starts I2C communication
  Wire.write(0x01 | 0x80);          // Select timing register
  Wire.write(0x02);                 // Nominal integration time = 402ms
  Wire.endTransmission();
}

void wifiConnection(){
  
  Serial.begin(115200);
  WiFi.begin(ssid, password);             // Connect to the network
  Serial.print("Connecting to ");
  Serial.print(ssid); Serial.println(" ...");
    int i = 0;
    while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(1000);
    Serial.print(++i); Serial.print(' ');
    if(i>20) break;
  }
  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());         // Send the IP address of the ESP8266 to the computer
  LED_ON();
}

void wifiReconnect(){
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    WiFi.mode(WIFI_STA);
    delay(1000);                      // Wait for a second
    WiFi.begin(ssid, password);             // Connect to the network
    Serial.print("Connecting to ");
    Serial.print(ssid); Serial.println(" ...");
    int i = 0;
    while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(1000);
    Serial.print(++i); Serial.print(' ');
    if(i>5) break;
  }
  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());         // Send the IP address of the ESP8266 to the computer
}

void sendToUbidots(){
  client.add("Shock Magnitude",AccMag,NULL, timestamp);
  client.add("AccelerometerX",gForceX,NULL, timestamp);
  client.add("AccelerometerY",gForceY,NULL, timestamp);
  client.add("AccelerometerZ",gForceZ,NULL, timestamp);
  client.add("Light", lux, NULL, timestamp);
  client.sendAll(true);
  client.add("Humidity",h,NULL, timestamp);   
  Serial.print("time stamp sent: ");
  Serial.println(timestamp);
  client.add("Temperature", f, NULL, timestamp);
  client.sendAll(true);

}
void recordAccelRegisters() {
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x3B); //Starting register for Accel Readings
  Wire.endTransmission();
  Wire.requestFrom(0b1101000,6); //Request Accel Registers (3B - 40)
  while(Wire.available() < 6);
  accelX = Wire.read()<<8|Wire.read(); //Store first two bytes into accelX
  accelY = Wire.read()<<8|Wire.read(); //Store middle two bytes into accelY
  accelZ = Wire.read()<<8|Wire.read(); //Store last two bytes into accelZ
  processAccelData();
}

void readTSL(){
  unsigned int dataTSL[4];
  for(int i = 0; i < 4; i++)
  {
    Wire.beginTransmission(TSLAddr);
    Wire.write((140 + i));          // Select data register
    Wire.endTransmission();
    
    Wire.requestFrom(TSLAddr, 1);   // Request 1 byte of data
    if(Wire.available() == 1)       // Read 1 bytes of data
    {
      dataTSL[i] = Wire.read();
     }
     delay(100);
  }
  
  // Convert the data
  double ch0 = ((dataTSL[1] & 0xFF) * 256) + (dataTSL[0] & 0xFF);
  double ch1 = ((dataTSL[3] & 0xFF) * 256) + (dataTSL[2] & 0xFF);
  lux = ch0-ch1;

  // Output data to serial monitor
  Serial.print("Visible Value :");
  Serial.println(lux);
  //client.add(Light,lux);
}
void recordTempAndHumidity(){
  
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
     h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
     f = dht.readTemperature(true);

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t) || isnan(f)) {
      Serial.println("Failed to read from DHT sensor!");
    }

 
}

void processAccelData(){
  gForceX = accelX/2048.0;
  gForceY = accelY/2048.0; 
  gForceZ = accelZ/2048.0; 

     AccMag= sqrt( sq(gForceX) + sq(gForceY)+ sq(gForceZ) );
}

void printData() {
     Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" %\t");
    Serial.print("Temperature: ");

    Serial.print(f);
    Serial.println(" *F\t");

  Serial.print("Accelerometer:  ");
  Serial.print(" Shock Magnitude= "); Serial.print(AccMag, 4);
  Serial.print(" || X="); Serial.print(gForceX);
  Serial.print(" || Y="); Serial.print(gForceY); 
  Serial.print(" || Z="); Serial.println(gForceZ);
  Serial.print("Orientation:  ");
  if(gForceZ>0.9) Serial.println("** Box is placed on normal position **"); 
  else if(gForceZ<-0.9) Serial.println("** Box is placed upside-down **");
  else if( (gForceX> 0.9) || (gForceX< -0.9) || (gForceY> 0.9) || (gForceY< -0.9) ) Serial.println("** Box is placed on side **");
  else Serial.println ("** Box is tilted **");
  Serial.print("Shocks:  ");
  if(AccMag<1.5) Serial.println("** Box is stationary **\n"); 
  else if( (AccMag>1.5) && (AccMag<3)) Serial.println("** Box is being shaked lightly **\n");
  else if( AccMag>3) Serial.println("** Box is being shaked extremely **\n");
  else ;
}

void LED_ON(){
    digitalWrite(LED_BUILTIN, LOW);   // Turn the LED ON
}
void LED_OFF(){
    digitalWrite(LED_BUILTIN, HIGH);   // Turn the LED OFF 
}

void parsing(){
  myDataFile = SPIFFS.open(filename, "r");              // Open the file again, this time for reading
  if (!myDataFile) Serial.println("file open failed");  // Check for errors
  
  while (myDataFile.available()) {
    char c = myDataFile.read();
    if (c == '\n') {
      parseLine(line);
      line = "";
  }
   else {
      line += c;
    }
}
  myDataFile.close();                                   // Close the file
}


void parseLine(String strLine) {
  String part1; //mag
  String part2; //acc x
  String part3; //acc y
  String part4; //acc z
  String humidity;
  String temperature;
  String timesince;
  String luxstr;
  part1 = strLine.substring(strLine.indexOf("Shock:")+ 6, strLine.indexOf(" X:")); //mag
  part2 = strLine.substring(strLine.indexOf(" X:") + 3, strLine.indexOf(" Y:")); //X
  part3 = strLine.substring(strLine.indexOf(" Y:") +3, strLine.indexOf(" Z:")); //Y
  part4 = strLine.substring(strLine.indexOf(" Z:") + 3, strLine.indexOf(" H:")); //Z
  humidity = strLine.substring(strLine.indexOf(" H:") + 3, strLine.indexOf(" F:"));
  temperature = strLine.substring(strLine.indexOf(" F:") + 3, strLine.indexOf(" L:"));
  //strLine.substring(strLine.indexOf(" Date: ")+ 1).toCharArray(timestamp, 40);
  luxstr = strLine.substring(strLine.indexOf(" L:")+3, strLine.indexOf(" Time: "));
  timesince = strLine.substring(strLine.indexOf(" Time: ")+7);

 Serial.println("****** Parsing:  ******"); 
 Serial.println(part1); 
 Serial.println(part2); 
 Serial.println(part3); 
 Serial.println(part4); 
 //Serial.println(timestamp);
 Serial.println(timesince);
 Serial.println(humidity);
 Serial.println(temperature);
  AccMag = part1.toFloat();
  gForceX = part2.toFloat();
  gForceY = part3.toFloat();
  gForceZ = part4.toFloat();
  h=humidity.toFloat();
  f=temperature.toFloat();
  lux= luxstr.toInt();
  timestamp=timesince.toInt();
  Serial.print("time after toFloat");
  Serial.println(timestamp);
  
   sendToUbidots();

}

void timeSetup(){
  Serial.print("IP number assigned by DHCP is ");
  Serial.println(WiFi.localIP());
  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
