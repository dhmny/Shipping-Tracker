


#include <Wire.h>
#include <ESP8266WiFi.h>
#include <UbidotsMicroESP8266.h>


int16_t accelX, accelY, accelZ;
float gForceX, gForceY, gForceZ, AccMag;

#define TOKEN "A1E-QrVm0RfSxsS8QSKFRT6BcdRgWsJaBN"
#define AccelerometerX "5a90e2e7c03f971788be2913" // Replace it with your Ubidots' variable ID
#define AccelerometerY "5a91e062c03f9733092222c9"
#define AccelerometerZ "5a91e069c03f9733092222cb"
#define ShockMagnitude "5a91f6fec03f97492ca09019"
const char* ssid     = "SABIC";         // The SSID (name) of the Wi-Fi network you want to connect to
const char* password = "11223344";     // The password of the Wi-Fi network


 Ubidots client(TOKEN);
void setup() {
  Serial.begin(115200);
  delay(200);
   WiFi.begin(ssid, password);             // Connect to the network
  Serial.print("Connecting to ");
  Serial.print(ssid); Serial.println(" ...");

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(1000);
    Serial.print(++i); Serial.print(' ');
  }

  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());         // Send the IP address of the ESP8266 to the computer
  Wire.begin(4,5);
  setupMPU();
}


void loop() {
  recordAccelRegisters();
  printData();
  delay(1000);
}

void setupMPU(){
  Wire.beginTransmission(0b1101000); //This is the I2C address of the MPU (b1101000/b1101001 for AC0 low/high datasheet sec. 9.2)
  Wire.write(0x6B); //Accessing the register 6B - Power Management (Sec. 4.28)
  Wire.write(0b00000000); //Setting SLEEP register to 0. (Required; see Note on p. 9)
  Wire.endTransmission();  
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x1C); //Accessing the register 1C - Acccelerometer Configuration (Sec. 4.5) 
  Wire.write(0b00011000); //Setting the accel to +/- 16g
  Wire.endTransmission(); 
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
  AccMag= sqrt( sq(gForceX) + sq(gForceY)+ sq(gForceZ) );
}



void printData() {
  Serial.print("Accelerometer:  ");
  //-------------------------------------
  Serial.print(" Shock Magnitude= ");
  Serial.print(AccMag, 4);
  client.add(ShockMagnitude,AccMag);
  //-------------------------------------
  Serial.print(" || X=");
  Serial.print(gForceX);
  client.add(AccelerometerX,gForceX);
  //-------------------------------------
  Serial.print("|| Y=");
  Serial.print(gForceY); 
  client.add(AccelerometerY,gForceY);
  //-------------------------------------
  if(gForceZ>0){
  Serial.print("|| Z=");
  Serial.println(gForceZ/(1.5));
 client.add(AccelerometerZ,gForceZ/(1.5));
}
  else{
  Serial.print("|| Z=");
  Serial.println(gForceZ*2.0); 
  client.add(AccelerometerZ,gForceZ*2.0);
  }
  if(gForceZ/(1.5)>1) Serial.println("** Box is placed on normal position **"); 
  else if(gForceZ*2.0<-0.9) Serial.println("** Box is placed upside-down **");
  else if( (gForceX> 0.9) || (gForceX< -0.9) || (gForceY> 0.9) || (gForceY< -0.9) ) Serial.println("** Box is placed on side **");
  else Serial.println("** Box is tilted **");

  if(AccMag<2) Serial.println("** Box is stationary **"); 
  else if( (AccMag>2) && (AccMag<3)) Serial.println("** Box is being shaked lightly **");
  else if( (AccMag>3) && (AccMag<5)) Serial.println("** Box is being shaked moderately **");
  else if( (AccMag>5) && (AccMag<20)) Serial.println("** Box is being shaked extremely **");
  else ;
  client.sendAll();



Serial.println("------------------------------");
  


}
