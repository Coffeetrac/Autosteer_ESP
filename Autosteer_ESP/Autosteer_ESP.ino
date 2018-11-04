TaskHandle_t Core1;
TaskHandle_t Core2;
  //##########################################################################################################
  //### Setup Zone ###########################################################################################
  // Just now,  only default values, will be overwritten by the Web config site
  //##########################################################################################################
    
  byte output_type = 4;       //set to 1  if you want to use Stering Motor + Cytron MD30C Driver
                              //set to 2  if you want to use Stering Motor + IBT 2  Driver
                              //set to 3  if you want to use IBT 2  Driver + PWM 2-Coil Valve
                              //set to 4  if you want to use  IBT 2  Driver + Danfoss Valve PVE A/H/M
                              

  byte input_type  = 2;       //0 = No ADS installed, Wheel Angle Sensor connected directly to ESP at GPIO 4 (attention 3,3V only)
                              //1 = Single Mode of ADS1115 - Sensor Signal at A0 (ADS)
                              //2 = Differential Mode - Connect Sensor GND to A1, Signal to A0

  byte IMU_type     = 1;      // set to 1 to enable BNO055 IMU
  
  byte Inclino_type = 1;      // set to 1 if MMA8452 is installed

  bool Invert_WAS   = 0;      // set to 1 to Change Direction of Wheel Angle Sensor - to + 
         
  byte SWEncoder    = 1;      // Steering Wheel ENCODER Installed
  byte pulseCountMax= 3;      // Switch off Autosteer after x Pulses from Steering wheel encoder 
  int SteerPosZero=512;
  //##########################################################################################################
  //### End of Setup Zone ####################################################################################
  //##########################################################################################################

// IO pins --------------------------------
#define SDA     21  //I2C Pins
#define SCL     22

#define Autosteer_Led  26
#define PWM_PIN        25
#define DIR_PIN        32
#define led1           14
#define led2           33
#define led3           15
#define led4           13

#define W_A_S           4
#define WORKSW_PIN     37   
#define STEERSW_PIN    35
#define encAPin        38
#define encBPin        34

//libraries -------------------------------
#include "Wire.h"
#include "Network_AOG.h"
#include "SSD1306Wire.h"
#include "BNO_ESP.h"
#include "Adafruit_ADS1015.h"
#include "MMA8452_AOG.h" 
#include "EEPROM.h"

// Variables ------------------------------
struct Storage {
    float Ko = 0.05f;  //overall gain
    float Kp = 5.0f;  //proportional gain
    float Ki = 0.001f;//integral gain
    float Kd = 1.0f;  //derivative gain 
    float steeringPositionZero = 13000;
    byte minPWMValue=10;
    int maxIntegralValue=20;//max PWM value for integral PID component
    float steerSensorCounts=100;
  };  Storage steerSettings;

 //loop time variables in microseconds
  const unsigned int LOOP_TIME = 50; //10hz 
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
  volatile bool steerEnable = false, toggleSteerEnable=false;
  byte relay = 0, uTurn = 0, speeed = 0, workSwitch = 0, steerSwitch = 1, switchByte = 0;
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
  int16_t roll = 0, roll_corr=0;
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

SSD1306Wire  display(0x3c, SDA, SCL);  //OLed Display
Adafruit_ADS1115 ads;     // Use this for the 16-bit version ADS1115
MMA8452 accelerometer;
WiFiServer server(80);
WiFiClient client;
AsyncUDP udp;


// Setup procedure ------------------------
void setup() {
  Wire.begin(SDA, SCL, 400000);
  Serial.begin(115200); 
  pinMode(Autosteer_Led, OUTPUT);
  pinMode(PWM_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  pinMode(led4, OUTPUT);
  pinMode(WORKSW_PIN, INPUT);
  pinMode(STEERSW_PIN, INPUT);
  pinMode(encAPin, INPUT);
  pinMode(encBPin, INPUT);

  
  //set up the pgn for returning data for autosteer
  toSend[0] = 0x7F;
  toSend[1] = 0xFD;

//------------------------------------------------------------------------------------------------------------  
  //create a task that will be executed in the Core1code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(Core1code, "Core1", 10000, NULL, 1, &Core1, 0);
  // Core1code,   /* Task function. */
  // "Core1",     /* name of task. */
  // 10000,       /* Stack size of task */
  // NULL,        /* parameter of the task */
  // 1,           /* priority of the task */
  // &Core1,      /* Task handle to keep track of created task */
  // 0);          /* pin task to core 0 */                  */
  delay(500); 

  //create a task that will be executed in the Core2code() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(Core2code, "Core2", 10000, NULL, 1, &Core2, 1); 
  delay(500); 
//------------------------------------------------------------------------------------------------------------

//Setup Interrupt -Steering Wheel encoder + SteerSwitchbutton
attachInterrupt(digitalPinToInterrupt(encAPin), EncoderA_ISR, FALLING);
attachInterrupt(digitalPinToInterrupt(encBPin), EncoderB_ISR, FALLING);
attachInterrupt(digitalPinToInterrupt(STEERSW_PIN), Steersw_ISR, FALLING);

if (input_type==0)  SteerPosZero =2048;                //Starting Point with ESP ADC 2048 
if (input_type >0 && input_type < 3 )  SteerPosZero =13000;  //with ADS start with 13000  

  ledcSetup(0,1000,8);  // PWM Output with channel 0, 1kHz, 8-bit resolution (0-255)
  ledcSetup(1,1000,8);  // PWM Output with channel 1, 1kHz, 8-bit resolution (0-255)
  ledcAttachPin(PWM_PIN,0);  // attach PWM PIN to Channel 0
  ledcAttachPin(DIR_PIN,1);  // attach PWM PIN to Channel 1

if (EEprom_empty_check()==1) { //first start?
    EEprom_write_all();     //write default data
  }
if (EEprom_empty_check()==2) { //data available
    EEprom_read_all();
  }
EE_done =1; // 
EEprom_show_memory();  //
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
