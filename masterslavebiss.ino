/*
  Czyli te ID to symulowane są, na sztywno wpisane w kodzie
  W tym punkcie piątym jak jest scenariusz to jest <adres>-<operacja> a nie <adres>-<adres>
  I tyle, chyba na razie wszystko jasne

  O matko ten numer sekwencyjny to już trochę dużo. Jak payload będzie za duży to system musi podzielić go na mniejsze ramki i potem w tym numerze sekwencyjnym mu powiedzieć, że ej to jest pierwsza dopiero :(
  Chyba na razie go pominiemy i zobaczymy co dalej
*/
#include "structs.h";

FRAME masterFrame;
FRAME slaveFrame;
FRAME bufFrame;
/* DEBUG FLAG FOR LOGGING */

//DEVICE deviceList[25];
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
boolean SCAN_BEGIN = 0;
boolean F2 = 0;
boolean DEBUG_INFO = 1;

char emptyLoad[16] = {(char) 0};

void setup() {
  Serial.begin(9600);
  masterFrame = initFrame();
  slaveFrame = initFrame();
  bufFrame = initFrame();

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
      F_CHOOSEMS = 0;
      F1 = 1;
      SCAN_BEGIN=1;
    }
    if (ms_choice == 'S' || ms_choice == 's') {
      if (DEBUG_INFO) {
        Serial.println("[DEBUG] device is set to be a SLAVE");
      }
      IS_MASTER = 0;
      F_CHOOSEMS = 0;
      F1 = 1;
    }
  }
  if (F1) {
    if (IS_MASTER == 1) {
      if (SCAN_BEGIN) {
        if (DEBUG_INFO) {
          Serial.println("[DEBUG] MASTER: BEGIN SCAN");
        }
        for (int i = 1; i <= 26; i++) {
          if (i != DEVICE_ID && i != 26) {
            setFrame(masterFrame, DEVICE_ID, i, 0x01, 1, emptyLoad, 0x00);
          }
          if (i == 26) {
            SCAN_BEGIN = 0;
          }
        }
      }
      /* 1. Wyślij ramkę cyklu testowania obecności do węzła o numerze N
         2. Odczekaj 200ms na odpowiedź
         3. Jeśli otrzymasz odpowiedź dodaj ten numer do listy aktywnych urządzeń

         No ja bym tu dał listę struktur[25] urządzeń. Jeden argument to numer a drugi to operacja

         Jak to skończy to łapać z serial portu input żeby ustalić sekwencję


       **** Pamiętaj żeby wykluczyć swój ID ***** (chociaż w sumie może nie trzeba?)
      */
    }
    if (IS_MASTER == 0) {
      /*Odbierz ramkę mastera z cyklu testowania obecności węzłów i odeślij swoją asap*/

    }
    if (IS_MASTER == -1) {
      Serial.println("ERR, are you a slave or a master?");
    }
  }
  if (F2) {
    if (IS_MASTER == 1) {
      /*
        0x03
        0x04
        0x06
        0x07
        0x09
        0x0A

        0x0C
      */
    }
    else if (IS_MASTER == 0) {

      /*
         0x05
         0x08
         0x0B

         0x0C
      */

    }
    else {
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
