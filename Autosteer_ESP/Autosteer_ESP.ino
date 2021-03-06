TaskHandle_t Core1;
TaskHandle_t Core2;

 //##########################################################################################################
  //### Setup Zone ###########################################################################################
  // Just default values
  //##########################################################################################################
  #define useOLED_Display 0         // 0 if Oled Display is not connected
                                    // 1 if Oled Display is connected
                  
  #define timeoutRouter  65         // Time (seconds) to wait for WIFI access, after that own Access Point starts      
                              
struct Storage{    
  char ssid[24]      = "yourSSID";          // WiFi network Client name
  char password[24]  = "YourPassword";      // WiFi network password

  byte output_type = 2;       //set to 1  if you want to use Stering Motor + Cytron MD30C Driver
                              //set to 2  if you want to use Stering Motor + IBT 2  Driver
                              //set to 3  if you want to use IBT 2  Driver + PWM 2-Coil Valve
                              //set to 4  if you want to use  IBT 2  Driver + Danfoss Valve PVE A/H/M
                              
  byte input_type  = 2;       //0 = No ADS installed, Wheel Angle Sensor connected directly to ESP at GPIO 4 (attention 3,3V only)
                              //1 = Single Mode of ADS1115 - Sensor Signal at A0 (ADS)
                              //2 = Differential Mode - Connect Sensor GND to A1, Signal to A0

  byte IMU_type     = 0;      // set to 1 to enable BNO055 IMU
  
  byte Inclino_type = 0;      // set to 1 if MMA8452 is installed

  bool Invert_WAS   = 0;      // set to 1 to Change Direction of Wheel Angle Sensor - to + 
         
  byte SWEncoder    = 0;      // Steering Wheel ENCODER Installed
  byte pulseCountMax= 3;      // Switch off Autosteer after x Pulses from Steering wheel encoder 
  
  int SteerPosZero  = 512;

  byte SteerSwitchType = 2;   //0 = enable = switch high (3,3V) //1 = enable = switch low(GND) //2 = toggle = button to low(GND)
                              //3 = enable = button to high (3,3V), disable = button to low (GND), neutral = 1,65V
  
  byte WorkSW_mode = 1;       // 0 = disabled   // 1 = digital ON/OFF // 2 = analog Value 0...4095 (0 - 3,3V)
  
  byte Invert_WorkSW = 0;     // 0 = Hitch raised -> High    // 1 = Hitch raised -> Low
  
  unsigned int WorkSW_Threshold = 1200;    // Value for analog hitch level to switch workswitch  
  //##########################################################################################################
  //### End of Setup Zone ####################################################################################
  //##########################################################################################################
  float Ko = 0.05f;  //overall gain  
  float Kp = 5.0f;  //proportional gain  
  float Ki = 0.001f;//integral gain
  float Kd = 1.0f;  //derivative gain 
  float steeringPositionZero = 13000;  byte minPWMValue=10;
  int maxIntegralValue=20;//max PWM value for integral PID component
  float steerSensorCounts=100;  int roll_corr = 0;
};  Storage steerSettings;


//Accesspoint name and password:
const char* ssid_ap     = "AG_Autosteer_ESP_Net";
const char* password_ap = "";

//static IP
IPAddress myip(192, 168, 1, 77);   // Autosteer module
IPAddress gwip(192, 168, 1, 77);   // Gateway & Accesspoint IP
IPAddress mask(255, 255, 255, 0);
IPAddress myDNS(8, 8, 8, 8);       //optional

unsigned int portMy  = 5577; //this is port of this module: Autosteer = 5577
unsigned int portAOG = 8888; // port to listen for AOG

//IP address to send UDP data to:
IPAddress ipDestination(192, 168, 1, 255);
unsigned int portDestination = 9999; // Port of AOG that listens



// IO pins --------------------------------
#define SDA     21  //I2C Pins
#define SCL     22
#define RX1      9  
#define TX1     10  
#define RX2     13  
#define TX2     12 

#define Autosteer_Led  25
#define PWM_PIN        32
#define DIR_PIN        26
#define LED_PIN_WIFI   33
#define led1           14
#define led2           15

#define W_A_S           4
#define WORKSW_PIN     37   
#define STEERSW_PIN    38
#define encAPin        35
#define encBPin        34

//libraries -------------------------------
#include "Wire.h"
#include "Network_AOG.h"
#if (useOLED_Display)
  #include "SSD1306Wire.h"  // 0.96" OLED
  //#include "SH1106Wire.h" // Alternate 1,3"
#endif
#include "BNO_ESP.h"
#include "Adafruit_ADS1015.h"
#include "MMA8452_AOG.h" 
#include "EEPROM.h"

// Variables ------------------------------
 // WiFi status LED blink times: searching WIFI: blinking 4x faster; connected: blinking as times set; data available: light on; no data for 2 seconds: blinking
  unsigned long LED_WIFI_time = 0;
  unsigned long LED_WIFI_pulse = 700;   //light on in ms 
  unsigned long LED_WIFI_pause = 700;   //light off in ms
  boolean LED_WIFI_ON = false;
  unsigned long UDP_data_time = 0;
  
 //loop time variables in microseconds
  const unsigned int LOOP_TIME = 100; //10hz 
  unsigned int lastTime = LOOP_TIME;
  unsigned int currentTime = LOOP_TIME;
  unsigned int dT = 50000;
  byte count = 0;
  byte watchdogTimer = 0;

 //Kalman variables
  float rollK = 0, Pc = 0.0, G = 0.0, P = 1.0, Xp = 0.0, Zp = 0.0;
  float XeRoll = 0;
  const float varRoll = 0.1; // variance,
  const float varProcess = 0.0055; //0,00025 smaller is more filtering

   //program flow
  bool isDataFound = false, isSettingFound = false, AP_running=0,EE_done = 0;
  int header = 0, tempHeader = 0, temp;
  int AnalogValue = 0;
  volatile bool steerEnable = false, toggleSteerEnable=false;
  byte relay = 0, uTurn = 0, speeed = 0, workSwitch = 0, workSwitchOld = 0, steerSwitch = 1, switchByte = 0;
  float distanceFromLine = 0, corr = 0; // not used
  int16_t idistanceFromLine = 0;
  float olddist=0;
//  unsigned long oldmillis;  


  //steering variables
  float steerAngleActual = 0;
  int steerPrevSign = 0, steerCurrentSign = 0; // the steering wheels angle currently and previous one
  int16_t isteerAngleSetPoint = 0; //the desired angle from AgOpen
  float steerAngleSetPoint = 0;
  long steeringPosition = 0, steeringPosition_corr = 0,actualSteerPos=0; //from steering sensor
  float steerAngleError = 0; //setpoint - actual
  float distanceError = 0; //
  volatile int  pulseACount = 0, pulseBCount = 0;; // Steering Wheel Encoder

  
  //IMU, inclinometer variables
  bool imu_initialized=0;
  int16_t roll = 0;
  uint16_t x_ , y_ , z_;

  //pwm variables
  int pwmDrive = 0, drive = 0, pwmDisplay = 0, pwmOut = 0;
  float pValue = 0, iValue = 0, dValue = 0;
    
  //integral values - **** change as required *****
  int maxIntErr = 200; //anti windup max
  int maxIntegralValue = 20; //max PWM value for integral PID component 

  //Array to send data back to AgOpenGPS
  byte toSend[] = {0,0,0,0,0,0,0,0,0,0};
  //data that will be received from server
  uint8_t data[10];


// Debug ----------------------------------
byte state_after=0, state_previous=0, breakreason=0;
// Instances ------------------------------
#if (useOLED_Display)
  SSD1306Wire  display(0x3c, SDA, SCL);  //OLed 0.96" Display
  //SH1106Wire  display(0x3C, SDA, SCL);  //OLed 1.3" Display
#endif
Adafruit_ADS1115 ads;     // Use this for the 16-bit version ADS1115
MMA8452 accelerometer;
WiFiServer server(80);
WiFiClient client;
AsyncUDP udp;


// Setup procedure ------------------------
void setup() {
  Wire.begin(SDA, SCL, 400000);
  Serial.begin(115200); 
  //Serial1.begin(115200, SERIAL_8N1, RX1, TX1);
  //Serial2.begin(38400,SERIAL_8N1,RX2,TX2); 
  
  pinMode(Autosteer_Led, OUTPUT);
  pinMode(PWM_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(LED_PIN_WIFI, OUTPUT);
  if (steerSettings.WorkSW_mode > 0) {
    pinMode(WORKSW_PIN, INPUT_PULLUP);
  }
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);

  
  //set up the pgn for returning data for autosteer
  toSend[0] = 0x7F;
  toSend[1] = 0xFD;

  ledcSetup(0,1000,8);  // PWM Output with channel 0, 1kHz, 8-bit resolution (0-255)
  ledcSetup(1,1000,8);  // PWM Output with channel 1, 1kHz, 8-bit resolution (0-255)
  ledcAttachPin(PWM_PIN,0);  // attach PWM PIN to Channel 0
  ledcAttachPin(DIR_PIN,1);  // attach PWM PIN to Channel 1

  if (steerSettings.input_type==0)  steerSettings.SteerPosZero =2048;                //Starting Point with ESP ADC 2048 
  if (steerSettings.input_type >0 && steerSettings.input_type < 3 )  steerSettings.SteerPosZero =13000;  //with ADS start with 13000  
  
  restoreEEprom();
  
  //------------------------------------------------------------------------------------------------------------  
  //create a task that will be executed in the Core1code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(Core1code, "Core1", 10000, NULL, 1, &Core1, 0);
  delay(500); 
  //create a task that will be executed in the Core2code() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(Core2code, "Core2", 10000, NULL, 1, &Core2, 1); 
  delay(500); 
  //------------------------------------------------------------------------------------------------------------
  
  while ((my_WiFi_Mode == 0)) {   // Waiting for WiFi Access
    delay(1000);
  }
  //Setup Interrupt -Steering Wheel encoder + SteerSwitchbutton
  delay(2000);
  pinMode(WORKSW_PIN, INPUT_PULLUP);
  if (steerSettings.SteerSwitchType > 0 ){ pinMode(STEERSW_PIN, INPUT_PULLUP); }
  if (steerSettings.SteerSwitchType == 0) { pinMode(STEERSW_PIN, INPUT_PULLDOWN); }
  pinMode(encAPin, INPUT_PULLUP);
  pinMode(encBPin, INPUT_PULLUP);


  //Setup Interrupt -Steering Wheel encoder + SteerSwitchbutton
  attachInterrupt(digitalPinToInterrupt(encAPin), EncoderA_ISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(encBPin), EncoderB_ISR, FALLING);
  if (steerSettings.SteerSwitchType >= 2) { attachInterrupt(digitalPinToInterrupt(STEERSW_PIN), Steersw_ISR, FALLING); }
}
  



void loop() {
  
}



//ISR SteerSwitch Interrupt
void Steersw_ISR() // handle pin change interrupt for Steersw Pin 
  {
   static unsigned long last_interrupt_time = 0;
   unsigned long interrupt_time = millis();
   // If interrupts come faster than 300ms, assume it's a bounce and ignore
   if (interrupt_time - last_interrupt_time > 300 ) 
     {
      //steerEnable = !steerEnable;
      toggleSteerEnable=1;
     }
   last_interrupt_time = interrupt_time;
  }

 //ISR Steering Wheel Encoder
  void EncoderA_ISR()
  {       
    #if (SWEncoder >=0)      
         pulseACount++; 
         //digitalWrite(led1, !digitalRead(led1));
      
    #endif     
} 
 //ISR Steering Wheel Encoder
  void EncoderB_ISR()
  {       
    #if (SWEncoder >=0)      
         pulseBCount++; 
         //digitalWrite(led2, !digitalRead(led2));
    #endif     
} 
