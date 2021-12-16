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

////////////// Your sensor configuration here //////////////
int potPin = A1;



RF24 radio(7, 8); // CE, CSN
const byte address[6] = "00001";


FRAME masterFrame;
FRAME slaveFrame;
FRAME bufFrame;
/* DEBUG FLAG FOR LOGGING */


int numOfDevices = 0;
DEVOP deviceOpList[25];
int deviceList[25];
int DEVICE_ID = 7;

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
boolean GETOPS = 0;
boolean F2 = 0;
boolean DEBUG_INFO = 1;

unsigned long sendTimestamp;

char emptyLoad[16] = {(char) 255};

void setup() {
  sensorSetup();
  displaySetup();
  setupEmptyTable();
  setupEmptyDevOpTable();
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
          Serial.println("[DEBUG] MASTER: BEGIN  SCAN");
        }
        numOfDevices = 0;
        for (int i = 1; i <= 26; i++) {
          if (i != DEVICE_ID && i != 26) {
            setFrame(masterFrame, DEVICE_ID, i, 0x01, 1, emptyLoad, 0x00);
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

                if (slaveFrame.slaveId == i && slaveFrame.fun == 0x02) {
                  if (DEBUG_INFO) {
                    Serial.print("[DEBUG] found device with id ");
                    Serial.println(i);
                  }
                  deviceList[numOfDevices] = i;
                  numOfDevices += 1;
                }
              }
            }
          }
          if (i == 26) {
            if (numOfDevices > 0) {
              Serial.print("Found devices with ids: ");
              for (int j = 0; j < numOfDevices; j++) {
                Serial.print(deviceList[j]);
                Serial.print(" ");
              }
              Serial.println("");
            }
            //end scan
            SCAN_BEGIN = 0;
            GETOPS = 1;
            /*Przechodzimy do sekcji z ustalaniem kolejności zadania, nie mam pomysłu jeszcze na to*/
          }
        }
      }
      if (GETOPS) {
        if (Serial.available() > 0) {
          String monitorInput = "";
          monitorInput = Serial.readStringUntil('\n');
          parseOpString(monitorInput);
        }
      }
    }
    if (IS_MASTER == 0) {
      changeToReceive();
      if (radio.available()) {
        char received[24] = "";
        radio.read(&received, sizeof(received));
        masterFrame = stringToFrame(String(received));

        if (masterFrame.slaveId == DEVICE_ID && masterFrame.fun == 0x01) {
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
      boolean shouldWork = true;
      int devOpIter = 0;
      while (shouldWork) {
        if (deviceOpList[devOpIter].id == -1) {
          //nie ma więcej operacji w liście, zakończ pętlę
          shouldWork = false;
          break;
        }

        /*
           Sprawdź czy master ma w swojej tablicy urządzeń takie urządzenie, które wpisał użytkownik
        */
        if (containsDevice(deviceOpList[devOpIter].id)) {
          /*
            rób rzeczy
          */

          if (deviceOpList[devOpIter].nextOp == 3) { //0x03 wysłanie danych bez zabezpieczeń (M > S)
            changeToSend();


            /////////////////////////////////
            // fetch data from sensor here //
            /////////////////////////////////

            int arraySize = 16;
            char paddedSensorValue[16] = "";
            readSensorToCharTable(paddedSensorValue);


            for (int i = 0; i < 16; i++) {
              Serial.print(paddedSensorValue[i]);
            }
            Serial.println(" ");


            char load[16]; //insert load instead of emptyLoad of course
            setFrame(masterFrame, DEVICE_ID, deviceOpList[devOpIter].id, 0x03, 1, paddedSensorValue, 0x00);


          }
          if (deviceOpList[devOpIter].nextOp == 4) { //0x04 żądanie przesyłania danych ze Slave bez zabezpieczeń (M > S)


            changeToSend();

            setFrame(masterFrame, DEVICE_ID, deviceOpList[devOpIter].id, 0x04, 1, emptyLoad, 0x00);
            //send this shit


            changeToReceive();
            ///wait for signal


            /////////////////////////
            // show data on screen //
            /////////////////////////

          }
          if (deviceOpList[devOpIter].nextOp == 6) { //0x06 wysłanie danych z CRC8 (M > S)
            //nah man, not happening

          }
          if (deviceOpList[devOpIter].nextOp == 7) { //0x07 żądanie przesyłania danych ze Slave z CRC8 (M > S).

          }
          if (deviceOpList[devOpIter].nextOp == 9) { //0x09 wysłanie danych z CRC8 i szyfrowaniem AES128 (M > S)

          }
          if (deviceOpList[devOpIter].nextOp == 10) { //0x0A żądanie przesyłania danych ze Slave z CRC8 i szyfrowaniem AES128 (M > S).

          }
          //to nie będzie w deviceOp a zawsze trzeba na to poczekać
          //if (deviceOpList[devOpIter].nextOp == 12) { //0x0C potwierdzenie odbioru danych z węzła Master lub Slave.

          //}
        }

        //else: nie rób nic, przejdź do następnej operacji w liście
        //op  eracje zakończone, przejdź do następnej
        devOpIter += 1;
      }
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
void setupEmptyTable() {
  for (int i = 0; i <= 16; i++) {
    emptyLoad[i] = (char) 255;
  }
}


void setupEmptyDevOpTable() {
  DEVOP buf;
  buf.id = -1;
  for (int i = 0; i <= 25; i++) {
    deviceOpList[i] = buf;
  }
}
/*
    Please insert strings in this manner:
    DD-OO,DD-00,DD-00
    DD - device
    OO - operation
*/
void parseOpString(String input) {
  /*
     I know it could be better, but it's late, okay?
  */
  DEVOP buf;
  String str = input;
  String strs[20];
  int stringCount = 0;
  int devOpIter = 0;

  while (str.length() > 0) {
    int commaIndex = str.indexOf(',');
    if (commaIndex == -1) {
      strs[stringCount++] = str;
      break;
    }
    else {
      strs[stringCount++] = str.substring(0, commaIndex);
      str = str.substring(commaIndex + 1);
    }
  }

  for (int i = 0; i < stringCount; i++)
  {
    String devop = strs[i];
    int dashIndex = str.indexOf('-');

    buf.id = devop.substring(0, dashIndex).toInt();
    buf.nextOp = devop.substring(dashIndex + 1).toInt();

    deviceOpList[devOpIter++] = buf;
  }
}

boolean containsDevice(int deviceId) {
  for (int i = 0; i < numOfDevices; i++) {
    if (deviceList[i] == deviceId) {
      return true;
    }
  }
  return false;
}

void sensorSetup() {
  pinMode(potPin, INPUT);
}

int readSensor() {
  return analogRead(potPin);
}

void readSensorToCharTable(char *table) {
  int sensorValue = readSensor();
  String sensorValueAsString = String(sensorValue);

  while (sensorValueAsString.length() < 16) {
    sensorValueAsString += char (255);
  }

  char sensorValueAsCharTable[16] = "";
  sensorValueAsString.toCharArray(sensorValueAsCharTable, sensorValueAsString.length());

  strcpy(table, sensorValueAsCharTable);

}

void displaySetup() {
}
