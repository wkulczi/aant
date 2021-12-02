/*
  Czyli te ID to symulowane są, na sztywno wpisane w kodzie
  W tym punkcie piątym jak jest scenariusz to jest <adres>-<operacja> a nie <adres>-<adres>
  I tyle, chyba na razie wszystko jasne
*/
#include "structs.h"

boolean DEBUG_INFO = 1;


int DEVICE_ID = 4;


/* Faza wyboru typu urządzenia master/slave*/
boolean F_CHOOSEMS = 1;
const byte numChars = 32;
char receivedChars[numChars]; // an array to store the received data
boolean newData = false;
char ms_choice = 'A';

int IS_MASTER = -1;

/*Faza pierwsza - skanowanie urządzeń przez MASTER*/
boolean F1 = 0;

FRAME testframe;

void setup() {
  Serial.begin(9600);

  if (DEBUG_INFO) {
    Serial.print("[DEBUG] Device with id ");
    Serial.print(DEVICE_ID);
    Serial.println(" is ready.");
  }

}

void loop() {
  if (F_CHOOSEMS) {
    recvWithEndMarker();
    showNewData();
    if (ms_choice == 'M' || ms_choice == 'm') {
      if (DEBUG_INFO) {
        Serial.println("[DEBUG] device is set to be a MASTER");
      }
      IS_MASTER = 1;
    }
    if (ms_choice == 'S' || ms_choice == 's') {
      if (DEBUG_INFO) {
        Serial.println("[DEBUG] device is set to be a SLAVE");
      }
      IS_MASTER = 0;
    }
    F_CHOOSEMS = 0;
    F1 = 1;
  }
  if (F1) {
    if (IS_MASTER == 1) {
      
    }
    if (IS_MASTER == 0) {

    }
    if (IS_MASTER == -1) {
      Serial.println("ERR, are you a slave or a master?");
    }
  }
}



void recvWithEndMarker() {
  static byte ndx = 0;
  char endMarker = '\n';
  char rc;

  // if (Serial.available() > 0) {
  while (Serial.available() > 0 && newData == false) {
    rc = Serial.read();

    if (rc != endMarker) {
      receivedChars[ndx] = rc;
      ndx++;
      if (ndx >= numChars) {
        ndx = numChars - 1;
      }
    }
    else {
      receivedChars[ndx] = '\0'; // terminate the string
      ndx = 0;
      newData = true;
    }
  }
}

void showNewData() {
  if (newData == true) {

    if (DEBUG_INFO) {
      Serial.print("[DEBUG] Device received data: ");
      Serial.println(receivedChars);
    }

    ms_choice = receivedChars[0];
    newData = false;
  }
}
