void SetRelays(void)
 {
    if (bitRead(relay,0)) digitalWrite(led1, HIGH);
    else digitalWrite(led1, LOW);
    if (bitRead(relay,1)) digitalWrite(led2, HIGH);
    else digitalWrite(led2, LOW); 
    if (bitRead(relay,2)) digitalWrite(led3, HIGH);
    else digitalWrite(led3, LOW); 
    //if (bitRead(relay,3)) digitalWrite(led4, HIGH);
    //else digitalWrite(led4, LOW); 
    
    //if (bitRead(relay,4)) bitSet(PORTB, 1); //Digital Pin 9
    //else bitClear(PORTB, 1); 
    //if (bitRead(relay,5)) bitSet(PORTB, 4); //Digital Pin 12
    //else bitClear(PORTB, 4); 
    //if (bitRead(relay,6)) bitSet(PORTC, 4); //analog Pin A4
    //else bitClear(PORTC, 4); 
    //if (bitRead(relay,7)) bitSet(PORTC, 5); //Analog Pin A5
    //else bitClear(PORTC, 5); 
  }

//--------------------------------------------------------------
//  EEPROM Data Handling
//--------------------------------------------------------------
#define EEPROM_SIZE 128
#define EE_ident1 0xDE  // Marker Byte 0 + 1
#define EE_ident2 0xED


//--------------------------------------------------------------
byte EEprom_empty_check(){
    
  if (!EEPROM.begin(EEPROM_SIZE))  
    {
     Serial.println("failed to initialise EEPROM"); delay(1000);
     return false;
    }
  if (EEPROM.read(0)!= EE_ident1 || EEPROM.read(1)!= EE_ident2)
     return true;  // is empty
  
  if (EEPROM.read(0)== EE_ident1 && EEPROM.read(1)== EE_ident2)
     return 2;     // data available
     
 }
//--------------------------------------------------------------
void EEprom_write_all(){
  EEPROM.write(0, EE_ident1);
  EEPROM.write(1, EE_ident2);
  EEPROM.write(2, output_type);
  EEPROM.write(3, input_type);
  EEPROM.write(4, IMU_type );
  EEPROM.write(5, Inclino_type);
  EEPROM.write(6, Invert_WAS);
  EEPROM.write(7, SWEncoder);
  EEPROM.write(8, pulseCountMax);
  EEPROM.put( 9, roll_corr);
  EEPROM.put(11, SteerPosZero); 
  EEPROM.put(16, ssid);
  EEPROM.put(40, password);  
  EEPROM.put(64, steerSettings);
  //EEPROM.put(xx, 
  EEPROM.commit();
}
//--------------------------------------------------------------
void EEprom_read_all(){
  output_type = EEPROM.read(2);
  input_type  = EEPROM.read(3);
  IMU_type    = EEPROM.read(4);
  Inclino_type= EEPROM.read(5);
  Invert_WAS  = EEPROM.read(6);
  SWEncoder   = EEPROM.read(7);
  pulseCountMax= EEPROM.read(8);
  EEPROM.get(9, roll_corr); //high low bytes
  EEPROM.get(11, SteerPosZero); 
  //EEPROM.read(12, 111);
  //EEPROM.read(13, 111);
  EEPROM.get(16, ssid);
  EEPROM.get(40, password);  
  EEPROM.get(64, steerSettings); 
}
//--------------------------------------------------------------
void EEprom_show_memory(){
byte c2=0, data_;

  Serial.println(" bytes read from Flash . Values are:");
  for (int i = 0; i < EEPROM_SIZE; i++)
  { 
    data_=byte(EEPROM.read(i));
    if (data_ < 0x10) Serial.print("0");
    Serial.print(data_,HEX); 
    if (c2>=15) {
       Serial.println();
       c2=-1;
      }
    else Serial.print(" ");
    c2++;
  }
}




   
