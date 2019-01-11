#include "Arduino.h"
#include "Wire.h"

#define addr_this               3
#define light_sensor            A0

#define pacakge_source          0
#define pacakge_event           1
#define package_current_time_0  2
#define package_current_time_1  3
#define package_current_time_2  4
#define package_current_time_3  5
#define pacakge_dest            6

typedef byte package[7];
const byte addr_cell[2] = {0x01, 0x02};

/*
* package form :
*   [0]: source
*   [1]: event
*   [2]: destination
*   [3:6]: current time
*/

int         syncronize  (void);
int         moveDetect  (void);
void        ledCtrl     (package message_receive);
void        monitor     (void);
byte*       i2cCtrl     (byte cellId, byte event, byte dest, long curTime);
bool        destPred    (package message, byte cell_eg);
inline bool isNeighbor  (int i);

package message_receive;

const byte port_cell[2]     =   {10,11};
const byte port_cell_mov[2] =   {2, 4};
const byte monitor_port[2]  =   {7, 8};

bool    health[2] = {true, true};
bool    fullLightOn[2];
int     badLED;
long    delayOfNeighbors;
long    time_count[2];

int             ledState[2];
int             buttonState[2];
int             lastButtonState[2];
unsigned long   lastDebounceTime = 0;
unsigned long   debounceDelay = 50;

void setup()
{
  //Serial.begin(9600);
  Wire.begin(addr_this);
  Wire.onReceive(callbackFunction);
  pinMode(port_cell_mov[0], INPUT_PULLUP);
  pinMode(port_cell_mov[1], INPUT_PULLUP);
  pinMode(monitor_port[0], INPUT);
  pinMode(monitor_port[1], INPUT);
  pinMode(port_cell[0], OUTPUT);
  pinMode(port_cell[1], OUTPUT);
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
    const int ctrl = moveDetect();

    if(ctrl == -1) { }
    else runtimeCtrl(ctrl);

    if(millis() - time_count[0] > 1000 && addr_cell[0] != 0x00) {
      analogWrite(port_cell[0], 63);
      time_count[0] = millis();
      fullLightOn[0] = false;
    }
    if(millis() - time_count[1] > 1000 && addr_cell[1] != 0x00) {
      analogWrite(port_cell[1], 63);
      time_count[1] = millis();
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
   for(int i = 0; Wire.available(); i++){
        message_receive[i] = Wire.read();
        if(i > sizeof(package))
          break;
    }
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

void runtimeCtrl(int ctrl) {
    package message_send;
    const long curTime = millis() - delayOfNeighbors;

    analogWrite(port_cell[ctrl], 255);
    fullLightOn[ctrl] = true;

    memcpy(message_send, i2cCtrl(ctrl, 1, 0, curTime), 7 * sizeof(byte));
    if(destPred(message_receive, addr_cell[ctrl])){
        int desti = (((((addr_cell[ctrl] >> 4) & 0xF) * 2 - ((message_receive[pacakge_source] >> 4) & 0xF)) << 4) | (((addr_cell[ctrl]) & 0xF) * 2 - ((message_receive[pacakge_source]) & 0xF)));
        package message_send_temp;
        memcpy(message_send_temp, i2cCtrl(ctrl, 2, desti, curTime), 7 * sizeof(byte)); 
        if(desti == addr_cell[!ctrl])
            memcpy(message_send, message_send_temp, 7 * sizeof(byte)); 
    }
    memcpy(message_receive, message_send, sizeof(package));
    time_count[ctrl] = millis();
}

void ledCtrl(package message_receive) {
    if(message_receive[pacakge_dest] == 0x00 && addr_cell[0] == 0x00) {
        if(message_receive[pacakge_event] == 254){
            badLED++;
            message_receive[pacakge_event] = 250;
        } else if(message_receive[pacakge_event] == 255) {
            badLED--;
            message_receive[pacakge_event] = 250;
        }
        if(badLED == 0) {
            analogWrite(port_cell[0],0);
        } else {
            analogWrite(port_cell[0],255);
        }
    } else if(message_receive[pacakge_event] == 1) {
        if(fullLightOn[0] == false && isNeighbor(0)){
            analogWrite(port_cell[0],127);  //half
            time_count[0] = millis();
        }
        if(fullLightOn[1] == false && isNeighbor(1)){
            analogWrite(port_cell[1],127);
            time_count[1] = millis();
        }
        message_receive[pacakge_event]=0;
    } else if (message_receive[pacakge_event] == 2) {
        if ( message_receive[pacakge_dest] == addr_cell[0] ) {
            analogWrite(port_cell[0],255);
            time_count[0] = millis();
            fullLightOn[0] = true;
        } else if ( message_receive[pacakge_dest] == addr_cell[1]) {
            analogWrite(port_cell[1],255);
            time_count[1] = millis();
            fullLightOn[1] = true;
        }
    }
}

inline bool isNeighbor(int i){
  return (abs(message_receive[pacakge_source] - addr_cell[i]) == 17) || (abs(message_receive[pacakge_source] - addr_cell[i]) == 1) ||
         (abs(message_receive[pacakge_source] - addr_cell[i]) == 15) || (abs(message_receive[pacakge_source] - addr_cell[i]) == 16);
}

byte* i2cCtrl(byte cellId, byte event, byte dest, long curTime) {
    static package message;
    message[pacakge_source] = addr_cell[cellId];
    message[pacakge_event] = event;
    message[pacakge_dest] = dest;
    message[package_current_time_0] = (curTime >> 24) & 0xFF;    //split a long into 4 byte
    message[package_current_time_1] = (curTime >> 16) & 0xFF;
    message[package_current_time_2] = (curTime >> 8) & 0xFF;
    message[package_current_time_3] = curTime & 0xFF;
    Wire.beginTransmission(4);
    Wire.write(message, sizeof(package));
    Wire.endTransmission();
//    Serial.println("Begin");
//    Serial.println(message[0]);
//    Serial.println(message[1]);
//    Serial.println(message[6]);
//    Serial.println("End");
    return message;
}

void monitor(){
    bool monitor_test1 = 0, monitor_test2 = 0;
    for(int i = 0; i < 200; i++) {
        if (digitalRead(monitor_port[0]) == 1) {
            monitor_test1 = 1;
        }
        if (digitalRead(monitor_port[1]) == 1) {
            monitor_test2 = 1;
        }
    }
    if(monitor_test1 == 0 && health[0] == true) {
        memcpy(message_receive, i2cCtrl(0, 254, 0x00, millis() - delayOfNeighbors), 7 * sizeof(byte));
        health[0] = false;
    } else if(monitor_test1 != 0 && health[0] == false) {
        memcpy(message_receive, i2cCtrl(0, 255, 0x00, millis() - delayOfNeighbors), 7 * sizeof(byte));
        health[0] = true;
    } 
    if(monitor_test2 == 0 && health[1] == true) {
        memcpy(message_receive, i2cCtrl(1, 254, 0x00, millis() - delayOfNeighbors), 7 * sizeof(byte));
        health[1] = false;
    } else if(monitor_test2 != 0 && health[1] == false) {
        memcpy(message_receive, i2cCtrl(1, 255, 0x00, millis() - delayOfNeighbors), 7 * sizeof(byte));
        health[1] = true;
    }
}

void moveDetectinCell(bool buttonId){
    const auto reading = digitalRead(port_cell_mov[buttonId]);
    if(reading != lastButtonState[buttonId]) {
        lastDebounceTime = millis();
    }
    if((millis() - lastDebounceTime) > debounceDelay) {
        if(reading != buttonState[buttonId]) {
            buttonState[buttonId] = reading;
            ledState[buttonId] = !ledState[buttonId];
        }
    }
    lastButtonState[buttonId] = reading;
}

int moveDetect() {
    moveDetectinCell(0);
    if(!ledState[0]){
        moveDetectinCell(1);
        if(!ledState[1])
            return -1;
        return 1;
    }
    return 0;
}

//predict the destination
bool destPred(package message, byte cell_eg)
{
    //combine 4 byte into a long
    unsigned long time1 = message[package_current_time_0];
    time1 = time1 << 8 | message[package_current_time_1];
    time1 = time1 << 8 | message[package_current_time_2];
    time1 = time1 << 8 | message[package_current_time_3];
    long t = millis() - time1;
    //  Serial.println(message[0] - cell_eg);
    if(((abs(message[pacakge_source] - cell_eg) == 1 || abs(message[pacakge_source] - cell_eg) == 16)) && (t * 56 >= 28000)){
    //  Serial.println(t);
        return 1;
    }
    if((abs(message[pacakge_source] - cell_eg)==15 || abs(message[pacakge_source] - cell_eg)==17) && ((millis() - time1) * 56 >= 35000)) {
        return 1;
    }
    return 0;
}
