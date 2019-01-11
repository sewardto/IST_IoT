#include "Wire.h"

#define port_cell_0 10
#define port_cell_1 11
#define port_cell_mov_0 2
#define port_cell_mov_1 4
#define monitor_port_0 7
#define monitor_port_1 8
#define addr_this 3
#define light_sensor A0

typedef byte package[7];

/*
* package form :
*   [0]: source
*   [1]: event
*   [2]: destination
*   [3:6]: current time
*/

int syncronize();
void ledCtrl(package message_receive);
int moveDetect();
byte* i2cCtrl(byte cellId, byte event, byte dest, long curTime);
bool destPred(package message, byte cell_eg);
void monitor();

package message_receive;
package message_send;
const byte addr_cell[2] = {0x01, 0x02};
const byte port_cell[2] = {10,11};
const byte port_cell_mov[2] = {2, 4};
const byte monitor_port[2] = {7, 8};
bool button_state[2];
bool flag[2];
long count1, count2;
long delayOfNeighbors;
bool health[2] = {true, true};
bool fullLightOn[2] = {false, false};
int badLED;

void setup()
{
  Serial.begin(9600);
  Wire.begin(addr_this);
  Wire.onReceive(callbackFunction);
  pinMode(port_cell_mov_0, INPUT_PULLUP);
  pinMode(port_cell_mov_1, INPUT_PULLUP);
  pinMode(monitor_port_0, INPUT);
  pinMode(monitor_port_1, INPUT);
  pinMode(port_cell_0, OUTPUT);
  pinMode(port_cell_1, OUTPUT);
  //delayOfNeighbors = syncronize();
}

void loop()
{
//  const int light = analogRead(light_sensor);
//  if(light < 300) {
/*for(int i = 0; i < sizeof(package); i++){
        message_receive[i] = Wire.read();
    }*/
    ledCtrl(message_receive);
    monitor();
    //Serial.println("Hello");
    const int ctrl = moveDetect();
    const long curTime = millis() - delayOfNeighbors;
    byte desti;
    switch(ctrl){
      case 0:
        analogWrite(port_cell[0], 255);    //full
        fullLightOn[0] = true;
        memcpy(message_send, i2cCtrl(0, 1, 0, curTime), 7 * sizeof(byte));
        if(destPred(message_receive, addr_cell[0])){
          desti = ((((addr_cell[0] >> 4) & 0xF) * 2 - ((message_receive[0] >> 4) & 0xF))  << 4) | (((addr_cell[0]) & 0xF) * 2 - ((message_receive[0]) & 0xF));
          Serial.println(desti);
          package message_send_temp;
          memcpy(message_send_temp, i2cCtrl(0, 2, desti, curTime), 7 * sizeof(byte));
          if(desti == addr_cell[1])
            memcpy(message_send, message_send_temp, 7 * sizeof(byte));
        }
        memcpy(message_receive, message_send, sizeof(package));
        count1 = millis();
        break;
      case 1:
        analogWrite(port_cell[1], 255);
        fullLightOn[1] = true;
        memcpy(message_send, i2cCtrl(1, 1, 0, curTime), 7 * sizeof(byte));
        if(destPred(message_receive, addr_cell[1])){
          desti = (((((addr_cell[1] >> 4) & 0xF) * 2 - ((message_receive[0] >> 4) & 0xF)) << 4) | (((addr_cell[1]) & 0xF) * 2 - ((message_receive[0]) & 0xF)));
          package message_send_temp;
          memcpy(message_send_temp, i2cCtrl(1, 2, desti, curTime), 7 * sizeof(byte)); 
          if(desti == addr_cell[0])
            memcpy(message_send, message_send_temp, 7 * sizeof(byte)); 
        }
        memcpy(message_receive, message_send, sizeof(package));
        count2 = millis();
        break;
      default:
        break;
    }

    if(millis() - count1 > 1000 && addr_cell[0] != 0x00) {
      analogWrite(port_cell[0], 63);
      count1 = millis();
      fullLightOn[0] = false;
    }
    if(millis() - count2 > 1000 && addr_cell[1] != 0x00) {
      analogWrite(port_cell[1], 63);
      count2 = millis();
      fullLightOn[1] = false;
    }
//  }
//  else {
//    analogWrite(port_cell[0], 0);
//    analogWrite(port_cell[1], 0);
//  }
}

void callbackFunction(int i)
{
  //Serial.println("Begin");
   for(int i = 0; Wire.available(); i++){
        message_receive[i] = Wire.read();
       // Serial.println(message_receive[i]);
        if(i > sizeof(package))
          break;
    }
      //Serial.println("End");
}

int syncronize() {
  long times = millis();
  byte time_count[1][4];
  Wire.beginTransmission(4);
  Wire.write((byte*)&times, sizeof(long));
  Wire.endTransmission();
  for(int i = 0; i < 1; ){
    for(int j = 0; Wire.available(); j++){
        time_count[i][j] = Wire.read();
        if(j > sizeof(long)){
            i++;
            break;
        }
    }
  }
  long time_rec = (*(long*)&time_count[1] + *(long*)&time_count[2]) / 4;
  return millis() - time_rec;
}

void ledCtrl(package message_receive) {
  //Serial.println(badLED);
  if(message_receive[2] == 0x00 && addr_cell[0] == 0x00) {
      if(message_receive[1] == 254){
        badLED++;
        message_receive[1] = 250;
      } else if(message_receive[1] == 255) {
        badLED--;
        message_receive[1] = 250;
      }
      if(badLED == 0) {
        analogWrite(port_cell[0],0);
      } else {
        analogWrite(port_cell[0],255);
      }
  }
  else if(message_receive[1] == 1) {
    if(fullLightOn[0] == false && ((abs(message_receive[0] - addr_cell[0])==17) || (abs(message_receive[0] - addr_cell[0])==1) || (abs(message_receive[0] - addr_cell[0])==15) || (abs(message_receive[0] - addr_cell[0])==16))){
      analogWrite(port_cell[0],127);  //half
      count1 = millis();
    }
      
    if(fullLightOn[1] == false && ((abs(message_receive[0] - addr_cell[1])==17) || (abs(message_receive[0] - addr_cell[1])==1) || (abs(message_receive[0] - addr_cell[1])==15) || (abs(message_receive[0] - addr_cell[1])==16))){
       analogWrite(port_cell[1],127);
       count2 = millis();
    }
      message_receive[1]=0;
  }
  else if (message_receive[1] == 2) {
    Serial.println(message_receive[2]);
    if ( message_receive[2] == addr_cell[0] ) {
      analogWrite(port_cell[0],255);
      count1 = millis();
      fullLightOn[0] = true;
    }
    else if ( message_receive[2] == addr_cell[1]) {
      analogWrite(port_cell[1],255);
      count2 = millis();
      fullLightOn[1] = true;
    }
  }
}

inline bool isNeighbor(int i){
  return (abs(message_receive[0] - addr_cell[i]) == 17) || (abs(message_receive[0] - addr_cell[i]) == 1) || (abs(message_receive[0] - addr_cell[i]) == 15) || (abs(message_receive[0] - addr_cell[i]) == 16);
}

byte* i2cCtrl(byte cellId, byte event, byte dest, long curTime) {
    static package message;
    message[0] = addr_cell[cellId];
    message[1] = event;
    message[2] = dest;
    message[3] = (curTime >> 24) & 0xFF;    //split a long into 4 byte
    message[4] = (curTime >> 16) & 0xFF;
    message[5] = (curTime >> 8) & 0xFF;
    message[6] = curTime & 0xFF;
    Wire.beginTransmission(4);
    Wire.write(message, sizeof(package));
    Wire.endTransmission();
//    Serial.println("Begin");
//    Serial.println(message[0]);
//    Serial.println(message[1]);
//    Serial.println(message[2]);
 //   Serial.println("End");
    return message;
}

void monitor(){
  int count1, count2;
  bool monitor_test1 = 0, monitor_test2 = 0;
  for(int i=0; i < 200; i++) {
    if (digitalRead(monitor_port_0) == 1) {
      monitor_test1 = 1;
    }
    if (digitalRead(monitor_port_1) ==1) {
      //Serial.println(count2);
      monitor_test2=1;
    }
  }
  //Serial.println(monitor_test1);
  //Serial.println(monitor_test2);
    if(monitor_test1 == 0 && health[0] == true) {
      memcpy(message_receive, i2cCtrl(0, 254, 0x00, millis() - delayOfNeighbors), 7 * sizeof(byte));
      //Serial.println("BAD");
      health[0] = false;
    }
    else if(monitor_test1 != 0 && health[0] == false) {
      memcpy(message_receive, i2cCtrl(0, 255, 0x00, millis() - delayOfNeighbors), 7 * sizeof(byte));
        //Serial.println("NICE");
      health[0] = true;
    }
    if(monitor_test2 == 0 && health[1] == true) {
      memcpy(message_receive, i2cCtrl(1, 254, 0x00, millis() - delayOfNeighbors), 7 * sizeof(byte));
      //Serial.println("BAD1");
      health[1] = false;
    }
    else if(monitor_test2 != 0 && health[1] == false) {
      memcpy(message_receive, i2cCtrl(1, 255, 0x00, millis() - delayOfNeighbors), 7 * sizeof(byte));
        //Serial.println("NICE1");
      health[1] = true;
    }
}

//use a capacitance to amplify the button
void moveDetectinCell(bool buttonId){
    const auto button_input = digitalRead(port_cell_mov[buttonId]);
      if(button_input == HIGH)
        flag[buttonId] = 1;
      else
        flag[buttonId] = 0;
}

int moveDetect() {
    moveDetectinCell(0);
    if(!flag[0]){
        moveDetectinCell(1);
        if(!flag[1])
            return -1;
        return 1;
    }
    return 0;
}

//predict the destination
bool destPred(package message, byte cell_eg)
{
  //combine 4 byte into a long
  unsigned long time1 = message[3];
  time1 = time1 << 8 | message[4];
  time1 = time1 << 8 | message[5];
  time1 = time1 << 8 | message[6];
  long t = (millis() - time1);
//  Serial.println(message[0] - cell_eg);
  if((abs(message[0] - cell_eg) == 1 || abs(message[0] - cell_eg) == 16)){ //&& t * 5600 <= 4000000) {
      Serial.println(t);
      return 1;
  }
  if(abs(message[0] - cell_eg)==15 || abs(message[0] - cell_eg)==17){// && (millis() - time1) * 5600 <= 57000) {
      return 1;
  }
  return 0;}
