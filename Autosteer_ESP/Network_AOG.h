// all about Network

#include <WiFi.h>
#include <AsyncUDP.h>

#include <WiFiAP.h>
#include <WiFiClient.h>


// WiFi network Client name and password:
char ssid[24]     = "yourSSID";
char password[24]  = "YourPassword";

//Accesspoint name and password:
const char* ssid_ap     = "AG_Autosteer_ESP_Net";
const char* password_ap = "passport";



//Declarations
void WiFi_Start_STA(void);


//static IP
IPAddress myip(192, 168, 1, 77);  // Autosteer module
IPAddress gwip(192, 168, 1, 77);   // Gateway & Accesspoint IP
IPAddress mask(255, 255, 255, 0);
IPAddress myDNS(8, 8, 8, 8); //optional

unsigned int portMy = 5577; //this is port of this module: Autosteer = 5577
unsigned int portAOG = 8888; // port to listen for AOG

//IP address to send UDP data to:
//const char * ipDestination = "192.168.1.255"; // PC with AOG
IPAddress ipDestination(192, 168, 1, 255);
unsigned int portDestination = 9999; // Port of AOG that listens



// Wifi variables & definitions

#define MAX_PACKAGE_SIZE 2048
char HTML_String[6000];
char HTTP_Header[150];

byte my_WiFi_Mode = 0;  // WIFI_STA = 1 = Workstation  WIFI_AP = 2  = Accesspoint
//---------------------------------------------------------------------
// Allgemeine Variablen

int Aufruf_Zaehler = 0;

#define ACTION_SET_SSID        1  
#define ACTION_SET_OUTPUT_TYPE 2  // also adress at EEPROM
#define ACTION_SET_WAS_TYPE    3
#define ACTION_SET_WAS_ZERO    4
#define ACTION_SET_WAS_INVERT  5
#define ACTION_SET_IMU_TYPE    6
#define ACTION_SET_INCLINO     7
#define ACTION_SET_INCL_ZERO   8
#define ACTION_SET_ENCODER     9

int action;

// Radiobutton output
char output_driver_tab[5][22] = {"None", "Cytron MD30 + SWM", "IBT_2 +SWM", "IBT_2 +PWM Valve", "IBT_2 +Danfoss Valve"};

// Radiobutton analog input
char was_input_tab[3][25] = {"Arduino/ESP direct", "ADS 1115 single", "ADS 1115 differential"};

// Radiobutton WAS Invert
char was_invert_tab[2][15] = {"not inverted", "inverted"};

// Radiobutton IMU Heading Unit
char imu_type_tab[2][10] = {"None", "BNO 055"};

// Radiobutton Inclinometer
char inclino_type_tab[2][10] = {"None", "MMA 8452"};

// Radiobutton Encoder
char encoder_type_tab[2][11] = {"None", "Installed"};

char tmp_string[20];
//---------------------------------------------------------------------
