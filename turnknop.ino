#include <Arduino.h>
#include <Servo.h>
#define MY_BAUD_RATE 9600

#define MY_DEBUG
#define MY_RADIO_NRF24
#define MY_NODE_ID 6

#define SERVO_PIN 8
#define SWITCH_PIN 7

#define CHILD_ID_SERVO 0
#define POWER_DOWN_AFTER 5000

#include <SPI.h>
#include <MySensors.h>

Servo servo1;
uint8_t currentPos = 0;
bool initialValueSent = false;
int sw = LOW;
unsigned long powerUpAt;
int p_address = 1;

MyMessage msgPosition(CHILD_ID_SERVO, V_PERCENTAGE);

void setup() {  
  servo1.attach(SERVO_PIN);
  pinMode(SWITCH_PIN, OUTPUT);   
  digitalWrite(SWITCH_PIN, sw);    
}


void presentation()
{

  // Send the Sketch Version Information to the Gateway
  sendSketchInfo("Fan Sketch", "1.0");

   // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_SERVO, S_COVER);
}

void sendCurrentPosition() {
  Serial.print("Sending position ");
  Serial.print(currentPos);
  uint8_t percentage = 0;
  if (currentPos == 0) {
    percentage = 0;
  } else if (currentPos == 1) {
    percentage = 33;
  } else if (currentPos == 2) {
    percentage = 66;
  } else if (currentPos == 3) {
    percentage = 100;
  } 
  Serial.print(" percentage ");
  Serial.println(percentage);
  
  send(msgPosition.set(percentage));
}

void receive(const MyMessage &message) {
  if (message.isAck()) {
     Serial.println("This is an ack from gateway");
  }
  Serial.println("Message received");
  switch (message.sensor){
    case CHILD_ID_SERVO:
      handleServoMessage(message);
      break;
    default:
      Serial.println("Unknown id");    
      break;
  }
  Serial.println("Done processing message");    
}

void handleServoMessage(const MyMessage &message) {
  Serial.println("Controlling ther servo");
  if (message.type == V_PERCENTAGE) {                
    uint8_t newPercentage = message.getByte();     
    Serial.print("New percentage ");
    Serial.println(newPercentage);
    initialValueSent = true;       
    if (newPercentage > 66 && newPercentage <= 100) {
      currentPos = 3;
      write2Servo();
      sendCurrentPosition();
    } else if (newPercentage > 33 && newPercentage <= 66) {
      currentPos = 2;
       write2Servo();
      sendCurrentPosition();
    } else if (newPercentage > 0 && newPercentage <= 33) {
      currentPos = 1;
       write2Servo();
      sendCurrentPosition();
    } else if (newPercentage <=  0) {
      currentPos = 0;
      write2Servo();
      sendCurrentPosition();
    }
  } else if (message.type == V_UP) {        
    Serial.println("UP!");
    if (currentPos < 3) {
      currentPos++;
      write2Servo();
      sendCurrentPosition();
    }                
  } else if (message.type == V_DOWN) {        
    Serial.println("DOWN!");
    if (currentPos > 0) {
      currentPos--;
      write2Servo();
      sendCurrentPosition();
    }        
  } else if (message.type == V_STOP) {        
    Serial.println("STOP!");
    currentPos = 0;
    write2Servo();
    sendCurrentPosition();
  } else {
    Serial.println("Unknown variable type");
  }
}

void write2Servo() {
  sw = HIGH;
  digitalWrite(SWITCH_PIN, sw); 
  //wait for servo to power up
  servo1.write(currentPos * 30);  
  saveState(p_address, currentPos);
  Serial.print("Moved servo to ");    
  Serial.println(currentPos * 30);    
  powerUpAt = millis();  
}

void loop() {  
  if (!initialValueSent) {
    currentPos = loadState(p_address);
    Serial.print("read position from eeprom ");
    Serial.println(currentPos);
    Serial.println("Sending initial value");
    sendCurrentPosition();
    Serial.println("Requesting initial value from controller");
    request(CHILD_ID_SERVO, V_PERCENTAGE);
    wait(2000, C_SET, V_PERCENTAGE);
  }

  unsigned long on4 = millis() - powerUpAt;  
  if (sw == HIGH && on4 > POWER_DOWN_AFTER) {
    Serial.println("Powering down servo");
    powerUpAt = 0;
    sw = LOW;
    digitalWrite(SWITCH_PIN, sw);
  }
}
