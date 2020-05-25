
/* S4 PocketQube payload software
 *  June 2019
 *  Ken Biba/AeroPac, Inc.
 *  v 1.0 for S4Qube, S4Egg - baseline payload code
 *  For use with ItsyBitsy M4 Express, Sensor Board (GPS, IMU, TVOC+eCO2+BME280, spectrometer, light, radiation, dust), 
 *  Ground Imaging Board (visible, thermal,  ground facing spectrometer)

S4Qube PocketQube science payload.   12 core sensors  many optional sensors, LoRa telemetry, LEO minded but really for sounding rockets and HAB

S4Egg - subset of baseline air quality (pressure, humidity, temperature, TVOC, eCO2) sensors with options for more via QWIIC connections

 
This configuration for ItstyBitsy M4 Express for S4Qube and S4Egg

S4 Interfaces

  Serial port    VCC, TX, RX, GND .  (QWIIC)
  I2C            VCC, SDA, SCL, GND . (QWIIC)
  SPI            LoRa radio, SPI flash
  IO (A0, A3)    VCC, A3, A0, GND     (QWIIC)

S4 Bus

  VCC, GND, SDA, SCL, A0, A3 Tx, Rx, D5, 
 

Pins Used:

  3v - power
  gnd 
  #0/RXD
  #1/TXD
  SDA/SCL
  D5 . - burn wire
  A1 - battery level
  A0/A3 .  - IO
  SCK/MOSI/MISO   - SPI
  12/11/10 - LoRA radio CS/IRQ/Reset
  Builtin LED on 13
  New LED on D2 for GPS lock
  D9 - 1-Wire bus

Free Pins:
  D3/D4 for new I2C bus
  

Issues:

Watchdog timer to deal with processor stalls - interrupt issues?  Update watchdog for SAMD51

Core sensors:

*Supports LoRa telemetry (SPI CS pin 10, INT 6, RST 11)
*Supports GPS MediaTek   (QWIIC)
*Supports LSM9DS1 ST 9DOF inertial sensor  (I2C 0x6B, 0x1E)
*Supports Intersema MS5611 altitude/baro/temperature sensor  (I2C 0xEE/0x77)
*Supports multiple memory - internal SPI Flash, external SPI Flash, QWIIC OpenLog
*Supports air quality sensors

ToDo:   

*Store binary data but diagnostic data is text
Add dust sensor - thru serial port (Create new library - model on TinyGPS being fed off serial port)
*Update to new QWIIC GPS - test
Update to SPI flash storage + addtional SPI flash storage (store binary)
Update to (optional) QWIIC SD OpenLog additional storage as alternative
Update to VEML6075
Update to IR Array (Sparkfun library)
Add spectrometer AMX7265 libary (Tindie?)
Keep lightning sensor as optional QWIIC sensor with Sparkfun library - no interrupt needed - just poll
Update 1-wire - test
Add .jpeg serial camera (Adafruit library)
Simple beacon LoRa protocol

Burn-wire placeholder interface on D5 with A2 microswitch detect

Download data:
  Flight data - 10 Hz? 5 Hz?
  Image data - 1 Hz?
  Dust data - 1 Hz?
  Spectrum data?
  Beacon telemetry
  Sensor telemetry (optional)

Implemented: * = tested

*MCP9808 I2C external temperature sensor  (I2C address 0x18)
*CCS811 CO2 TVOC sensor (0x5A) air quality sensor I2C
*VEML6075 UV I2C ight sensor (0x10) https://www.tindie.com/products/onehorse/veml6070-uv-light-sensor/
*TSL2591 IR+visible I2C (0x29) light sensor https://www.adafruit.com/products/1980
MLX90614 IR I2C (0x5A) temperature sensor https://www.adafruit.com/products/1747  5-90 degree FOV
*Bosch BME280 pressure, temperature, humidity sensor (0x76)
*1-wire temperature sensors - via d/a port = up to two, sampled at ~ 1 Hz 
*Radiation sensor - IO port

Done but not thoroughtly tested:

Think about jumpers:  1 for diagnostic mode, 1 for short/long range mode - in v2.2 Sensor Board - add software v2.3 revision
Think about two-phase protocol - different speeds, different ranges - one for just GPS, one for complete telemetry. - needs testing


Payload To-Do:

Think about state LEDs - particularly one for GPS lock and telemetry send - future board

Think about added LBT function to allow multiple senders on channel - does it work? 


*/
#define PQ_VERSION  1

// Define configuration

enum S4Config {
  Egg,                                        // Egg configuration
  Qube,                                       // Basic 1P configuration
  QubePlus,                                   // Exgtended 1P configuration with dust sensor and radiation sensor
  Qube2,                                      // Fantasized 2P configuration
  Ground,                                      // Ground station
  Custom
};

S4Config S4 = Qube;                                   // Qube, Ground, Egg, QubePlus

bool    TELEMETRYMODE = true;                         // Telemetry mode: true = track mode, false = sensor mode
bool    DIAGNOSTIC    = true;                         // Enable diagnostic on the console port
bool    TELEMETRY     = false;                        // Enable telemetry via LoRa = true
bool    LOG           = true;                        // Enable local recording = true
#define TIMECHECK     false
#define FLOWENABLE    false                           // Secondary dianostic
#define WATCHDOG      false

bool    enableTrigger = true;                        // Trigger to enable recording sensor data
#define baroTrigger   false                           // Trigger is change in baro altitude after power up
#define baroLevel     100                             // altitude difference after power up
#define accelTrigger  false                            // Trigger is change in axial acceleration after power up
#define accelLevel    3                              // Axial acceleration above gravity to declare trigger.  -1 is normal when
#define deployTrigger false                           // Trigger is light detect of deployment after power up
#define lightLevel    10                              // Level of light to declare trigger

// Define configured devices

bool    GPS           = true;                           // MediaTech QWIIC GPS sensor - I2C
bool    IMU           = true;                           // LSM9DS1 9DOF inertial sensor - I2C
bool    MS5611sensor  = true;                           // Intersema baro sensor - I2C
bool    uLog          = false;                         // Local microSD storage - QWIIC
bool    iLog          = true;                          // M4 Express 2MB QSPI internal flash
bool    eLog          = false;                          // External SPI 16 MB Flash on D7
bool    dLog          = true;                          // display log on console
bool    LORA          = false;                          // LoRa telemetry radio - SPI
bool    DUST          = false;                          // Dust sensor  - serial
bool    RAD           = false;                          // Beta/Gamma/X-Ray radiation spectrometer - IO port
bool    ONEWIRE       = false;                          // One-Wire temperature sensors - D9
bool    TSL2591       = false;                          // Visible + IR sensor - I2C
bool    VEML6070      = true;                          // UV sensor - I2C
bool    VEML6075      = false;                          // Advanced UV sensor - I2C
bool    BMEPres       = true;                           // Pressure, temp, humidity - I2C .  BME280
bool    CCS811sensor  = true;                           // eCO2, TVOC - I2C
bool    BME680        = false;                          // Pressure, temp, humidity, TVOC - I2C
bool    MLX90614      = false;                          // Remote IR temperature - I2C
bool    MCP9808       = false;                          // Remote temperature - I2C
bool    MLX90640      = false;                          // IR imaging sensor
bool    VC0706        = false;                          // Serial camera
bool    AS7265        = false;                           // 410-940 nm 18 channel spectrometer
bool    AS3935        = false;                          // Lightning sensor
bool    Display       = false;                          // Ground Station display
bool    Aerosol       = false;                          // Synthetic I2C aerosol spectrometer - dust + radiation


const int numSensors = 24;
const struct {
    uint8_t address;
    String  name;
    bool    *state;
  } sensors[numSensors] = {
    {0x03,  "AS3935 lightning sensor", &AS3935},
    {0x10,  "QWIIC Mediatek GPS sensor", &GPS},
//    {0x10,  "VEML6075 UV light sensor", &VEML6075},                 // I2C conflict with Qwiic GPS
    {0x18,  "MCP9808 temperature sensor",&MCP9808},
    {0x1E,  "LSM9DS1 9DOF IMU",&IMU},
    {0x2a,  "QWIIC uSD OpenLog storage", &uLog},
    {0x29,  "TSL2591 IR+visible light sensor", &TSL2591},
    {0x33,  "MLX90640 IR Array", &MLX90640},
    {0x38,  "VEML6070 UV Sensor", &VEML6070},
    {0x39,  "VEML6070 UV Sensor", &VEML6070},
    {0x3D,  "QWIIC Micro OLED display", &Display},
    {0x49,  "AS7265 18 channel 410-940nm spectrometer", &AS7265},
    {0x5A,  "CCS811 CO2, TVOC sensor", &CCS811sensor},
//    {0x5A,  "MLX90614 IR sensor", &MLX90614},
    {0x6B,  "LSM9DS1 9DOF IMU", &IMU},
    {0x76,  "BME280 pressure, temperature, humidity sensor", &BMEPres},
//  {0x76,  "BME680, pressure, temperature, humidity, TVOC sensor", &BME680},
    {0x77,  "MS5611 pressure, temperature sensor", &MS5611sensor},
  };


// Add libraries
#include "PQ_Payload.h"                           // Configuration
#include <Wire.h>
#include <SPI.h>                                  // Standard SPI library
#include <RH_RF95.h>                              // RadioHead RF95 Library
#include <RHSPIDriver.h>
#include <RHGenericDriver.h>
#include <TinyGPS++.h>                            // GPS decoder library
#include <SparkFun_I2C_GPS_Arduino_Library.h>     // QWIIC Mediatek high altitude GPS
#include <Adafruit_SPIFlash.h>                    // SPI Flash library (internal AND external?)
#include <Adafruit_SPIFlash_FatFs.h>
#include <SerialFlash.h>                          // Mission memory
#include <SparkFun_Qwiic_OpenLog_Arduino_Library.h> // Backup QWIIC SD card storage
#include <MS5611.h>                               // MS5611 baro sensor library
#include <SparkFunLSM9DS1.h>                      // 9DOF sensor
#include <Adafruit_TSL2591.h>                     // Visible and near IR light sensor
#include <SparkFunBME280.h>                       // Low altitude pressure, temperature and humidity sensor
#include <SparkFunCCS811.h>                       // TVOC and eCO2 sensor
#include <Adafruit_VEML6075.h>                    // UV light sensor
#include <Adafruit_VEML6070.h>                    // UV light sensor
#include <Adafruit_MCP9808.h>                     // Temperature sensor
#include <Adafruit_MLX90614.h>                    // Temperature sensor
#include "AS7265X.h"
//#include <utility/imumaths.h>
#include <OneWire.h>
#include <DallasTemperature.h>                    // 1-Wire temperature sensor
#include <Adafruit_SleepyDog.h>
//#include <Adafruit_VC0706.h>                      // Serial camera library
//#include <SparkFun_AS3935.h>
#include <Adafruit_DotStar.h>
#include <TimeLib.h>                              // Unix timestamp Library
// IR Imager 

// Dust library

// Define configuration

#define RH_HAVE_SERIAL true
#define CONSOLESPEED  115200
#define GPSSPEED          9600
#define DUMPREGISTERS    false                 // Dump modem registers
#define DECLINATION   14.5

#define DEFAULTFREQUENCY   12                 // Index to default operations frequency for the radio
#define MINFREQUENCY      903.0
#define DEFAULTPOWER      20                  // Transmit power in dBm for the radio
#define DEFAULTLORAMODE   16                  // Default LoRa mode for full sensor transmission ~ 10 Kb/s
#define TRACKLORAMODE     10                  // LoRa mode for tracking data transmission ~ 2.2 kB/s
#define iLOGTIME          500                 // Internal sensor logging interval (msec)
#define eLOGTIME          500                 // External sensor logging interval (msec)
#define uLOGTIME          1000                // uSD card logging interval (msec)
#define dLOGTIME          1000                // console display logging interval (msec)
#define TELEMETRYTIME     2000                // msec between telemetry packets
#define ONEWIRETIME       1000                // 1-Wire timeout
#define WATCHTIME         2000                // Watchdog timeout
#define WATCHRESET         200                // Time between watchdog resets
#define I2C_CLOCK         400000              // I2C clock speed Cortex-M4 is capable to 3400000
#define FEATHER           false
#define ITSYBITSY         true
#define CORTEXM0          false
#define CORTEXM4          true



uint8_t PAYLOAD    =      100;                // Payload address
uint8_t GATEWAY    =        0;                // Ground station address
#define BROADCAST         255
#define MAXFREQ           18


#define RFM95_CS 12                       
#define RFM95_RST 10
#define RFM95_INT 11

// Packet types contained in LoRa headerId in header
#define LOGOFF       10                   // Local log disabled
#define LOGON         9                   // Local log enabled
#define TELTRACK      8                   // Tracking telemetry
#define TELSNSR       7                   // Full sensor telemetry
#define TELOFF        6                   // No Telemetry
#define CHANGEMODE    5                   // Change radio network parameters
#define ACK           4                   // Response to PING, and CHANGEMODE
#define PING          3                   // Requests ACK from destination
#define TRACK         2                   // Basic GPS tracking packet
#define IMAGE         1                   // IR imaging payload
#define SENSOR        0                   // Full sensor payload (except for imaging)

unsigned long transmitTime = 0;


// indicator LEDs . - the color NeoPixel, the red builtin LED and the additional LED
//  Color NeoPixel - RED for board S4 failure on boot, GREEN for success on 
//  Red LED on processor - blinks with each saved sensor frame
//  LED on processor board - blinks with each transmitted frome

#define   VOLT               A1                 // Battery voltage
#define   LED                LED_BUILTIN        // Red LED on Itsybitsy M4 Express for packet status

#define   telemetryLED  2                       // LED for telemetry transmission/reception 

#define   numPixel  1                           // NeoPixel for system status in main loop
#define   clkPin    6
#define   datPin    8

// Options

#define OPTION0 A4
#define OPTION1 A5
#define OPTION2 A6

bool  Option0 = true;                         // Pin A4 - jumper ON = diagnostic mode
bool  Option1 = true;                         // Pin A5 - jumper ON = full sensor telemetry

bool TRACKMODE = TRACKLORAMODE;               // Tracking telemetry mode
bool SENSORMODE = DEFAULTLORAMODE;            // Full sensor telemetry mode

String logHeader = "time,msec,volt,";
const String radioHeader = "rssi,pwr,mode,freq,";
const String gpsHeader = "lat,lon,alt,sats,";
const String imuHeader = "ax,ay,az,gx,gy,gz,mx,my,mz,yaw,pitch,roll,head,";



// Packet format - 48 byte (max) frame
//     Destination address  - use from underlying header
//     Source address  - use from underlying header
//     Packet type:  encode in headerid from underlying header
//     Flags currently unused
//     Either track (short) payload or sensor (long) payload

struct gpsLocation {                          // From GPS
  double  lat;
  double  lon;
  float   alt;
  uint8_t sats;
  bool    lock;
};

struct dateTime {                             // From GPS
  unsigned long  unixTime;                    // Unix time stamp (seconds)
  unsigned long  milli;                       // From Arduino  (milliseconds)
};

struct attitude {                             // From LSM9DS1 sensor  
  float aX;
  float aY;
  float aZ;
  float gX;
  float gY;
  float gZ;
  float mX;
  float mY;
  float mZ;
  float yaw;
  float pitch;
  float roll;
  float heading;
};


struct radiation {
  uint16_t  events;                       // From First Sensor - cumulative counts since turn on
  uint16_t  noise;                        // Cumulative noise events
  int       pulseWidth;                   // Intensity of the latest pulse
  float     energy;                       // Estimated MeV of latest event
};

struct pms5003 {
    uint16_t  pm10_standard, pm25_standard, pm100_standard;
    uint16_t  pm10_env, pm25_env, pm100_env;
    uint16_t  particles_03um, particles_05um, particles_10um, particles_25um, particles_50um, particles_100um;
};

struct pms5003frame {
  uint16_t  framelen;
  pms5003   data;
  uint16_t  unused;
  uint16_t  checksum;
};

struct airQuality {
  pms5003 dust;                             // From PMS5003 dust sensor.   Current dust measurement
  int   voc;                                // From CS811 sensor.   Current measurement
  int   co2;                                // From CS811 sensor. Current measurement.
  float humidity;                           // From BME280 sensor.   Current measurement.
  float temperature;                        // From BME280 sensor.   Current measurement.
  float pressure;                           // From BME280 sensor.   Current measurement.
  float baroAlt;                            // From BME280 sensor.
  struct {
    bool  strike;
    uint8_t distance;
  } lightning;                              // from AS3935 sensor
};


struct light {
  uint16_t  uva;                            // From VEML6095/VEML6070 sensor.   Current measurement.  *5 to get radiant power
  uint16_t  uvb;
  uint16_t  ir;                             //  From TSL2591 sensor
  uint16_t  visible;                        //  From TSL2591 sensor
  float     spectrumData[18];               //  From AS7265  sensors
};

struct extTemp {                            // Can also use for 1-wire temperature sensors if configured
  float temp1;                              //  MCP9808 temp sensor
  float temp2;                              //  MLX9614 temp IR sensor
  float temp1Wire[2];                       // 1 Wire temperature sensors
};


// Multspectral image of .jpg, ir and spectra
// Only need enough resolution in .jpg to align image with sat image for feature recognition

struct irImage {
  float thermister;
  float temperature[768];
};


struct LoRa {
  uint8_t mode    = DEFAULTLORAMODE;
  uint8_t txPower = DEFAULTPOWER;
  uint8_t freq    = DEFAULTFREQUENCY;
};

struct irPacket {
  uint8_t   version;
  uint8_t   voltage;                        // Last measured payload battery voltage in tenths of volt
  LoRa      lora;
  uint8_t   txSeq = 0;                      // Currentframe sequence number
  long      timeStamp;
  gpsLocation loc;
  irImage ir;
} irPacket;


struct trackPacket {
  uint8_t   version;
  uint8_t   voltage;                        // Last measured payload battery voltage in tenths of volt
  LoRa      lora;
  uint8_t   txSeq = 0;                       // Currentframe sequence number
  long      timeStamp;
  gpsLocation loc;
} trackPacket;


struct sensorPacket {
  uint8_t   version;
  uint8_t   voltage;                          // Last measured payload battery voltage in tenths of volt
  LoRa      lora;
  uint8_t   txSeq = 0;                        // Currentframe sequence number
  long      timeStamp;
  gpsLocation loc;
  attitude pA;
  float pressure;
  float temperature;
  volatile radiation rad;
  airQuality air;
  light lite;
  extTemp tmp;
};

struct sensorBody {
  attitude pA;
  float pressure;
  float temperature;
  volatile radiation rad;
  airQuality air;
  light lite;
  extTemp tmp;
};


struct packetHeader {
  uint8_t   version;
  uint8_t   voltage;
  LoRa      lora;
  uint8_t   txSeq = 0;
  long      timeStamp;
  gpsLocation loc;
};

struct packet {
  packetHeader  header;
  union body {
    irImage ir;
    sensorBody  sensor;
  } body;
};

sensorPacket sndPacket;                        // Buffer for packet to send
packet rcvPacket;                        // Buffer for received packets


struct sensorRecord {
  uint8_t       version;
  uint8_t       voltage;
  gpsLocation   location;
  attitude      att;
  dateTime      dt;
  float         pressure;
  float         temperature;
  airQuality    air;
  radiation     rad;
  light         lite;
  extTemp       temp;
} sensorRecord;

// flashImageStore

struct state {                                // Payload state
  uint8_t       type;                         // current command type from ground station
  uint8_t       lastSource;                   // Address of last received packet
  uint8_t       lastDestination;              // Address of last sent packet
  bool          setup = false;
  dateTime      dt;
  int           rssi;
  int           sigQual;
  float         voltage;                      // Payload voltage
  uint8_t       txSeq;                        // Transmit sequence number
  uint8_t       rxSeq;                        // Received sequence number
  unsigned long  rxCount;                    // Count of received packets
  uint8_t       rxBad = 0;                   // Cound of ground station bad packets received
  LoRa          lora;
  int           lastRSSI;                     // Last payload reported RSSI
  int           lastSNR;                      // Last payload reported SNR
  gpsLocation   loc;
  gpsLocation   ground;                       // Ground station location when used 
  long          bearing;                      // Bearing from ground station to S4
  long          elevation;                    // Above horizon from ground station to S4
  long          distance;                     // From gound station to S4
  long          course;
  bool          sensordata  = false;          // Flag for ground station denoting at least one full sensor packet received
  attitude      sA;
  float         pressure;
  float         temperature;
  float         baroAlt;
  radiation     rad;
  uint16_t      radSpectrum[512];
  airQuality    air;
  light         lite;
  extTemp       tmp;
  irImage       ir;
  long          lastTime      = 0;
  long          lastReceived  = 0;
  long          lastTelemetry = 0;
  long          lastiLog       = 0;
  long          lasteLog       = 0;
  long          lastuLog       = 0;
  long          lastdLog       = 0;
  long          lastOneWire   = 0;
  long          lastWatchdog  = 0;
}  state;



// Singleton instance of the LoRa driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// Table of LoRa modes - define 22 with 4 being the ones RadioHead has defined
// Four of these are unusable due to inaccurate crystals

static const RH_RF95::ModemConfig modeTable[]  =
{
    //  1d,     1e,      26
    { 0x42,   0xc4,    0x08},       // 0 Bw31_25Cr45Sf4096              163.1,              NO
    { 0x62,   0xc4,    0x08},       // 1 Bw62_5Cr45Sf4096               146.5,  -20   .15
    { 0x48,   0x94,    0x08},       // 2 Bw31_25Cr48Sf512               155.6,   -12.5 .36
    { 0x82,   0xc4,    0x08},       // 3 Bw250Cr45Sf4096                154,   -20 .  59
    { 0x72,   0xc4,    0x08},       // 4 Bw125Cr45Sf4096                154,   -20   .29
    { 0x78,   0xc4,    0x08},       // 5 Bw125Cr48Sf4096                154,   -20   .18
    { 0x62,   0x94,    0x00},       // 6 Bw62_5Cr45Sf512                152,   -12.5 1.2
    { 0x92,   0xc4,    0x00},       // 7 Bw500Cr45Sf4096                151,  -20   1.2
    { 0x82,   0xa4,    0x00},       // 8 Bw250Cr45Sf1024                149,  -15   2.3
    { 0x72,   0x94,    0x00},       // 9 Bw125Cr45Sf512                 149, -12.5 2.3
    { 0x92,   0xb4,    0x00},       // 10 Bw500Cr45Sf2048               148.4,   -17.5 2.3
    { 0x92,   0xa4,    0x00},       // 11 Bw500Cr45Sf1024                146,   -15   4.7
    { 0x82,   0x94,    0x00},       // 12 Bw250Cr45Sf512                 146,   -12.5 4.7
    { 0x72,   0x84,    0x00},       // 13 Bw125Cr45Sf256                 146,   -10   4.7
    { 0x62,   0x74,    0x00},       // 14 Bw62_5Cr45Sf128                146,   -7.5  4.7
    { 0x42,   0x64,    0x00},       // 15 Bw31_25Cr45Sf64                 145.4
    { 0x72,   0x74,    0x00},       // 16 Bw125Cr45Sf128 (the chip default) 143, -7.5  9.4
    { 0x92,   0x74,    0x00},       // 17 Bw500Cr45Sf128                  137,  -7.5 37.5
    { 0x42,   0xb4,    0x08},       // 18 Bw31_25Cr45Sf2048              160.8              NO
    { 0x42,   0xa4,    0x08},       // 19 Bw31_25C445Sf1024               158.4
    { 0x62,   0xb4,    0x08},       // 20 Bw62_5Cr45Sf2048                157.5             MAYBE
    { 0x62,   0xa4,    0x08},       // 21 Bw62_5Cr45Sf1024                155
    { 0x72,   0xb4,    0x08},       // 22 Bw125Cr45Sf2048                 154.5
    { 0x72,   0xa4,    0x00},       // 23 Bw125Cr45Sf1024                 152
    { 0x72,   0x74,    0x00},       // 24 Bw125Cr45Sf128                  143
    { 0x92,   0x94,    0x00},       // 25 Bw500Cr45Sf512                  143
    { 0x92,   0x84,    0x00},       // 26 Bw500Cr45Sf256                  140
    { 0x92,   0x74,    0x00},       // 27 Bw500Cr45Sf128                  137

};

#define MAXMODE 28

//  Four different logging methods
//    -- M4 internal flash storage (mostly used for S4Egg)
//    -- External SPI flash storage (mostly used for sensor data for S4Qube) . imaging data on separate flash
//    -- Console display display
//    -- Optional I2C configured SD card
//    -- can all be configured for parallel storage, with optionally configured snapshot times of sensors
//
//  Flash Storage Methods
//  Internal File
//    One binary file of fixed size records - sensorRecord
//    Keep adding to end of one file until full
//    Upload log file thru USB connection to host computer to Dashboard and then erase after successful upload
//    On S4Egg on internal flash (iLog), on S4Qube on external flash (eLog)
//    On S4Egg use SPIFlash library, on S4Qube use SerialFlash library . 
//    if booted with no usb connection to host, presume in flight mode and overwrite data file with new data
//    if USB connection to host - ask to download existing data as .csv to host, otherwise display new data as .csv to console

// For ItsyBitsy M4 use internal 2 MB QSPI flash for sensor storage
#define INTERNAL_FLASH     SPIFLASHTYPE_W25Q16BV  // Flash chip type.
#define EXTERNAL_FLASH     SPIFLASHTYPE_W25Q128


//#if defined(__SAMD51__)
  // Alternatively you can define and use non-SPI pins, QSPI isnt on a sercom
  Adafruit_SPIFlash iflash(PIN_QSPI_SCK, PIN_QSPI_IO1, PIN_QSPI_IO0, PIN_QSPI_CS);
//#else
//  #if (SPI_INTERFACES_COUNT == 1)
    #define FLASH_SS       9                    // Flash chip SS pin.
    #define FLASH_SPI_PORT SPI                   // What SPI port is Flash on?
//  #else
//    #define FLASH_SS       SS1                    // Flash chip SS pin.
//    #define FLASH_SPI_PORT SPI1                   // What SPI port is Flash on?
//  #endif
  
//Adafruit_SPIFlash iflash(FLASH_SS, &FLASH_SPI_PORT);     // Use hardware SPI
//#endif

Adafruit_W25Q16BV_FatFs ifatfs(iflash);




String    logFile = "snsrLog.dat";                      // Name of sensor file
String    rootDirectory = "/";                          // Root directory
uint16_t  recordPtr     = 0;                            // Pointer to next record
uint16_t  recordSize    = sizeof(sensorRecord);         // Record size
uint16_t  recordNum     = 0;                            // Number of sensor records
File      ilogFile;
File      elogFile;
#define   iSize            2000000
#define   eSize           16000000
uint16_t  irecordMax     = iSize/recordSize;           // Maximum number of internal flash records
uint16_t  erecordMax     = eSize/recordSize;          // Maximum number of external flash records

OpenLog   uSDlog;

// Opens log file for access, does not erase.

bool initLogs() {
  bool result = false;

  state.lastiLog = 0;                 // Time of last iLog
  state.lasteLog = 0;                 // Time of last eLog
  state.lastuLog = 0;                 // Time of last uLog
  state.lastdLog = 0;                 // Time of last dLog
  
  if (!LOG) {
    diagnostic("Logging not configured");
    return(false);
  }

  result |= initiLog();
//  result |= initeLog();
//  result |= inituLog();
  
  if (result) 
    diagnostic ("Success opening sensor log file"); 
  else 
    diagnostic ("Failure opening sensor log file");
  recordNum = 0;
  return (true);
}

// Initialize internal Flash log

bool initiLog() {

    if (iLog) {  
      if (!iflash.begin(INTERNAL_FLASH)) {
        diagnostic ("Internal flash error");
        diagnostic("Could not mount flash chip");
        return (false);
      }

      if (!ifatfs.begin()) {
        diagnostic("Failure to mount internal flash filesystem!");
        return(false);
      }
      
      diagnostic("Internal flash file system mounted");

      ilogFile = ifatfs.open(logFile, FILE_WRITE);
      if (!ilogFile) {
        diagnostic("Failure opening sensor log file for writing");
        Serial.print("Result is "); Serial.println(ilogFile);
        Serial.print("Log file is "); Serial.println(logFile);
        return (false);
      }
      diagnostic("Success opening log file for writing");
    }
    return(true);
}

// Initialize external Flash log

bool initeLog() {
      // Change this when external flash fixed

    if (eLog) {
      if (!iflash.begin(INTERNAL_FLASH)) {
        diagnostic ("Internal flash error");
        return (false);
      }
      ilogFile = ifatfs.open(logFile, FILE_WRITE);
  
      if (!ilogFile) {
        diagnostic("Error opening sensor log file");
        return (false);
      }
    }
    return(true);
}

bool inituLog() {

  if (uLog) {
    if (!uSDlog.begin()) return(false);
    return(true);
  }
  return(true);
}

// Erases sensor log files

bool eraseLogs() {
  bool state = false;
  
  if (!LOG) return (false);

  if (iLog) state |= eraseiLog();
  if (eLog) state |= eraseeLog();

  return(state);
}

bool eraseiLog() {
  if (iLog) {
      if (!ifatfs.remove(logFile)) return(false); else {
        diagnostic("Log file removed");
        return(true);
      }
      
  }
  return(false);
}

bool eraseeLog() {
  if (eLog) {
    if (!ifatfs.remove(logFile)) return(false); else {
      diagnostic("Log file removed");
      return(true);
    }
  }
  return (false);
}


// Read indicated sensor log record in external data structure sensorRecord

void readSensorLogs(uint16_t record) {

  if (!LOG) return;
  // Position to record (recordNum * size)

  readiLog(record);
  readeLog(record);
  
  return;
 
}

void readiLog(uint16_t record) {
  if (iLog) {
    ilogFile.seek(record*recordSize);
    // read the record indicated by record number into sensorRecord buffer
    ilogFile.read(&sensorRecord, sizeof(sensorRecord));
  }
}

void readeLog(uint16_t record) {
  if (eLog) {
    ilogFile.seek(record*recordSize);
    // read the record indicated by record number into sensorRecord buffer
    ilogFile.read(&sensorRecord, sizeof(sensorRecord));
  }
}

// Appends sensor record in sensorRecord to appropriate log file

#define flushcount  20
long bufcount = 0;

bool  writeiLog() {
  long bytes;
  String msg;

  if (!LOG) return (false);

  if (!iLog) return (false);

  // Append sensor record to current end of file
  // Writes until file full 

  if (recordNum < irecordMax) {
    bytes = ilogFile.write((char *)&sensorRecord, sizeof(sensorRecord));
//    msg = "Write iLog with record # " + String(recordNum) + " of " + String(bytes) + " bytes and bufcount " + String(bufcount);
//    diagnostic(msg);
    bufcount++;
    recordNum++;
    recordPtr = recordNum*sizeof(sensorRecord);
  }

// Every N records close file and reopen to make sure file is written
// N = 4k/sizeof(sensorRecord)  
  if (bufcount >= flushcount) {
    bufcount = 0;
    ilogFile.close();
    ilogFile = ifatfs.open(logFile, FILE_WRITE);
//    diagnostic("Log file flushed");
  }
  return(true);
  
}

bool  writeeLog() {

  if (!LOG || !eLog) return (false);

  // Append sensor record to current end of file
  // Writes until file full 

  if (recordNum < erecordMax) 
    elogFile.write((char *)&sensorRecord, sizeof(sensorRecord));

  recordNum++;
  recordPtr = recordNum*sizeof(sensorRecord);
  return(true);
  
}

//  Copy state vector to binary sensor save format

void  saveSensorState() {

  if (!LOG) return;
  sensorRecord.version = PQ_VERSION;
  sensorRecord.voltage = state.voltage;
  sensorRecord.location = state.loc;
  sensorRecord.att = state.sA;
  sensorRecord.dt  = state.dt;
  sensorRecord.pressure = state.pressure;
  sensorRecord.temperature = state.temperature;
  sensorRecord.air = state.air;
  sensorRecord.rad = state.rad;
  sensorRecord.lite = state.lite;
  sensorRecord.temp = state.tmp;
}

void retrieveSensorState() {
   state.voltage = sensorRecord.voltage;
   state.loc = sensorRecord.location;
   state.sA = sensorRecord.att;
   state.dt = sensorRecord.dt;
   state.pressure = sensorRecord.pressure;
   state.temperature = sensorRecord.temperature;
   state.air = sensorRecord.air;
   state.rad = sensorRecord.rad;
   state.lite = sensorRecord.lite;
   state.tmp = sensorRecord.temp;
}


/*    Flash light on S4 indicating connection and ready to upload
 *    Save max record ptr
 *    For each sensor record from 0 to recordNum
 *      Read each record
 *      Encode record as text, write as .csv to usb serial port
 *      Host acknowledges receipt by sending number of records and ACK character
 *      Flash a light on S4 indicating record upload success
*      Erase sensor file
*      recordNum = 0;
*      recordPtr = 0;
 */

bool  uploadLogs() {
  String msg;
  long i = 0, numRecords = 0, fileSize = 0;
  
  // Find log file on appropriate flash media
  // Find number of records by dividing length of file by record size
  // read thru all the records, format as .csv and send to serial port

  if (!LOG) {
    diagnostic("No logging");
    return(false);
  }
  if (iLog) {
    if (!iflash.begin(INTERNAL_FLASH)) {
      diagnostic("Internal flash error");
      return(false);
    }
    if (!ifatfs.begin()) {
      diagnostic("No internal flash filesytem");
      return(false);
    }
    if (!ifatfs.exists(logFile)) {
      diagnostic("No log file");
      return(false);
    }
    ilogFile = ifatfs.open(logFile, FILE_READ);
    fileSize = ilogFile.size();
    numRecords = fileSize/recordSize;
  } else if (eLog) {
    if (!ifatfs.exists(logFile)) return(false);
    ilogFile = ifatfs.open(logFile, FILE_READ);
    fileSize = ilogFile.size();
    numRecords = fileSize/recordSize;
  }

  // Output log file
  Serial.print("Log file is "); Serial.print(fileSize); Serial.print(" bytes and "); Serial.print(numRecords); Serial.println(" sensor records");
  diagnostic(logHeader);
  while (i < numRecords) {
      readSensorLogs(i);                       // read next binary sensor record from flash storage to sensorRecord buffer
      retrieveSensorState();                  // .  Reformat from the binary sensor record to state vector
      msg = createStateMsg();                 // .  Reformat as .csv from state vector
      Serial.println(msg);                    //    Output to USB
      i++;
  }

  if (ilogFile) ilogFile.close();
  return(true);
}


 
void writeLogs () {
  String msg;
  
  if (!LOG) return;

  if (iLog && (millis() - state.lastiLog) >= iLOGTIME) {
    // Blink LED
    LEDon();
    writeiLog();
    LEDoff();
    state.lastiLog = millis();
  }

  if (eLog && (millis() - state.lasteLog) >= eLOGTIME) {
    writeeLog();
    state.lasteLog = millis();
  }

  // Write to optional SD card storage on I2C
  
  if (uLog && (millis() - state.lastuLog) >= uLOGTIME) {
    msg = createStateMsg();
    uSDlog.println(msg);
    state.lastuLog = millis();
  }
  
  // Display on console for local logging

  if (dLog && (millis() - state.lastdLog) >= dLOGTIME) {
    msg = createStateMsg();
    Serial.println(msg);
    state.lastdLog = millis();
  }  

}


// LED methods

// On ItsyBitsy M4 Express all we have is single red LED
// constant RED = error
// three blink = good startup
// one blink = sent packet

#define BLINKDELAY 100         // blink delay in msec

bool initLED() {
  pinMode(LED, OUTPUT);
  return (true);
}

void LEDon() {
  digitalWrite(LED, HIGH);
}

void LEDoff() {
  digitalWrite(LED, LOW);
}

void LEDblink() {
  LEDon();
  delay(BLINKDELAY);
  LEDoff();
}

void LEDblink2() {
  LEDblink();
  delay(BLINKDELAY);
  LEDblink();
}

void LEDblink3() {
  LEDblink();
  delay(BLINKDELAY);
  LEDblink(); 
  delay(BLINKDELAY);
  LEDblink();
} 

Adafruit_DotStar clrLED = Adafruit_DotStar(numPixel, datPin, clkPin);

bool initCLED() {
  clrLED.begin();
  clrLED.setBrightness(64);
  clrLED.show();
}


  
void greenCLED() {
  clrLED.setPixelColor(0,255,0,0);
  clrLED.show();
}

void redCLED() {
  clrLED.setPixelColor(0,0,255,0);
  clrLED.show();
}

// GPS methods

I2CGPS myI2Cgps;

TinyGPSPlus gps;

//The following tells the TinyGPS library to scan for the PMTK001 sentence
//This sentence is the response to a configure command from the user
//Field 1 is the packet number, 2 indicates if configuration was successful
TinyGPSCustom configureCmd(gps, "PMTK001", 1); //Packet number
TinyGPSCustom configureFlag(gps, "PMTK001", 2); //Success/fail flag

bool initGPS() {

    diagnostic("init GPS ");
    
    if (!GPS) {
      diagnostic("GPS not configured");
      return(true);
    }
    
    if (myI2Cgps.begin() == false) {
      diagnostic("GPS not available");
      return(false);
    }

    unsigned long startTime = millis();

    String configString;
    
    configString = myI2Cgps.createMTKpacket(220, ",200");             // Enable 5 Hz updates
    myI2Cgps.sendMTKpacket(configString);
    configString = myI2Cgps.createMTKpacket(285, ",2,25");            // Pulse per second after 3D fix
    myI2Cgps.sendMTKpacket(configString);
    configString = myI2Cgps.createMTKpacket(314, ",0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0");
    myI2Cgps.sendMTKpacket(configString);   
    configString = myI2Cgps.createMTKpacket(886, ",3");               // Balloon mode
    myI2Cgps.sendMTKpacket(configString);
    
    do {
      while (myI2Cgps.available())
        gps.encode(myI2Cgps.read());
    } while (millis() - startTime < 5000);

    if (gps.charsProcessed() < 10) {
      diagnostic(String(gps.charsProcessed()));
      diagnostic("GPS not available");
      return(false);
    } else {
      diagnostic("GPS chars processed " + String(gps.charsProcessed()));
    }

    logHeader += gpsHeader;
    diagnostic("GPS configured and operating"); 
    return (true);
}


void getGPS() {

    // Process GPS strings, decode and store in state vector

    timeCheck("get GPS ");

    state.dt.milli = millis();
    if (!GPS) return;
    
    if (!myI2Cgps.available()) return;
    
    while (myI2Cgps.available()) {
      gps.encode(myI2Cgps.read());
    }
    
    if (gps.location.isValid() || S4 != Ground ) {
      state.loc.sats = gps.satellites.value();
      state.loc.lat = gps.location.lat();
      state.loc.lon = gps.location.lng();
      state.loc.sats = gps.satellites.value();
    } else if (gps.location.isValid() || S4 == Ground) {
      state.ground.sats = gps.satellites.value();
      state.ground.lat = gps.location.lat();
      state.ground.lon = gps.location.lng();
      state.ground.sats = gps.satellites.value();
    }
        
    if (state.loc.sats >= 4) state.loc.lock = true;
    else state.loc.lock = false;
    
    if (gps.altitude.isValid()) {
        state.loc.alt = gps.altitude.meters();
    }

    if (gps.date.isValid() || gps.time.isValid()) {
         setTime(gps.time.hour(),gps.time.minute(),gps.time.second(),gps.date.day(),gps.date.month(),gps.date.year());
         state.dt.unixTime = now();   
    }

}

String createGPSmsg(gpsLocation loc) {
  String msg = "";

  if (!GPS) return(msg);
  msg += String(loc.lat)+",";
  msg += String(loc.lon)+",";
  msg += String(loc.alt)+",";
  msg += String(loc.sats)+",";

  return (msg);
}

String createGPStime() {
  String msg = "";

  if (!GPS) return(msg);
  msg += String(state.dt.unixTime)+",";

  return(msg);
}

//  Output diagnostic string via USB port, and optional I2C display

void diagnostic (String msg) {

  if (DIAGNOSTIC) {
    Serial.print(millis()); 
    Serial.print(": "); 
    Serial.println(msg);

    // Add to small display if available
  }
}

// Create string with state information

String createStateMsg() {

  gpsLocation loc;

  loc = state.loc;

  String msg = "";
  
  msg += createGPStime();
  msg += String(state.dt.milli)+",";
  msg += String(state.voltage)+",";
  msg += createRadioMsg();
  msg += createGPSmsg(loc);
  msg += createIMUmsg();
  msg += createMS5611msg();
  msg += createBMEmsg();
  msg += createCCS811msg();
  // Dust
  // Light - spectrometer UV .  6070
  msg += createTSL2591msg();
  msg += createVEML6070msg();
  msg += createVEML6075msg();
  msg += createAS7265msg();
  msg += createMCP9808msg();
  msg += createMLX90614msg();
  msg += create1WireMsg();
  msg += createRadmsg();
  msg += msg.length();
  return(msg);
  
}

// Dump current state

void diagnosticState () {

  diagnostic(createStateMsg());
}


// LoRa telemetry methods


// Global telemetry method 

void sendTelemetry() {

  timeCheck("send Telemetry ");
 
  if ((millis() - state.lastTelemetry) >= TELEMETRYTIME) {
    if (TELEMETRY) {
      if (TELEMETRYMODE) sendPacket(GATEWAY, TRACK, 0);
      else sendPacket(GATEWAY, SENSOR, 0);
    }
    state.lastTelemetry = millis();
  }
}
 
// Set LoRa radio mode

bool setRadioMode(uint8_t mode, uint8_t power, uint8_t freq) {
  float frequency;
  
  mode = constrain(mode, 0, MAXMODE);
  power = constrain(power, 5, 23);
  freq = constrain(freq, 0, MAXFREQ);
  frequency = MINFREQUENCY + freq;
  String msg = F("Changing Radio Mode: LoRa: ");
  msg = msg + mode + " Pwr: " + power + " Freq: " + frequency;
  diagnostic(msg);
  rf95.setModemRegisters(&modeTable[mode]);
  rf95.setTxPower(power);
  if (!rf95.setFrequency(frequency)) {
    diagnostic(F("Setting frequency failure"));
    return(false);
  }
  state.lora.mode = mode;
  state.lora.txPower = power;
  state.lora.freq = freq;
  setRadioLongDistance();
  if (DUMPREGISTERS) rf95.printRegisters();
  return (true);
}

// Function to initialize the radio duing setup.

bool initRadio() {

  timeCheck("init Radio ");
  
  if (!LORA) {
    diagnostic("Radio not configured");
    return(true);
  }
  rf95.setThisAddress (PAYLOAD);
  if (!rf95.init())  {
    diagnostic(F("Radio init failed"));
    if (DUMPREGISTERS) rf95.printRegisters();
    
// Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
//    rf95.printRegisters();

    return(false);
  }
  setRadioMode(state.lora.mode,state.lora.txPower,state.lora.freq);
  logHeader += radioHeader;
  diagnostic("Radio success");
  return(true);
}

bool sendPacket(uint8_t to, uint8_t type, uint8_t flags) {  // Send a message 
  uint8_t i;
  int packetlength;
  String m = "";
                         
  if (!(LORA && TELEMETRY)) return(true);

  timeCheck("send Packet ");
  LEDon();
  transmitTime = millis();
  rf95.setHeaderTo (to);
  rf95.setHeaderFrom (PAYLOAD);
  rf95.setHeaderId (type);
  rf95.setHeaderFlags (flags);
  state.lastDestination = to;
  state.txSeq++; 
  switch (type) {
    case TRACK: {
      trackPacket.txSeq = state.txSeq;                               // Payload transmit sequence number
      trackPacket.lora = state.lora;
      trackPacket.voltage = (int) ((state.voltage)*100);
      trackPacket.timeStamp = millis();
      trackPacket.loc = state.loc;
      packetlength = sizeof(trackPacket);
      uint8_t *packetPtr = (uint8_t *)&trackPacket;  
      if (!rf95.send(packetPtr, packetlength)) {
          diagnostic("Packet send failure");                           // Send packet 
      }
      m = "Track packet sent to "+String(to)+" ";
      break;
    }
    case SENSOR: {
      sndPacket.txSeq = state.txSeq;                               // Payload transmit sequence number
      sndPacket.lora = state.lora;
      sndPacket.voltage = (int) ((state.voltage)*100);
      sndPacket.timeStamp = millis();
      sndPacket.loc = state.loc;
      sndPacket.pA = state.sA;
      sndPacket.pressure = state.pressure;
      sndPacket.temperature = state.temperature;
      sndPacket.rad.events = state.rad.events;
      sndPacket.rad.noise = state.rad.noise;
      sndPacket.rad.pulseWidth = state.rad.pulseWidth;
      sndPacket.rad.energy = state.rad.energy;
      sndPacket.lite = state.lite;
      sndPacket.air = state.air;
      sndPacket.tmp = state.tmp;
      packetlength = sizeof(sndPacket);
      uint8_t *packetPtr = (uint8_t *)&sndPacket;  
      if (!rf95.send(packetPtr, packetlength)) {
          diagnostic("Packet send failure");                           // Send packet 
      }
      m = "Sensor packet sent to "+String(to)+" ";
      break;
    }
    case IMAGE: {
      irPacket.txSeq = state.txSeq;                               // Payload transmit sequence number
      irPacket.lora = state.lora;
      irPacket.voltage = (int) ((state.voltage)*100);
      irPacket.timeStamp = millis();
      irPacket.ir = state.ir;
      packetlength = sizeof(irPacket);
      uint8_t *packetPtr = (uint8_t *)&irPacket;  
      if (!rf95.send(packetPtr, packetlength)) {
          diagnostic("Packet send failure");                           // Send packet 
      }
      rf95.waitPacketSent();                                       // Block until packet transmitted
      rf95.setModeRx();                                            // Set radio to receive mode for next packetm = "Image packet sent to "+String(to)+" ";;
      m = "IR image packet sent to "+String(to)+" ";
      break;
    }
    case ACK: {
      trackPacket.lora = state.lora;
      trackPacket.voltage = (int) ((state.voltage)*100);
      trackPacket.timeStamp = millis();
      trackPacket.loc = state.loc;
      packetlength = sizeof(trackPacket);
      uint8_t *packetPtr = (uint8_t *)&trackPacket;  
      if (!rf95.send(packetPtr, packetlength)) {
          diagnostic("Packet send failure");                           // Send packet 
      }
      m = "Ack packet sent to "+String(to)+" ";
      break;
    }
  }
  rf95.waitPacketSent();
  rf95.setModeRx();                                            // Set radio to receive mode for next packet
  transmitTime = millis() - transmitTime;
  m += String(transmitTime)+"msec, octets: "+String(packetlength);
  diagnostic(m);       
  LEDoff();                             
}

bool receivePacket() {

  if (!LORA) return (true);

  timeCheck("receive Packet ");
  uint8_t len = sizeof(rcvPacket);
  uint8_t *packetPtr = (uint8_t *)&rcvPacket;
  if (!rf95.recv(packetPtr, &len)) {
    diagnostic("Bad received packet");  
    return(false);
  } 
  diagnostic ("Packet received from "+String(rf95.headerFrom()));
  if ((rf95.headerFrom() != GATEWAY) || (rf95.headerFrom() != BROADCAST)) return(false);
  if ((rf95.headerTo() != PAYLOAD) || (rf95.headerTo() != BROADCAST)) return(false);
  state.type = rf95.headerId();
  state.rxSeq = rcvPacket.header.txSeq;
  state.rxBad = rf95.rxBad();
  state.lastRSSI = rf95.lastRssi();
  state.lastSNR = readLastSNR();
  state.lastSource = rf95.headerFrom();
  state.loc = rcvPacket.header.loc;
  state.voltage = rcvPacket.header.voltage/100;
  state.lora = rcvPacket.header.lora;
  state.rxSeq = rcvPacket.header.txSeq;
  state.ground = rcvPacket.header.loc;
  state.rxCount++;
  return(true);
}

// Process received telemetry packets in base station before forwarding to ground station computer
//  Receive packets of various types and process them based on packet type.
//  Copy data to the state vector based on packet type

bool receiveTelemetry() {

  if (!(LORA && TELEMETRY)) return(false);

  if (!receivePacket()) return(false);

  switch (state.type) {
    case TRACK:                                       // Receive and process a tracking telemetry packet
      break;
    case SENSOR:                                      // Receive and process a full sensor packetreceived
      state.sensordata = true;                        // Sensor data has been received
      // Copy the received data to the state vector
      state.sA = rcvPacket.body.sensor.pA;
      state.pressure = rcvPacket.body.sensor.pressure;
      
      break;
    case IMAGE:
      break;
    case ACK:                                         // Receive and process an ACK packet
      break;
    default:
      break;
  }
  return(true);
}


// For ground station ... output received telemetry data from state vector as .csv out USB port
//    Output either just tracking information or complete sensor data depending on what has been received.
//    Prefix the ground station's location and the vector to the S4 to the output

void outputTelemetry() {
  
}


String createRadioMsg() {
  String msg = "";

  if (!LORA) return(msg);
  msg += String(state.lastRSSI)+",";
  msg += String(state.lora.txPower)+",";
  msg += String(state.lora.mode)+",";
  msg += String(state.lora.freq+MINFREQUENCY)+",";
  
  return(msg);
}


// IMU and high altitude Baro sensor Methods

MS5611 baro;
double referencePressure;

#define MS5611_ADDR 0x77

LSM9DS1 amu;

#define LSM9DS1_M  0x1E // Would be 0x1C if SDO_M is LOW  
#define LSM9DS1_AG  0x6B // Would be 0x6A if SDO_AG is LOW  
#define PRINT_CALCULATED

const String MS5611Header = "press,temp,baroalt,";

bool initMS5611(){

  if (!MS5611sensor) {
    diagnostic("MS5611 not configured");
    return(true);
  }

  timeCheck("init MS5611 ");

  if (!baro.begin(MS5611_HIGH_RES)) {
    diagnostic("MS5611 failure");
    return(true);
  }
  
  referencePressure = baro.readPressure();
  state.voltage = readVolt();

  logHeader += MS5611Header;
  diagnostic("MS5611 success");
  return(true);
}

bool initIMU() {

  if (!IMU) {
    diagnostic("LSM9DS1 not configured");
    return(true);
  }

  timeCheck("init IMU ");
  amu.settings.device.commInterface = IMU_MODE_I2C;
  amu.settings.device.mAddress = LSM9DS1_M;
  amu.settings.device.agAddress = LSM9DS1_AG;
  if (!amu.begin()) {
    diagnostic("LSM9DS1 failure");
    return(false);
  }
  amu.setGyroScale(G_SCALE_2000DPS);
  amu.setAccelScale(A_SCALE_16G);
  amu.setMagScale(M_SCALE_4GS);

  logHeader += imuHeader;
  diagnostic("LSM9DS1 success");
  return(true);
  
}

bool getVoltage() {
  
  state.voltage = readVolt();

  return(true);
}

bool getMS5611() {

  // Baro sensor

  if (!MS5611sensor) return(true);

  timeCheck("get MS5611 ");
  state.pressure = baro.readPressure();
  state.temperature = baro.readTemperature();
  state.baroAlt = baro.getAltitude(state.pressure, referencePressure);

  return(true);
}

bool getIMU() {

  // Get IMU sensor
  if (!IMU) return(true);

  timeCheck("get IMU ");
  if (amu.accelAvailable()) {
    amu.readAccel();
    state.sA.aX = amu.calcAccel(amu.ax);
    state.sA.aY = amu.calcAccel(amu.ay);
    state.sA.aZ = amu.calcAccel(amu.az);
  }
  if (amu.gyroAvailable()) {
    amu.readGyro();
    state.sA.gX = amu.calcGyro(amu.gx);
    state.sA.gY = amu.calcGyro(amu.gy);
    state.sA.gZ = amu.calcGyro(amu.gz);
  }
  if (amu.magAvailable()) {
    amu.readMag();
    state.sA.mX = amu.calcMag(amu.mx);
    state.sA.mY = amu.calcMag(amu.my);
    state.sA.mZ = amu.calcMag(amu.mz);
  }
  
  calcAttitude(state.sA.aX, state.sA.aY, state.sA.aZ, -state.sA.mY, -state.sA.mX, state.sA.mZ);

  attitude tempA = getQuaternion(state.sA);

  return(true);

}

void calcAttitude(float aX, float aY, float aZ, float mX, float mY, float mZ) {
  
  float roll = atan2(aY, aZ);
  float pitch = atan2(-aX, sqrt(aY * aY + aZ * aZ));
  float heading;
  
  if (mY == 0)
    heading = (mX < 0) ? PI : 0;
  else
    heading = atan2(mX, mY);
    
//  heading -= DECLINATION * PI / 180;
  
  if (heading > PI) heading -= (2 * PI);
  else if (heading < -PI) heading += (2 * PI);
  else if (heading < 0) heading += 2 * PI;
  
  // Convert everything from radians to degrees:
  heading *= 180.0 / PI;
  pitch *= 180.0 / PI;
  roll  *= 180.0 / PI;

  state.sA.heading = magAdjust(heading);
  state.sA.pitch  = pitch;
  state.sA.roll = roll;
}

String createIMUmsg() {
  String msg = "";

  if (!IMU) return(msg);
  msg += String(state.sA.aX)+",";
  msg += String(state.sA.aY)+",";
  msg += String(state.sA.aZ)+",";
  msg += String(state.sA.gX)+",";
  msg += String(state.sA.gY)+",";
  msg += String(state.sA.gZ)+",";
  msg += String(state.sA.mX)+",";
  msg += String(state.sA.mY)+",";
  msg += String(state.sA.mZ)+",";
  msg += String(state.sA.yaw)+",";
  msg += String(state.sA.pitch)+",";
  msg += String(state.sA.roll)+",";
  msg += String(state.sA.heading)+",";
  
  return(msg);
}

String createMS5611msg() {
  String msg = "";

  if (!MS5611sensor) return(msg);
  msg += String(state.pressure)+",";
  msg += String(state.temperature)+",";
  msg += String(state.baroAlt)+",";
  
  return(msg);
}

// Dust sensor methods


// VEML6075 UV sensor

Adafruit_VEML6075 uv = Adafruit_VEML6075();

const String VEML6075Header = "uva,uvb,";

bool initVEML6075() {

  if (!VEML6075) {
    diagnostic("VEML6075 not configured");
    return(true);
  }

  timeCheck("init 6075 ");
  uv.setIntegrationTime(VEML6075_100MS);                   // Integration time constant.
  logHeader += VEML6075Header;
  diagnostic("VEML6075 setup success");
  return(true);
}

bool getVEML6075() {

  if (!VEML6075) return(true);
  timeCheck("get 6075 ");
  state.lite.uva = uv.readUVA();
  state.lite.uvb = uv.readUVB();

  return(true);
}

String createVEML6075msg() {
  String msg;

  if (!VEML6075) return(msg);
  msg += String(state.lite.uva)+",";
  msg += String(state.lite.uvb)+",";

  return(msg);
}

// VEML6070 UV Light Sensor

Adafruit_VEML6070 uv2 = Adafruit_VEML6070();

const String VEML6070Header = "uva,";

bool initVEML6070() {

  if (!VEML6070) {
    diagnostic("VEML6070 not configured");
    return(true);
  }

  timeCheck("init 6070 ");
  uv2.begin(VEML6070_1_T);
  logHeader += VEML6070Header;
  diagnostic("VEML6070 Setup success");
  return(true);
  
}

void getVEML6070() {

  if (!VEML6070) return;

  timeCheck("get 6070 ");
  state.lite.uva = uv2.readUV();
  
} 

String createVEML6070msg() {
  String msg;

  if (!VEML6070) return(msg);
  msg += String(state.lite.uva)+",";
  return(msg);
  
}

// TSL2591 Visible and IR Light Sensor

Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591);

const String TSL2591Header = "visible,ir,";

bool initTSL2591() {

  if (!TSL2591) {
    diagnostic("TSL2591 not configured");
    return(true);
  }

  timeCheck("init TSL2591 ");
  if (!tsl.begin()) return (false);
  tsl.setGain(TSL2591_GAIN_MED);                //25x gain
  tsl.setTiming(TSL2591_INTEGRATIONTIME_100MS); // shortest integration time

  logHeader += TSL2591Header;
  diagnostic("TSL2591 setup success");
  return(true);
}

bool getTSL2591() {

  if (!TSL2591) return(true);

  timeCheck("get TSL2591 ");
  state.lite.visible = tsl.getLuminosity(TSL2591_VISIBLE);
  state.lite.ir = tsl.getLuminosity(TSL2591_INFRARED);
  
}

String createTSL2591msg() {
  String msg;

  if (!TSL2591) return(msg);
  
  msg += String(state.lite.visible)+",";
  msg += String(state.lite.ir)+",";

  return(msg);
}

// AS7265x Spectrometer 18 Channel 410-940 nm Spectrometer
//   Configure either side looking on sensor board or ground looking on imaging board
//   Either as alternative to TSL2591 or complement

uint16_t AS7265Freq[18] = {610, 680, 730, 760, 810, 860, 560, 585, 645, 705, 900, 940, 410, 435, 460, 485, 510, 535}; // latest data sheet

const String AS7265Header = "610,680,730,760,810,860,560,585,645,705,900,940,410,435,460,485,510,535,";
/* choices are: 
 *  ledIndCurrent led_ind_1_mA, led_ind_2_mA, led_ind_4_mA, led_ind_8_mA
 *  ledDrvCurrent led_drv_12_5_mA, led_drv_25_mA, led_drv_50_mA, led_drv_100_mA
 */
uint8_t ledIndCurrent0 = led_ind_1_mA, ledDrvCurrent0 = led_drv_12_5_mA;
uint8_t ledIndCurrent1 = led_ind_1_mA, ledDrvCurrent1 = led_drv_12_5_mA;
uint8_t ledIndCurrent2 = led_ind_1_mA, ledDrvCurrent2 = led_drv_12_5_mA;

/* choices are:
 *  gain = gain_1x, gain_4x, gain_16x, gain_64x (default 16x)
 *  mode = mode0, mode1, mode2, mode3 (default mode2)
 *  intTime 1 - 255 (default 20) 
*   integration time = intTime * 2.8 milliseconds, so 20 * 2.8 ms == 56 ms default
*   maximum integration time = 714 ms
 */
uint8_t AS7265gain = gain_16x, AS7265mode = mode2, AS7265intTime = 5;

#define intPin 8

AS7265X AS7265X(intPin);


bool initAS7265() {

  if (!AS7265) {
    diagnostic("AS7265 spectrometer not configured");
    return(true);
  }

  timeCheck("init AS7265 ");
  AS7265X.init(AS7265gain,AS7265mode, AS7265intTime);
  AS7265X.configureLed(ledIndCurrent0,ledDrvCurrent0, 0);
  AS7265X.disableIndLed(0);
  AS7265X.disableDrvLed(0);
  delay(100);
  AS7265X.configureLed(ledIndCurrent1,ledDrvCurrent1, 1);
  AS7265X.disableIndLed(1);
  AS7265X.disableDrvLed(1);
  delay(100);
  AS7265X.configureLed(ledIndCurrent2,ledDrvCurrent2, 2);
  AS7265X.disableIndLed(2);
  AS7265X.disableDrvLed(2);
  delay(100);

  logHeader += AS7265Header;
  diagnostic("AS7265 spectrometer setup success");

}

void getAS7265() {

  if (!AS7265) return;

  timeCheck("get AS7265 ");
  uint8_t status = AS7265X.getStatus();
  if (status & 0x02) {
    AS7265X.readCalData(state.lite.spectrumData);
  }
}

String createAS7265msg() {
  String msg = "";

  if (!AS7265) return(msg);

  for (uint8_t i=0; i<18; i++) {
    msg += String(state.lite.spectrumData[i])+",";
  }

  return (msg);
  
}

// BME280 pressure sensor.   Used for humidity, pressure, and air temperature for air quality.   
//    I2C address changed to 0x76

BME280 airbme;

int BMEi2c = 0x76;

const String BME280Header = "hum,press,temp,baroalt,";

bool initBME280() {

  if (!BMEPres) {
    diagnostic("BME 280 not configured");
    return (true);
  }
  timeCheck("init BME280 ");
  airbme.settings.commInterface = I2C_MODE;
  airbme.settings.I2CAddress = BMEi2c;
  airbme.settings.runMode = 3;   // 3, Normal mode
  airbme.settings.tStandby = 0;  // 0, -.5 ms
  airbme.settings.filter = 0;    // 0, filter off
  //tempOverSample can be:
  //  0, skipped
  //  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
  airbme.settings.tempOverSample = 1;
  //pressOverSample can be:
  //  0, skipped
  //  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
  airbme.settings.pressOverSample = 1;
  //humidOverSample can be:
  //  0, skipped
  //  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
  airbme.settings.humidOverSample = 1;
  if (!airbme.begin()) {
    diagnostic("BME 280 failure");
    return(false);
  }

  logHeader += BME280Header;
  diagnostic("BME 280 setup success");
  return(true);
  
}

bool getBME280 () {

  if (!BMEPres) return (true);

  timeCheck("get BME280 ");
  state.air.temperature = airbme.readTempC();
  state.air.pressure = airbme.readFloatPressure();
  state.air.humidity = airbme.readFloatHumidity();
  state.air.baroAlt = airbme.readFloatAltitudeMeters();
  
  return(true);
}

String createBMEmsg () {
  String msg;

  if (!BMEPres) return(msg);
  msg += String(state.air.humidity)+",";
  msg += String(state.air.temperature)+",";
  msg += String(state.air.pressure)+",";
  msg += String(state.air.baroAlt)+",";

  return(msg);
}

// CCS811 Air Quality Sensor

//#define CCS811_ADDR 0x5B              // Default I2C address
#define CCS811_ADDR 0x5A            // Alternate I2C address

// Measurement period
// 1 second

CCS811 myCCS811(CCS811_ADDR);

const String CCS811Header = "co2,tvoc,";

bool initCCS811() {

  if (!CCS811sensor) {
    diagnostic("CCS811 not configured");
    return(true);
  }
  timeCheck("init CCS811 ");
  CCS811Core::status returnCode = myCCS811.begin();
  if (returnCode == CCS811Core::SENSOR_SUCCESS) {
    myCCS811.setDriveMode(1);                         // Measurement every second
    logHeader += CCS811Header;
    diagnostic("CCS811 setup success");
    return (true);
  }
  else {
    diagnostic("CCS811 failure");
    return(false);
  }
}

bool getCCS811() {

  if (!CCS811sensor) return(true);

   timeCheck("get CCS811 ");
  if (!myCCS811.dataAvailable()) return(true);

  if (BMEPres) {
      myCCS811.setEnvironmentalData(state.air.humidity,state.air.temperature);
  }
  myCCS811.readAlgorithmResults();
  state.air.co2 = myCCS811.getCO2();              // ppm
  state.air.voc = myCCS811.getTVOC();             // ppb
}

String createCCS811msg () {
  String msg;

  if (!CCS811sensor) return(msg);
  
  msg += String(state.air.co2)+",";
  msg += String(state.air.voc)+",";

  return(msg);
}

//  External temperature sensors

// MCP9808 

Adafruit_MCP9808 temp1Sensor = Adafruit_MCP9808();

const String MCP9808Header = "temp,";

bool initMCP9808() {

  if (!MCP9808) {
    diagnostic("MCP9808 not configured");
    return(true);
  }

  timeCheck("init MCP9808 ");
  if (!temp1Sensor.begin()) return(false);

  logHeader += MCP9808Header;
  diagnostic("MCP9808 setup success");
  return(true);
}

bool getMCP9808() {

  if (!MCP9808) return(true);

  timeCheck("get MCP9808 ");
  temp1Sensor.shutdown_wake(0);           // Wake up from low power mode 200 microA consumption
  state.tmp.temp1 = temp1Sensor.readTempC();
  temp1Sensor.shutdown_wake(1);           // Return to ow power mode

  return(true);
}

String createMCP9808msg() {
  String msg;

  if (!MCP9808) return(msg);
  msg += String(state.tmp.temp1);
  return(msg);
}

// MLX90614 IR temperature sensor

Adafruit_MLX90614 mlx = Adafruit_MLX90614();

const String MLX90614Header = "atemp,otemp,";

bool initMLX90614() {

  if (!MLX90614) {
    diagnostic("MLX90614 not configured");
    return(true);
  }
  
  timeCheck("init MLX90614 ");
  if (!mlx.begin()) return(false);

  logHeader += MLX90614Header;
  diagnostic("MLX90614 setup success");
  return (true);
}

bool getMLX90614() {

  if (!MLX90614) return(true);

  timeCheck("get MLX90614 ");
  state.tmp.temp1 = mlx.readAmbientTempC();
  state.tmp.temp2 = mlx.readObjectTempC();

  return(true);
}

String createMLX90614msg() {
  String msg;

  if (!MLX90614) return(msg);

  msg += String(state.tmp.temp1)+",";
  msg += String(state.tmp.temp2)+",";

  return(msg);
}



// Imaging sensors - IR, visible, spectrum


// Visible light cam - on Serial1

#define SerCamSpeed 38400

bool initCam() {
  
}

void  getCam() {
  
}

String createCamMsg() {
  
}

// IR Array Sensor


// Beta, x-ray and gamma radiation sensor
// uSv = cpm/53.032 to convert to a dose measurement

#define signalPin A0
#define noisePin  A3

volatile int newSignal = 0;
volatile int newNoise = 0;
volatile int newPulseWidth = 0;
volatile bool pulseState = false;
volatile unsigned long duration = 0;

const String radHeader = "events,noise,pulse,energy,";

bool initRad () {

  if (!RAD) {
    diagnostic("Radiation spectrometer not configured");
    return(true);
  }
  timeCheck("init rad ");
  pinMode(signalPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(signalPin), signalCount, FALLING);
  pinMode(noisePin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(noisePin), noiseCount, FALLING);

  logHeader += radHeader;
  diagnostic("Radiation spectrometer setup success");
  return(true);
}

void getRad() {

  if (!RAD) return;
  timeCheck("get rad ");
  state.rad.events += newSignal;
  state.radSpectrum[newPulseWidth] += newSignal;                // if there is LOTS of signal, this is inaccurate
  newSignal = 0;
  state.rad.noise += newNoise;
  newNoise = 0;
  state.rad.pulseWidth = newPulseWidth;
  state.rad.energy += 0.0018*pow(2.71828,0.0194*newPulseWidth);             // Estimated energy vs pulse width
  newPulseWidth = 0;
}

// Data collection done at interrupt time to detect leading edge of signal and noise pulses from detector
//   The length of the pulse is correlated to particle energy.



void signalEnd() {
  attachInterrupt(digitalPinToInterrupt(signalPin), signalCount, FALLING);
  newPulseWidth = micros() - duration;
  pulseState = false;
  return;
}


void signalCount() {
  attachInterrupt(digitalPinToInterrupt(signalPin), signalEnd, RISING);
  newSignal++;
  duration = micros();
  newPulseWidth = 0;
  pulseState = true;
  return;
}

void noiseCount() {
  newNoise++;
  return;
}

String createRadmsg() {
  String msg;

  if (!RAD) return(msg);
  msg += String(state.rad.events)+",";
  msg += String(state.rad.noise)+",";
  msg += String(state.rad.pulseWidth)+",";
  msg += String(state.rad.energy)+",";

  return(msg);
}

/* Waits for trigger event to start recording
 *  
 *  acceleration  - a certain acceleration in Z axis along line of flight at launch
 *  barometric    - a certain difference in altitude at launch
 *  deploy        - a certain difference in light level that would be detected on a deployment after launch at apogee
 *  
 *  
 */

long histAccel[4] = {0,0,0,0};       // Baseline acceleration trigger
long histBaroAlt[4] = {0,0,0,0};       // Baseline pressure trigger
long avgAccel, avgBaroAlt;
 
void waitForTrigger() {

  if (enableTrigger) return;          //   If trigger already enabled return;

  // Get baseline values for acceleration and pressure
 
  // The Y axis the thrust axis when positioned in an airframe
  
    if (accelTrigger) {
      getIMU();
      histAccel[0] = histAccel[1];
      histAccel[1] = histAccel[2];
      histAccel[2] = histAccel[3];
      histAccel[3] = state.sA.aY;
      avgAccel = (histAccel[0] + histAccel[1] + histAccel[2] + histAccel[3])/4;
      if (avgAccel > accelLevel) {
        enableTrigger = true;
        return;
      }
    }


  // The MS5611 is the default barometer contained on the IMU daughterboard
  
    if (baroTrigger) {
      getMS5611();
      histBaroAlt[0] = histBaroAlt[1];
      histBaroAlt[1] = histBaroAlt[2];
      histBaroAlt[2] = histBaroAlt[3];
      histBaroAlt[3] = state.baroAlt;
      avgBaroAlt = (histBaroAlt[0] + histBaroAlt[1] + histBaroAlt[2] + histBaroAlt[3])/4;
      if (avgBaroAlt > baroLevel) {
        enableTrigger = true;
        return;
      }
    
    }

    if (deployTrigger) {
    
    } 
  
}


//  Setup

void setup() {

  Wire.begin();
  Wire.setClock(I2C_CLOCK);
  Serial.begin(CONSOLESPEED);             // Console interface

  initCLED();

  if (!Serial) { 
      delay(5000);
  };

  diagnostic("Begin Setup");
  
  if (DIAGNOSTIC) I2Cscan();

  initOptions();                // Output state of boot options settings

  state.setup = true;

  greenCLED();
  
  diagnostic("Setup radio");

  // Setup LoRa radio
  state.lora.mode = DEFAULTLORAMODE;
  state.lora.txPower = DEFAULTPOWER;
  state.lora.freq = DEFAULTFREQUENCY;
  if (!initRadio()) {
    state.setup = false;
  }
  
  // Setup GPS
  if (!initGPS()) {
    state.setup = false;
  }

  // Setup IMU
  if (!initIMU()) {
    state.setup = false;
  }
  
  // Setup standard baro sensor
  if (!initMS5611()) {
    state.setup = false;
  }
 
  // Setup air quality pressure sensor BME280

  if (!initBME280()) {
    state.setup = false;
  }

  // Setup air quality sensor CCS811

  if (!initCCS811()) {
    state.setup = false;
  }

  // Setup dust sensor



  // Setup TSL2591 light sensor

  if (!initTSL2591()) {
    state.setup = false;
  } 
  
  // Setup uv sensor

  if (!initVEML6070()) {
    state.setup = false;
  }


  if (!initVEML6075()) {
    state.setup = false;
  } 

  // Set up spectrometer

  if (!initAS7265()) {
    state.setup = false;
  }

  // Setup MCP9808 Temp sensor

  if (!initMCP9808()) {
    state.setup=false;
  }

  // Setup MLX90416 Sensor

  if (!initMLX90614()) {
    state.setup = false;
  } 

  // Setup imaging board if configured

  // Setup 1-Wire temp sensors
  
  if (!init1WireTemp()) {
    state.setup = false;
  }

  // Setup radiation sensor

  if (!initRad()) {
    state.setup=false;
  }

  if (TELEMETRY) {
    diagnostic("Telemetry enabled");
    diagnostic("Payload address is "+String(PAYLOAD));
    diagnostic("Gateway address is "+String(GATEWAY));
  }

  if (Serial) {
    diagnostic("Begin uploading logs");
    uploadLogs();
    diagnostic("Erasing logs");
    eraseLogs();
  }
  

   if (!initLogs()) {
    state.setup = false;
  }

 
  
  // Finish setup
  state.lastTime = millis();
  if (state.setup) 
    diagnostic("Setup succeeded");
  else {
    diagnostic("Setup failed");
    redCLED();
    while (true) LEDon();
  }
  state.lastTelemetry = millis();
  state.lastOneWire = millis();
  state.lastWatchdog  = millis();
  diagnosticState();
  
  if (state.setup) greenCLED();
}

void flow (String txt) {
  if (FLOWENABLE) Serial.println(txt);
}

// Main processing loop


void loop() {
  String msg;
  
  //  getOptions();
  
  // Get core sensors
  
  getGPS();
  if (S4 == Ground) state.ground = state.loc;                   // In ground station, the local location is the ground station.
  getVoltage();
  getMS5611();
  getIMU();

  // Get light sensors
  getVEML6070();              // UV intensity
  // getVEML6075();           // Conflicts with I2C GPS
  getTSL2591();               // Visible and IR intensity
  getAS7265();                // 410-940 nm 18 channel spectrometer
  
  // Get air quality sensors
  getBME280();                // Humidity, temperature, pressure
  getCCS811();                // eCO2, TVOC
  getRad();                   // Radiation ... 
  // getDust();               // Particulate matter spectrometer

  // Lightning sensor
  
  // Temp sensors
  getMCP9808();
  getMLX90614();
  if ((millis() - state.lastOneWire) >= ONEWIRETIME) {
    get1WireTemp();  
    state.lastOneWire = millis();                                   
  }

  // Trigger imaging sensors
  // This all probably needs to be consolidated into one integrated imaging thingie for the imaging board

  timeCheck("Core loop is ");

  
// Process telemetry and logs

  if (S4 != Ground) {                                 // If flight S4
      saveSensorState();                              // Save state 
      if (enableTrigger) writeLogs();                 // If logs triggered write them
      sendTelemetry();                                // Send telemetry if configured and time
  } else {                                            // If ground station
    // do ground station
    if (receiveTelemetry()) {
      outputTelemetry();
    }
  }

}

void timeCheck (String s) {
    static long loopTime = millis();
    
    if (TIMECHECK) {
      loopTime = millis() - loopTime;
      Serial.print(s); Serial.println(millis());
    }
   
}



float  readVolt() {
  float voltage;

  voltage = analogRead(VOLT);
  voltage *= 2;
  voltage *= 3.3;
  voltage /= 1024;
  
  return (voltage);
}

int convertRssi(uint8_t rssi) {

  return(-157-convert2s(rssi));
}

int signalStrength (uint8_t rssi, uint8_t snr) {
  
  int strength;
  int tempsnr;
  
  tempsnr = convert2s(snr)/4;
  if (tempsnr < 0)
    strength = -157 + convertRssi(rssi) + tempsnr * .25;
  else
    strength = -157 + rssi;
  return (strength);
}

int convert2s(uint8_t field) {
  int temp =0;
  
  temp = field & 0x7F;
  if ((field & 0x80) == 0x80) temp *= -1;
  return (temp);
}

uint8_t readLastSNR () {
  return (rf95.spiRead(0x19));
}

uint8_t readCurRSSI () {
  return (rf95.spiRead(0x1b));
}
void setRadioLongDistance() {

// Set Low Data Rate Optimize   Register 0x26 0x08

//  rf95.spiWrite(0x26, 0x08);

// Set LoRa Detection Threshold   Register 0x37 0x0A

  rf95.spiWrite(0x37, 0x0A);

// Set LoRa Detection Optimize  Register 0x31 0x03

  rf95.spiWrite(0x31, 0x03);

}


  
void I2Cscan()
{
// scan for i2c devices
  uint8_t error, address;
  int nDevices;
  String msg;

  diagnostic("I2C Scanning...");

  nDevices = 0;
  for(address = 1; address < 127; address++ ) 
  {
// The i2c_scanner uses the return value of
// the Write.endTransmisstion to see if
// a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission(true);
    if (error == 0)
    {
      msg = "I2C device found at address 0x";
      if (address<16) msg += "0";
      msg += String(address,HEX);
      msg += " ! ";
      msg += sensorName(address);
      msg += sensorState(address);
      Serial.println(msg);
      nDevices++;
    } else if (error==4) {
      msg = "Unknown error at address 0x";
      if (address<16) 
        msg += "0";
      msg += String(address, HEX);
      Serial.println(msg);
    }
  }
  if (nDevices == 0)
    diagnostic("No I2C devices found");
  else {
    msg = String(nDevices) + " I2C devices found";
    diagnostic(msg);
  }
  diagnostic("I2C scan done");
    

}

// Enable the I2C sensors discovered on the scan

String sensorState(uint8_t address) {

  for (int i = 0; i < numSensors; i++) {
    if (sensors[i].address == address) 
      if (*sensors[i].state) return(" enabled");
      else return(" disabled");
  }

  return(" unknown");
}

String sensorName(uint8_t address) {

  for (int i = 0; i < numSensors; i++) {
    if (sensors[i].address == address) return(sensors[i].name);
  }
  return("Not found");
}

float magAdjust(float h) {

  double adjust = DECLINATION;
  double heading =+ adjust;
  if (heading <0) state.sA.heading = 360 + heading;
  else if (heading >= 360) state.sA.heading = heading - 360;
  else state.sA.heading = heading;

  return(heading);
}




// 1-Wire temperature sensor network for multiple sensors
// Uses D9 on QUBE processor board

#define ONE_WIRE_BUS 9                            // D/A port defining 1-Wire bus
#define MAX_1Wire    1
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature tempSensors(&oneWire);

DeviceAddress devAddr;
uint8_t addr[8];
int num1WireSensors = 0;

bool init1WireTemp () {

  if (!ONEWIRE) {
    diagnostic("1Wire sensors not configured");
    return (true);
  }
  tempSensors.begin();
  num1WireSensors = tempSensors.getDeviceCount();
  diagnostic("Found "+String(num1WireSensors)+" 1Wire devices");
  // Find devices on the bus 
  for (int i = 0; i<num1WireSensors; i++) {
    tempSensors.getAddress(devAddr, i);
    tempSensors.setResolution(devAddr, 9);
  }
  if (num1WireSensors == 0) return(false);
  // Set resolution on sensors
  tempSensors.requestTemperatures();
  return(true);
}

void get1WireTemp () {

  if (!ONEWIRE) return;
  for (int i =0; i < num1WireSensors; i++) {
    tempSensors.getAddress(devAddr, i);
    state.tmp.temp1Wire[i]= tempSensors.getTempC(devAddr);                         
  }
  tempSensors.requestTemperatures();
  return;
}

String create1WireMsg () {
  String msg = "";

  if (!ONEWIRE) return(msg);
  for (int i=0;i<4;i++) {
    msg += String(state.tmp.temp1Wire[i])+",";
  }

  return(msg);
}


//  Option pins setting
//  Diagnostic mode - OPTION0
//  Sensor/track mode - OPTION1
//  Only for Qube

// Setup options

static bool diagOpt = false, telOpt = false;

bool initOptions() {

  pinMode(OPTION0, INPUT_PULLUP);
  pinMode(OPTION1, INPUT_PULLUP);

//  if (!QUBE) return(false);

  diagOpt = digitalRead(OPTION0);
  telOpt = digitalRead(OPTION1);
  
  if (diagOpt) {
    Serial.println("Diagnotics enabled");
    DIAGNOSTIC = true;
  } else {
    Serial.println("Diagnostics disabled");
    DIAGNOSTIC = false;
  }
  if (telOpt) {
    Serial.println("Full sensor telemetry enabled");
    TELEMETRYMODE = true;                            // Do I need to reset radio?
  } else {
    Serial.println("Tracking telemetry enabled");
    TELEMETRYMODE = false;
  }
  
  return(true);
}

// Poll options during main loop so options are dynamic

void getOptions() {

//  if (!QUBE) return;
  
  diagOpt = digitalRead(OPTION0);                   // Read Diagnostic mode pins
  telOpt = digitalRead(OPTION1);                    // Read telemetry mode pins:  

  if (diagOpt) {
    DIAGNOSTIC = true;
  } else {
    DIAGNOSTIC = false;
  }
  if (telOpt && !TELEMETRYMODE) {                   // Telemetry mode changes from track to sensor
    TELEMETRYMODE = true;                            
    state.lora.mode = SENSORMODE;
    setRadioMode (state.lora.mode, state.lora.txPower, state.lora.freq);              // Change radio mode to new mode
  } else if (!telOpt && TELEMETRYMODE) {             // Telemetry mode changes from sensor to track
    TELEMETRYMODE = false;                           
    state.lora.mode = TRACKMODE;
    setRadioMode (state.lora.mode, state.lora.txPower, state.lora.freq);              // Change radio mode to new mode
  }
  
  return;
}



 
// Quaternion Library

// global constants for 9 DoF fusion and AHRS (Attitude and Heading Reference System)
float GyroMeasError = PI * (40.0f / 180.0f);   // gyroscope measurement error in rads/s (start at 40 deg/s)
float GyroMeasDrift = PI * (0.0f  / 180.0f);   // gyroscope measurement drift in rad/s/s (start at 0.0 deg/s/s)
// There is a tradeoff in the beta parameter between accuracy and response speed.
// In the original Madgwick study, beta of 0.041 (corresponding to GyroMeasError of 2.7 degrees/s) was found to give optimal accuracy.
// However, with this value, the LSM9SD0 response time is about 10 seconds to a stable initial quaternion.
// Subsequent changes also require a longish lag time to a stable output, not fast enough for a quadcopter or robot car!
// By increasing beta (GyroMeasError) by about a factor of fifteen, the response time constant is reduced to ~2 sec
// I haven't noticed any reduction in solution accuracy. This is essentially the I coefficient in a PID control sense; 
// the bigger the feedback coefficient, the faster the solution converges, usually at the expense of accuracy. 
// In any case, this is the free parameter in the Madgwick filtering and fusion scheme.
float beta = sqrt(3.0f / 4.0f) * GyroMeasError;   // compute beta
float zeta = sqrt(3.0f / 4.0f) * GyroMeasDrift;   // compute zeta, the other free parameter in the Madgwick scheme usually set to a small or zero value
#define Kp 2.0f * 5.0f // these are the free parameters in the Mahony filter and fusion scheme, Kp for proportional feedback, Ki for integral
#define Ki 0.0f

uint32_t delt_t = 0, count = 0, sumCount = 0;  // used to control display output rate
//float pitch, yaw, roll;
float deltat = 0.0f, sum = 0.0f;          // integration interval for both filter schemes
uint32_t lastUpdate = 0, firstUpdate = 0; // used to calculate integration interval
uint32_t Now = 0;                         // used to calculate integration interval

//float ax, ay, az, gx, gy, gz, mx, my, mz; // variables to hold latest sensor data values 
float q[4] = {1.0f, 0.0f, 0.0f, 0.0f};    // vector to hold quaternion
float eInt[3] = {0.0f, 0.0f, 0.0f};       // vector to hold integral error for Mahony method


attitude getQuaternion(attitude I) {
  attitude O;

  Now = micros();
  deltat = ((Now - lastUpdate)/1000000.0f); // set integration time by time elapsed since last filter update
  lastUpdate = Now;

  sum += deltat; // sum for averaging filter update rate
  sumCount++;
  
  // Sensors x, y, and z axes of the accelerometer and gyro are aligned. The magnetometer  
  // the magnetometer z-axis (+ up) is aligned with the z-axis (+ up) of accelerometer and gyro, but the magnetometer
  // x-axis is aligned with the -x axis of the gyro and the magnetometer y axis is aligned with the y axis of the gyro!
  // We have to make some allowance for this orientation mismatch in feeding the output to the quaternion filter.
  // For the LSM9DS1, we have chosen a magnetic rotation that keeps the sensor forward along the x-axis just like
  // in the LSM9DS0 sensor. This rotation can be modified to allow any convenient orientation convention.
  // This is ok by aircraft orientation standards!  
  // Pass gyro rate as rad/s
  MadgwickQuaternionUpdate(I.aX, I.aY, I.aZ, I.gX*PI/180.0f, I.gY*PI/180.0f, I.gZ*PI/180.0f,  -I.mX,  I.mY, I.mZ);
//  MahonyQuaternionUpdate(ax, ay, az, gx*PI/180.0f, gy*PI/180.0f, gz*PI/180.0f, -mx, my, mz);

  // Define output variables from updated quaternion---these are Tait-Bryan angles, commonly used in aircraft orientation.
  // In this coordinate system, the positive z-axis is down toward Earth. 
  // Yaw is the angle between Sensor x-axis and Earth magnetic North (or true North if corrected for local declination, looking down on the sensor positive yaw is counterclockwise.
  // Pitch is angle between sensor x-axis and Earth ground plane, toward the Earth is positive, up toward the sky is negative.
  // Roll is angle between sensor y-axis and Earth ground plane, y-axis up is positive roll.
  // These arise from the definition of the homogeneous rotation matrix constructed from quaternions.
  // Tait-Bryan angles as well as Euler angles are non-commutative; that is, the get the correct orientation the rotations must be
  // applied in the correct order which for this configuration is yaw, pitch, and then roll.
  // For more see http://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles which has additional links.
    O = I;
    O.yaw   = atan2(2.0f * (q[1] * q[2] + q[0] * q[3]), q[0] * q[0] + q[1] * q[1] - q[2] * q[2] - q[3] * q[3]);   
    O.pitch = -asin(2.0f * (q[1] * q[3] - q[0] * q[2]));
    O.roll  = atan2(2.0f * (q[0] * q[1] + q[2] * q[3]), q[0] * q[0] - q[1] * q[1] - q[2] * q[2] + q[3] * q[3]);
    O.pitch *= 180.0f / PI;
    O.yaw   *= 180.0f / PI; 
    O.yaw   -= 13.8f; // Declination at Danville, California is 13 degrees 48 minutes and 47 seconds on 2014-04-04
    O.roll  *= 180.0f / PI;

    return(O);
}

        void MadgwickQuaternionUpdate(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz)
        {
            float q1 = q[0], q2 = q[1], q3 = q[2], q4 = q[3];   // short name local variable for readability
            float norm;
            float hx, hy, _2bx, _2bz;
            float s1, s2, s3, s4;
            float qDot1, qDot2, qDot3, qDot4;

            // Auxiliary variables to avoid repeated arithmetic
            float _2q1mx;
            float _2q1my;
            float _2q1mz;
            float _2q2mx;
            float _4bx;
            float _4bz;
            float _2q1 = 2.0f * q1;
            float _2q2 = 2.0f * q2;
            float _2q3 = 2.0f * q3;
            float _2q4 = 2.0f * q4;
            float _2q1q3 = 2.0f * q1 * q3;
            float _2q3q4 = 2.0f * q3 * q4;
            float q1q1 = q1 * q1;
            float q1q2 = q1 * q2;
            float q1q3 = q1 * q3;
            float q1q4 = q1 * q4;
            float q2q2 = q2 * q2;
            float q2q3 = q2 * q3;
            float q2q4 = q2 * q4;
            float q3q3 = q3 * q3;
            float q3q4 = q3 * q4;
            float q4q4 = q4 * q4;

            // Normalise accelerometer measurement
            norm = sqrt(ax * ax + ay * ay + az * az);
            if (norm == 0.0f) return; // handle NaN
            norm = 1.0f/norm;
            ax *= norm;
            ay *= norm;
            az *= norm;

            // Normalise magnetometer measurement
            norm = sqrt(mx * mx + my * my + mz * mz);
            if (norm == 0.0f) return; // handle NaN
            norm = 1.0f/norm;
            mx *= norm;
            my *= norm;
            mz *= norm;

            // Reference direction of Earth's magnetic field
            _2q1mx = 2.0f * q1 * mx;
            _2q1my = 2.0f * q1 * my;
            _2q1mz = 2.0f * q1 * mz;
            _2q2mx = 2.0f * q2 * mx;
            hx = mx * q1q1 - _2q1my * q4 + _2q1mz * q3 + mx * q2q2 + _2q2 * my * q3 + _2q2 * mz * q4 - mx * q3q3 - mx * q4q4;
            hy = _2q1mx * q4 + my * q1q1 - _2q1mz * q2 + _2q2mx * q3 - my * q2q2 + my * q3q3 + _2q3 * mz * q4 - my * q4q4;
            _2bx = sqrt(hx * hx + hy * hy);
            _2bz = -_2q1mx * q3 + _2q1my * q2 + mz * q1q1 + _2q2mx * q4 - mz * q2q2 + _2q3 * my * q4 - mz * q3q3 + mz * q4q4;
            _4bx = 2.0f * _2bx;
            _4bz = 2.0f * _2bz;

            // Gradient decent algorithm corrective step
            s1 = -_2q3 * (2.0f * q2q4 - _2q1q3 - ax) + _2q2 * (2.0f * q1q2 + _2q3q4 - ay) - _2bz * q3 * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (-_2bx * q4 + _2bz * q2) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + _2bx * q3 * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
            s2 = _2q4 * (2.0f * q2q4 - _2q1q3 - ax) + _2q1 * (2.0f * q1q2 + _2q3q4 - ay) - 4.0f * q2 * (1.0f - 2.0f * q2q2 - 2.0f * q3q3 - az) + _2bz * q4 * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (_2bx * q3 + _2bz * q1) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + (_2bx * q4 - _4bz * q2) * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
            s3 = -_2q1 * (2.0f * q2q4 - _2q1q3 - ax) + _2q4 * (2.0f * q1q2 + _2q3q4 - ay) - 4.0f * q3 * (1.0f - 2.0f * q2q2 - 2.0f * q3q3 - az) + (-_4bx * q3 - _2bz * q1) * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (_2bx * q2 + _2bz * q4) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + (_2bx * q1 - _4bz * q3) * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
            s4 = _2q2 * (2.0f * q2q4 - _2q1q3 - ax) + _2q3 * (2.0f * q1q2 + _2q3q4 - ay) + (-_4bx * q4 + _2bz * q2) * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (-_2bx * q1 + _2bz * q3) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + _2bx * q2 * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
            norm = sqrt(s1 * s1 + s2 * s2 + s3 * s3 + s4 * s4);    // normalise step magnitude
            norm = 1.0f/norm;
            s1 *= norm;
            s2 *= norm;
            s3 *= norm;
            s4 *= norm;

            // Compute rate of change of quaternion
            qDot1 = 0.5f * (-q2 * gx - q3 * gy - q4 * gz) - beta * s1;
            qDot2 = 0.5f * (q1 * gx + q3 * gz - q4 * gy) - beta * s2;
            qDot3 = 0.5f * (q1 * gy - q2 * gz + q4 * gx) - beta * s3;
            qDot4 = 0.5f * (q1 * gz + q2 * gy - q3 * gx) - beta * s4;

            // Integrate to yield quaternion
            q1 += qDot1 * deltat;
            q2 += qDot2 * deltat;
            q3 += qDot3 * deltat;
            q4 += qDot4 * deltat;
            norm = sqrt(q1 * q1 + q2 * q2 + q3 * q3 + q4 * q4);    // normalise quaternion
            norm = 1.0f/norm;
            q[0] = q1 * norm;
            q[1] = q2 * norm;
            q[2] = q3 * norm;
            q[3] = q4 * norm;

        }
  
  
  
 // Similar to Madgwick scheme but uses proportional and integral filtering on the error between estimated reference vectors and
 // measured ones. 
            void MahonyQuaternionUpdate(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz)
        {
            float q1 = q[0], q2 = q[1], q3 = q[2], q4 = q[3];   // short name local variable for readability
            float norm;
            float hx, hy, bx, bz;
            float vx, vy, vz, wx, wy, wz;
            float ex, ey, ez;
            float pa, pb, pc;

            // Auxiliary variables to avoid repeated arithmetic
            float q1q1 = q1 * q1;
            float q1q2 = q1 * q2;
            float q1q3 = q1 * q3;
            float q1q4 = q1 * q4;
            float q2q2 = q2 * q2;
            float q2q3 = q2 * q3;
            float q2q4 = q2 * q4;
            float q3q3 = q3 * q3;
            float q3q4 = q3 * q4;
            float q4q4 = q4 * q4;   

            // Normalise accelerometer measurement
            norm = sqrt(ax * ax + ay * ay + az * az);
            if (norm == 0.0f) return; // handle NaN
            norm = 1.0f / norm;        // use reciprocal for division
            ax *= norm;
            ay *= norm;
            az *= norm;

            // Normalise magnetometer measurement
            norm = sqrt(mx * mx + my * my + mz * mz);
            if (norm == 0.0f) return; // handle NaN
            norm = 1.0f / norm;        // use reciprocal for division
            mx *= norm;
            my *= norm;
            mz *= norm;

            // Reference direction of Earth's magnetic field
            hx = 2.0f * mx * (0.5f - q3q3 - q4q4) + 2.0f * my * (q2q3 - q1q4) + 2.0f * mz * (q2q4 + q1q3);
            hy = 2.0f * mx * (q2q3 + q1q4) + 2.0f * my * (0.5f - q2q2 - q4q4) + 2.0f * mz * (q3q4 - q1q2);
            bx = sqrt((hx * hx) + (hy * hy));
            bz = 2.0f * mx * (q2q4 - q1q3) + 2.0f * my * (q3q4 + q1q2) + 2.0f * mz * (0.5f - q2q2 - q3q3);

            // Estimated direction of gravity and magnetic field
            vx = 2.0f * (q2q4 - q1q3);
            vy = 2.0f * (q1q2 + q3q4);
            vz = q1q1 - q2q2 - q3q3 + q4q4;
            wx = 2.0f * bx * (0.5f - q3q3 - q4q4) + 2.0f * bz * (q2q4 - q1q3);
            wy = 2.0f * bx * (q2q3 - q1q4) + 2.0f * bz * (q1q2 + q3q4);
            wz = 2.0f * bx * (q1q3 + q2q4) + 2.0f * bz * (0.5f - q2q2 - q3q3);  

            // Error is cross product between estimated direction and measured direction of gravity
            ex = (ay * vz - az * vy) + (my * wz - mz * wy);
            ey = (az * vx - ax * vz) + (mz * wx - mx * wz);
            ez = (ax * vy - ay * vx) + (mx * wy - my * wx);
            if (Ki > 0.0f)
            {
                eInt[0] += ex;      // accumulate integral error
                eInt[1] += ey;
                eInt[2] += ez;
            }
            else
            {
                eInt[0] = 0.0f;     // prevent integral wind up
                eInt[1] = 0.0f;
                eInt[2] = 0.0f;
            }

            // Apply feedback terms
            gx = gx + Kp * ex + Ki * eInt[0];
            gy = gy + Kp * ey + Ki * eInt[1];
            gz = gz + Kp * ez + Ki * eInt[2];

            // Integrate rate of change of quaternion
            pa = q2;
            pb = q3;
            pc = q4;
            q1 = q1 + (-q2 * gx - q3 * gy - q4 * gz) * (0.5f * deltat);
            q2 = pa + (q1 * gx + pb * gz - pc * gy) * (0.5f * deltat);
            q3 = pb + (q1 * gy - pa * gz + pc * gx) * (0.5f * deltat);
            q4 = pc + (q1 * gz + pa * gy - pb * gx) * (0.5f * deltat);

            // Normalise quaternion
            norm = sqrt(q1 * q1 + q2 * q2 + q3 * q3 + q4 * q4);
            norm = 1.0f / norm;
            q[0] = q1 * norm;
            q[1] = q2 * norm;
            q[2] = q3 * norm;
            q[3] = q4 * norm;
 
        }



void configureS4Mission() {

  switch (S4) {

    case Egg:                                   // Configure for baseline Egg
      GPS = false;
      IMU = false;
      MS5611sensor = false;
      LORA = false;
      TELEMETRY = false;
      CCS811sensor = true;
      BMEPres = true;
      break;
    case Qube:                                  // Configure for baseline 1P S4
      GPS = true;
      IMU = true;
      MS5611sensor = true;
      LORA = true;
      TELEMETRY = true;
      CCS811sensor = true;
      VEML6070 = true;
      BMEPres = true;
      break;
    case QubePlus:                               // Configure for maximum 1P S4
      GPS = true;
      IMU = true;
      MS5611sensor = true;
      LORA = true;
      TELEMETRY = true;
      CCS811sensor = true;
      VEML6070 = true;
      BMEPres = true;
      RAD = true;
      DUST = true;
      break;
    case Ground:                                  // Configure for 1P ground station
      GPS = true;
      LORA = true;
      TELEMETRY = true;
      break;
    case Custom:                                  // Allow for an arbitrary custom configuration
      break;
    default:
      break;
  }

  return;
}
