


#include <Wire.h>
#include <ESP8266WiFi.h>
#include <UbidotsMicroESP8266.h>
#include <FS.h> //spiff file system

int16_t accelX, accelY, accelZ;
float gForceX, gForceY, gForceZ, AccMag;
String line;


#define TOKEN "A1E-QrVm0RfSxsS8QSKFRT6BcdRgWsJaBN"
#define AccelerometerX "5a90e2e7c03f971788be2913" // Replace it with your Ubidots' variable ID
#define AccelerometerY "5a91e062c03f9733092222c9"
#define AccelerometerZ "5a91e069c03f9733092222cb"
#define ShockMagnitude "5a91f6fec03f97492ca09019"
const char* ssid     = "iPhone";         // The SSID (name) of the Wi-Fi network you want to connect to
const char* password = "11223344";     // The password of the Wi-Fi network
char filename [] = "/datalog.txt";                     // Assign a filename or use the format e.g. SD.open("datalog.txt",...);
File myDataFile;

 Ubidots client(TOKEN);
 
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  LED_ON();
  wifiConnection();
  creatTxt();
  setupMPU();
}

void loop() {
  recordAccelRegisters();
  printData();
  sendToUbidots();
  Serial.println("Wifi is connected. This is the IP address:\t");     // Write some data to it (26-characters)
  Serial.println(WiFi.localIP());         // Send the IP address of the ESP8266 to the computer

  if(WiFi.status() != WL_CONNECTED) { 
      LED_ON();
      while(WiFi.status() != WL_CONNECTED){
            Serial.println("Wifi is disconnected! Looking for a network!");     // Write some data to it (26-characters)
            wifiReconnect();
            recordAccelRegisters();
            printData();
            logTxt(AccMag, gForceX, gForceY, gForceZ);
            readTxt();
      }
      LED_OFF();
      parsing();
 }

}

//================ Functions =======================
void logTxt(float magshock, float valueX, float valueY, float valueZ){
  myDataFile = SPIFFS.open(filename, "a");        // Open a file for reading and writing (appending)
  myDataFile.print("Shock:");     // Write some data to it (26-characters)
  myDataFile.print(magshock);     // Write some data to it (26-characters)
  myDataFile.print(" X:");     // Write some data to it (26-characters)
  myDataFile.print(valueX);     // Write some data to it (26-characters)
  myDataFile.print(" Y:");     // Write some data to it (26-characters)
  myDataFile.print(valueY);     // Write some data to it (26-characters)
  myDataFile.print(" Z:");     // Write some data to it (26-characters)
  myDataFile.println(valueZ);     // Write some data to it (26-characters)
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
  SPIFFS.begin();
  if (SPIFFS.exists(filename)) SPIFFS.remove(filename); // First in this example check to see if a file already exists, if so delete it
  if (!myDataFile)Serial.println("file open failed");   // Check for errors 
}
void setupMPU(){
  Wire.begin(4,5);
  Wire.beginTransmission(0b1101000); //This is the I2C address of the MPU (b1101000/b1101001 for AC0 low/high datasheet sec. 9.2)
  Wire.write(0x6B); //Accessing the register 6B - Power Management (Sec. 4.28)
  Wire.write(0b00000000); //Setting SLEEP register to 0. (Required; see Note on p. 9)
  Wire.endTransmission();  
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x1C); //Accessing the register 1C - Acccelerometer Configuration (Sec. 4.5) 
  Wire.write(0b00011000); //Setting the accel to +/- 16g
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
  LED_OFF();
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
  client.add(ShockMagnitude,AccMag);
  client.add(AccelerometerX,gForceX);
  client.add(AccelerometerY,gForceY);
  client.add(AccelerometerZ,gForceZ);
  client.sendAll();
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

void processAccelData(){
  gForceX = accelX/2048.0;
  gForceY = accelY/2048.0; 
  gForceZ = accelZ/2048.0; 
 if(gForceZ>0){
  gForceZ=gForceZ/(1.5);
  }
  else{
  gForceZ=gForceZ*2.0; 
  }
     AccMag= sqrt( sq(gForceX) + sq(gForceY)+ sq(gForceZ) );
}

void printData() {
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

  part1 = strLine.substring(strLine.indexOf("Shock:")+ 6, strLine.indexOf(" X:")); //mag
  part2 = strLine.substring(strLine.indexOf(" X:") + 3, strLine.indexOf(" Y:")); //X
  part3 = strLine.substring(strLine.indexOf(" Y:") +3, strLine.indexOf(" Z:")); //Y
  part4 = strLine.substring(strLine.indexOf(" Z:") + 3); //Z

 Serial.println("****** Parsing:  ******"); 
 Serial.println(part1); 
 Serial.println(part2); 
 Serial.println(part3); 
 Serial.println(part4); 

  AccMag = part1.toFloat();
  gForceX = part2.toFloat();
  gForceY = part3.toFloat();
  gForceZ = part4.toFloat();
   sendToUbidots();

}

