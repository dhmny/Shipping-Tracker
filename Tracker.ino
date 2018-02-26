#include <Wire.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define TSLAddr 0x39
#define MPUAddr 0x68

void setup() {

  Wire.begin(4,5);
  Serial.begin(115200);

  Wire.beginTransmission(TSLAddr);  // Configure TSL
  Wire.write(0x00 | 0x80);          // Select control register
  Wire.write(0x03);                 // Power ON mode
  Wire.endTransmission();           

  Wire.beginTransmission(TSLAddr);  // Starts I2C communication
  Wire.write(0x01 | 0x80);          // Select timing register
  Wire.write(0x02);                 // Nominal integration time = 402ms
  Wire.endTransmission();

  
  Wire.beginTransmission(MPUAddr);     // Configure MPU
  Wire.write(0x1B);                 // Select gyroscope configuration register
  Wire.write(0x18);                 // Full scale range = 2000 dps
  Wire.endTransmission();
  
  Wire.beginTransmission(MPUAddr);     
  Wire.write(0x1C);                 // Select accelerometer configuration register
  Wire.write(0x18);                 // Full scale range = +/-16g
  Wire.endTransmission();
  
  Wire.beginTransmission(MPUAddr);
  Wire.write(0x6B);                 // Select power management register
  Wire.write(0x01);                 // PLL with xGyro reference
  Wire.endTransmission();
  delay(300);

}

void loop() {

  //Read TSL
  unsigned int data[4];
  for(int i = 0; i < 4; i++)
  {
    Wire.beginTransmission(TSLAddr);
    Wire.write((140 + i));          // Select data register
    Wire.endTransmission();
    
    Wire.requestFrom(TSLAddr, 1);   // Request 1 byte of data
    if(Wire.available() == 1)       // Read 1 bytes of data
    {
      data[i] = Wire.read();
     }
     delay(500);
  }
  
  // Convert the data
  double ch0 = ((data[1] & 0xFF) * 256) + (data[0] & 0xFF);
  double ch1 = ((data[3] & 0xFF) * 256) + (data[2] & 0xFF);

  // Output data to serial monitor
  Serial.print("Full Spectrum(IR + Visible) :");
  Serial.println(ch0);
  Serial.print("Infrared Value :");
  Serial.println(ch1);
  Serial.print("Visible Value :");
  Serial.println(ch0-ch1);

  //Read MPU
  unsigned int data[6];

  Wire.beginTransmission(MPUAddr);
  Wire.write(0x3B);               // Select data register
  Wire.endTransmission();
  
  Wire.requestFrom(MPUAddr, 6);      // Request 6 bytes of data
  
  if(Wire.available() == 6)       // Read 6 byte of data 
  {
    data[0] = Wire.read();
    data[1] = Wire.read();
    data[2] = Wire.read();
    data[3] = Wire.read();
    data[4] = Wire.read();
    data[5] = Wire.read(); 
  }
  
  // Convert the data
  int xAccl = data[0] * 256 + data[1];
  int yAccl = data[2] * 256 + data[3];
  int zAccl = data[4] * 256 + data[5];

  Wire.beginTransmission(MPUAddr);
  Wire.write(0x43);               // Select data register
  Wire.endTransmission();
  
  Wire.requestFrom(MPUAddr, 6);      // Request 6 bytes of datav
  
  if(Wire.available() == 6)       // Read 6 byte of data 
  {
    data[0] = Wire.read();
    data[1] = Wire.read();
    data[2] = Wire.read();
    data[3] = Wire.read();
    data[4] = Wire.read();
    data[5] = Wire.read(); 
  }
  // Convert the data
  int xGyro = data[0] * 256 + data[1];
  int yGyro = data[2] * 256 + data[3];
  int zGyro = data[4] * 256 + data[5];

  // Output data to serial monitor
  Serial.print("Acceleration in X-Axis : ");
  Serial.println(xAccl);
  Serial.print("Acceleration in Y-Axis : ");
  Serial.println(yAccl);
  Serial.print("Acceleration in Z-Axis : ");
  Serial.println(zAccl);
  Serial.print("X-Axis of Rotation : ");
  Serial.println(xGyro);
  Serial.print("Y-Axis of Rotation : ");
  Serial.println(yGyro);
  Serial.print("Z-Axis of Rotation : ");
  Serial.println(zGyro);
  delay(500);

}
