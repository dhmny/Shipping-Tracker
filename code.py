import sys
import smbus
import time
from time import sleep

#DHT22
import Adafruit_DHT
#MPU6050
from mpu6050 import mpu6050
#Ubidots
from ubidots import ApiClient
#API
api = ApiClient("A1E-3e0bc2ffb2c8d2d94c17fca7c4cb270d97c9")
#Variable IDs
Temperature= api.get_variable("5a0504f3c03f97753fa034b3")
AccelerometerX= api.get_variable("5a04e7b8c03f9756221b7046")
AccelerometerY= api.get_variable("5a0894edc03f975cd51eaabc")
AccelerometerZ= api.get_variable("5a0894ffc03f975cd51eaabf")
GyroscopeX= api.get_variable("5a0504e6c03f977503375ff2")
GyroscopeY= api.get_variable("5a089515c03f975e2b174006")
GyroscopeZ= api.get_variable("5a08953fc03f975df44658a2")
Humidity= api.get_variable("5a05fa6dc03f971c57aae5f3")
light = api.get_variable("5a11cf56c03f976ce011db8f")

#Sensor data code
sensor= mpu6050(0x68)
print " waiting for the sensor to callibrate..."
sleep(2)
#dht22
sensor = 22
pin = 22

while True:
#Here is where you usually put the code to capture the data, either through your GPIO pins or as a calculation. We'll simply $
# Get I2C bus
 bus = smbus.SMBus(1)
 temp           = sensor.get_temp()
 humidity, temperature = Adafruit_DHT.read_retry(sensor, pin)
# TSL2561 address, 0x39(57)
# Select control register, 0x00(00) with command register, 0x80(128)
#        0x03(03)    Power ON mode
 bus.write_byte_data(0x39, 0x00 | 0x80, 0x03)
# TSL2561 address, 0x39(57)
# Select timing register, 0x01(01) with command register, 0x80(128)
#        0x02(02)    Nominal integration time = 402ms
 bus.write_byte_data(0x39, 0x01 | 0x80, 0x02)
 
# MPU-6000 address, 0x68(104)
# Select gyroscope configuration register, 0x1B(27)
#               0x18(24)        Full scale range = 2000 dps
 bus.write_byte_data(0x68, 0x1B, 0x18)
# MPU-6000 address, 0x68(104)
# Select accelerometer configuration register, 0x1C(28)
#               0x18(24)        Full scale range = +/-16g
 bus.write_byte_data(0x68, 0x1C, 0x18)
# MPU-6000 address, 0x68(104)
# Select power management register1, 0x6B(107)
#               0x01(01)        PLL with xGyro reference
 bus.write_byte_data(0x68, 0x6B, 0x01)
 time.sleep(0.8)

# Read data back from 0x0C(12) with command register, 0x80(128), 2 bytes
# ch0 LSB, ch0 MSB
 data = bus.read_i2c_block_data(0x39, 0x0C | 0x80, 2)
    
    # Read data back from 0x0E(14) with command register, 0x80(128), 2 bytes
    # ch1 LSB, ch1 MSB
 data1 = bus.read_i2c_block_data(0x39, 0x0E | 0x80, 2)
    
    # Convert the data
 ch0 = data[1] * 256 + data[0]
 ch1 = data1[1] * 256 + data1[0]
 vis_light = ch1

# MPU-6000 address, 0x68(104)
# Read data back from 0x3B(59), 6 bytes
# Accelerometer X-Axis MSB, X-Axis LSB, Y-Axis MSB, Y-Axis LSB, Z-Axis MSB, Z-Axis LSB
 data = bus.read_i2c_block_data(0x68, 0x3B, 6)

# Convert the data
 xAccl = data[0] * 256 + data[1]
 if xAccl > 32767 :
        xAccl -= 65536

 yAccl = data[2] * 256 + data[3]
 if yAccl > 32767 :
        yAccl -= 65536

 zAccl = data[4] * 256 + data[5]
 if zAccl > 32767 :
        zAccl -= 65536

# MPU-6000 address, 0x68(104)
# Read data back from 0x43(67), 6 bytes
# Gyrometer X-Axis MSB, X-Axis LSB, Y-Axis MSB, Y-Axis LSB, Z-Axis MSB, Z-Axis LSB
 data = bus.read_i2c_block_data(0x68, 0x43, 6)
 
 # Convert the data
 xGyro = data[0] * 256 + data[1]
 if xGyro > 32767 :
        xGyro -= 65536

 yGyro = data[2] * 256 + data[3]
 if yGyro > 32767 :
        yGyro -= 65536

 zGyro = data[4] * 256 + data[5]
 if zGyro > 32767 :
        zGyro -= 65536

 try:
     #Temperature.save_value({'value':temp}) MPU6050 temprature sensor
  Temperature.save_value({'value':temperature})
  Humidity.save_value({'value':humidity})
  AccelerometerX.save_value({'value':xAccl})
  AccelerometerY.save_value({'value':yAccl})
  AccelerometerZ.save_value({'value':zAccl})
  GyroscopeX.save_value({'value':xGyro})
  GyroscopeY.save_value({'value':yGyro})
  GyroscopeZ.save_value({'value':zGyro})
  light.save_value({'value':vis_light})
  
 except ValueError:
  print "No JSON received" 

# Output data to screen
 print "-----------------------------------"
 print "Acceleration in X-Axis : %d" %xAccl
 print "Acceleration in Y-Axis : %d" %yAccl
 print "Acceleration in Z-Axis : %d" %zAccl
 print "-----------------------------------"
 print "X-Axis of Rotation : %d" %xGyro
 print "Y-Axis of Rotation : %d" %yGyro
 print "Z-Axis of Rotation : %d" %zGyro
 print "-----------------------------------"
 print('Temp={0:0.1f}*  Humidity={1:0.1f}%'.format(temperature, humidity))
 print "-----------------------------------"
 print "Full Spectrum(IR + Visible) :%d lux" %ch0
 print "Infrared Value :%d lux" %ch1
 print "Visible Value :%d lux" %(ch0 - ch1)
 print "                                   "
 
