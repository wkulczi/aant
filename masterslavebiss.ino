/*
  Czyli te ID to symulowane są, na sztywno wpisane w kodzie
  W tym punkcie piątym jak jest scenariusz to jest <adres>-<operacja> a nie <adres>-<adres>
  I tyle, chyba na razie wszystko jasne

  O matko ten numer sekwencyjny to już trochę dużo. Jak payload będzie za duży to system musi podzielić go na mniejsze ramki i potem w tym numerze sekwencyjnym mu powiedzieć, że ej to jest pierwsza dopiero :(
  Chyba na razie go pominiemy i zobaczymy co dalej
*/
#include "structs.h";
#include <SPI.h>;
#include <nRF24L01.h>;
#include <RF24.h>;

RF24 radio(7, 8); // CE, CSN
const byte address[6] = "00001";


FRAME masterFrame;
FRAME slaveFrame;
FRAME bufFrame;
/* DEBUG FLAG FOR LOGGING */

DEVICE deviceList[25];
int numOfDevices = 0;
int DEVICE_ID = 8;

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

unsigned long sendTimestamp;

char emptyLoad[16] = {(char) 255};

void setup() {
  setupEmptyTable();
  
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
      setupRadioSend();
      IS_MASTER = 1;
      F_CHOOSEMS = 0;
      F1 = 1;
      SCAN_BEGIN = 1;
    }
    if (ms_choice == 'S' || ms_choice == 's') {
      if (DEBUG_INFO) {
        Serial.println("[DEBUG] device is set to be a SLAVE");
      }
      setupRadioReceive();
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
            //
            //            intToChar i2c;
            //            i2c.charVal = masterFrame.slaveId;
            //            Serial.println(i2c.intVal);

            //send frame
            changeToSend();
            char text[24] = "";
            frameToString(masterFrame).toCharArray(text, 24);
            radio.write(&text, sizeof(text));

            //wait to receive response
            changeToReceive();
            sendTimestamp = millis();
            while (millis() - sendTimestamp < 200) {
              if (radio.available()) {
                char received[24] = "";
                radio.read(&received, sizeof(received));
                slaveFrame = stringToFrame(String(received));
                Serial.println(int(slaveFrame.slaveId));
                Serial.println(slaveFrame.fun);
                byteToChar b2c;
                b2c.byteVal = 0x02;

                if (slaveFrame.slaveId == i && slaveFrame.fun == b2c.charVal) {
                  if (DEBUG_INFO) {
                    Serial.print("[DEBUG] found device with id");
                    Serial.println(i);
                  }

                  DEVICE newDevice;
                  newDevice.id = i;
                  newDevice.nextOp = -1;
                  deviceList[numOfDevices] = newDevice;
                  numOfDevices += 1;
                }
              }
            }
          }
          if (i == 26) {
            //end scan
            SCAN_BEGIN = 0;
            /*Przechodzimy do sekcji z ustalaniem kolejności zadania, nie mam pomysłu jeszcze na to*/
          }
        }
      }
    }
    if (IS_MASTER == 0) {
      changeToReceive();
      if (radio.available()) {
        char received[24] = "";
        radio.read(&received, sizeof(received));
        masterFrame = stringToFrame(String(received));
        
        intToChar i2c;
        i2c.charVal = masterFrame.slaveId;

        byteToChar b2c;
        b2c.charVal = masterFrame.fun;
//todo tomorrow: change the structure of FRAME to hold appropriate values (i dont want to convert this everytime)
//        Serial.println(b2c.byteVal == 0x01);
//        Serial.println(i2c.intVal);

        if (masterFrame.slaveId == DEVICE_ID) {
          setFrame(slaveFrame, DEVICE_ID, DEVICE_ID, 0x02, 1, emptyLoad, 0x00);
          changeToSend();
          //send frame
          char text[24] = "";
          frameToString(slaveFrame).toCharArray(text, 24);
          radio.write(&text, sizeof(text));
        }
      }
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


void setupRadioReceive() {
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();
}

void setupRadioSend() {
  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening();
}


void changeToReceive() {
  radio.openReadingPipe(0, address);
  radio.startListening();
}

void changeToSend() {
  radio.openWritingPipe(address);
  radio.stopListening();
}

/*
Ogólnie historia jest taka, że jak zrobię tablicę pełną (char) 0 
to przy konwersji na stringa on to obcina, więc zrobiłem taką z 255.
Mam nadzieję, że nie będę tej decyzji żałował.
*/
void setupEmptyTable(){
  for(int i = 0; i<=16; i++){
    emptyLoad[i] = (char) 255;
  }
}
