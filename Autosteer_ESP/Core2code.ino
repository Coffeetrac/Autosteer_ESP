//Core2: this task only serves the Webpage

void Core2code( void * pvParameters ){
  Serial.println();
  Serial.print("Task2 running on core ");
  Serial.println(xPortGetCoreID());

  
// Start WiFi Client
 while (!EE_done){  // wait for eeprom data
  delay(10);
 }
 WiFi_Start_STA();
 if (my_WiFi_Mode == 0) WiFi_Start_AP(); // if failed start AP

 UDP_Start();  // start the UDP Client

  for(;;){
    //digitalWrite(DIR_PIN, LOW);
    //digitalWrite(DIR_PIN, HIGH);
    //if (!steerEnable) 
    WiFi_Traffic();
    delay(10);  
  
  }
}
