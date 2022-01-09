#include <FastCRC.h>
#include "structs.h"
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

FastCRC8 CRC8;
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
int DEVOPITER = 0;
int f2devOpIter = 0;
int deviceList[25];
int DEVICE_ID = 5;

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
boolean F2_SETUP;
boolean F2_READ;
boolean F2_WRITE;
boolean IS_BUSY = 0;
boolean DEBUG_INFO = 1;

unsigned long sendTimestamp;

char emptyLoad[16] = {(char)45};

unsigned long WAIT_FOR_ACK_TIME = 2000;

void setup()
{
  sensorSetup();
  displaySetup();
  setupEmptyTable();
  setupEmptyDevOpTable();
  F1 = 0;
  F2 = 0;
  F2_SETUP = 1;
  F2_READ = 0;
  F2_WRITE = 0;
  numOfDevices = 0;
  F_CHOOSEMS = 1;
  Serial.begin(9600);
  masterFrame = initFrame();
  slaveFrame = initFrame();
  bufFrame = initFrame();
  WAIT_FOR_ACK_TIME = 800;
  if (DEBUG_INFO)
  {
    Serial.print("[DEBUG] Device with id ");
    Serial.print(DEVICE_ID);
    Serial.println(" is ready.");
  }
}

void loop()
{
  if (F_CHOOSEMS)
  {
    numOfDevices = 0;
    recvWithEndMarker();
    showNewData();
    if (ms_choice == 'M' || ms_choice == 'm')
    {
      if (DEBUG_INFO)
      {
        Serial.println("[DEBUG] device is set to be a MASTER");
      }
      setupRadioSend();
      IS_MASTER = 1;
      F_CHOOSEMS = 0;
      F1 = 1;
      SCAN_BEGIN = 1;
    }
    if (ms_choice == 'S' || ms_choice == 's')
    {
      if (DEBUG_INFO)
      {
        Serial.println("[DEBUG] device is set to be a SLAVE");
      }
      setupRadioReceive();
      IS_MASTER = 0;
      F_CHOOSEMS = 0;
      F1 = 1;
    }
  }
  if (F1 > 0)
  {
    if (IS_MASTER == 1)
    {
      if (SCAN_BEGIN)
      {
        if (DEBUG_INFO)
        {
          Serial.println("[DEBUG] MASTER: BEGIN  SCAN");
          //          Serial.println(numOfDevices); => 0
        }
        for (int i = 1; i <= 26; i++)
        {
          if (i != DEVICE_ID && i != 26)
          {
            setFrame(masterFrame, DEVICE_ID, i, 0x01, 1, emptyLoad, 0);
            // send frame
            slaveFrame = stringToFrame(frameToString(masterFrame));
            changeToSend();
            char text[24] = "";
            frameToString(masterFrame).toCharArray(text, 24);
            radio.write(&text, sizeof(text));

            // wait to receive response
            changeToReceive();
            sendTimestamp = millis();
            while (millis() - sendTimestamp < 200)
            {
              if (radio.available())
              {
                char received[24] = "";
                radio.read(&received, sizeof(received));
                slaveFrame = stringToFrame(String(received));

                if (slaveFrame.slaveId == i && slaveFrame.fun == 0x02)
                {
                  if (DEBUG_INFO)
                  {
                    Serial.print("[DEBUG] found device with id ");
                    Serial.print(i);
                    Serial.print(" ");
                    Serial.println(numOfDevices);
                  }
                  deviceList[numOfDevices] = i;
                  //                  Serial.println(numOfDevices);  = > 3
                  numOfDevices = numOfDevices + 1;
                  //                  Serial.println(numOfDevices); => 4
                }
              }
            }
          }
          if (i == 26)
          {
            if (numOfDevices > 0)
            {
              // todo rozwiąż zagadkę dlaczego numOfDevices w którymś miejscu wskakuje do wartości 3
              Serial.print("Found devices with ids: ");
              for (int j = 0; j < numOfDevices; j++) {
                Serial.print(deviceList[j]);
                Serial.print(" ");
              }
              //              if (DEBUG_INFO)
              //              {
              //                Serial.println("");
              //                Serial.print("Found ");
              //                Serial.print(numOfDevices);
              //                Serial.println(" devices.");
              //              }
            }
            // end scan
            if (DEBUG_INFO)
            {
              Serial.println("FINISHED SCANNING");
              Serial.println("WAITING FOR USER INPUT.");
              Serial.println("FOLLOW THE STRUCTURE: DD-OO,DD-OO,DD-OO,..");
              Serial.println("WHERE DD - DEVICE NUMBER");
              Serial.println("WHERE OO - OPERATION NUMBER");
            }
            SCAN_BEGIN = 0;
            GETOPS = 1;
          }
        }
      }
      if (GETOPS)
      {
        if (Serial.available() > 0)
        {
          String monitorInput = "";
          monitorInput = Serial.readStringUntil('\n');
          parseOpString(monitorInput);
          if (DEVOPITER > 0)
          {
            GETOPS = 0;
            F1 = 0;
            F2 = 1;
            f2devOpIter = 0;
            if (DEBUG_INFO)
            {
              Serial.println("LEAVING F1");
            }
          }
        }
      }
    }
    if (IS_MASTER == 0)
    {
      changeToReceive();
      if (radio.available())
      {
        char received[24] = "";
        radio.read(&received, sizeof(received));
        masterFrame = stringToFrame(String(received));

        if (masterFrame.slaveId == DEVICE_ID && masterFrame.fun == 0x01)
        {
          setFrame(slaveFrame, DEVICE_ID, DEVICE_ID, 0x02, 1, emptyLoad, 0);
          changeToSend();
          // send frame
          char text[24] = "";
          frameToString(slaveFrame).toCharArray(text, 24);
          radio.write(&text, sizeof(text));

          Serial.println("LEAVING F1, AWAITING COMMAND");

          F1 = 0;
          F2 = 1;
          IS_BUSY = 0;
        }
      }
      /*Odbierz ramkę mastera z cyklu testowania obecności węzłów i odeślij swoją asap*/
    }
    if (IS_MASTER == -1)
    {
      Serial.println("ERR, are you a slave or a master?");
    }
  }
  if (F2)
  {
    if (IS_MASTER == 1)
    {
      if (deviceOpList[f2devOpIter].id == -1)
      {
        F2 = 0;
        Serial.println("leaving F2");
      }
      /*
           Sprawdź czy master ma w swojej tablicy urządzeń takie urządzenie, które wpisał użytkownik
      */
      if (containsDevice(deviceOpList[f2devOpIter].id))
      {
        /*
            rób rzeczy
        */
        if (deviceOpList[f2devOpIter].nextOp == 3)
        { // 0x03 wysłanie danych bez zabezpieczeń (M > S)
          if (F2_SETUP)
          {
            F2_READ = 0;
            F2_WRITE = 1;
            F2_SETUP = 0;
          }
          if (F2_WRITE)
          {
            changeToSend();

            /////////////////////////////////
            // fetch data from sensor here //
            /////////////////////////////////

            int arraySize = 16;
            char paddedSensorValue[16] = "";
            readSensorToCharTable(paddedSensorValue);

            if (DEBUG_INFO)
            {
              Serial.print("sending value (with padding): ");
              for (int i = 0; i < 16; i++)
              {
                Serial.print(paddedSensorValue[i]);
              }
              Serial.println(" ");
            }

            char load[16]; // insert load instead of emptyLoad of course
            setFrame(masterFrame, DEVICE_ID, deviceOpList[f2devOpIter].id, 0x03, 1, paddedSensorValue, 0);

            // send frame
            char text[24] = "";
            frameToString(masterFrame).toCharArray(text, 24);
            Serial.println(frameToReadableString(masterFrame));
            radio.write(&text, sizeof(text));

            F2_WRITE = 0;
            F2_READ = 1;
            sendTimestamp = millis();
          }
          if (F2_READ)
          {
            changeToReceive();
            if (radio.available())
            {
              char received[24] = "";
              radio.read(&received, sizeof(received));
              slaveFrame = stringToFrame(String(received));
              Serial.println(frameToReadableString(slaveFrame));
              if (slaveFrame.slaveId == deviceOpList[f2devOpIter].id && slaveFrame.fun == 0x0C)
              {
                if (DEBUG_INFO)
                {
                  Serial.println("[DEBUG] received ACK from slave");
                }
                F2_WRITE = 0;
                f2devOpIter += 1;
                F2_SETUP = 1;
              }
            }
            if (millis() - sendTimestamp > WAIT_FOR_ACK_TIME)
            {
              if (DEBUG_INFO)
              {
                Serial.print("[DEBUG] ACK ");
                Serial.print(deviceOpList[f2devOpIter].id);
                Serial.println(" not received!");
              }
              Serial.println("waiting for ACK timed out, retrying operation");
              F2_SETUP = 1;
            }
          }
        }
        if (deviceOpList[f2devOpIter].nextOp == 4)
        { // 0x04 żądanie przesyłania danych ze Slave bez zabezpieczeń (M > S)
          if (F2_SETUP)
          {
            // todo może dodaj fazę WAIT4ACK czy coś
            changeToSend();
            setFrame(masterFrame, DEVICE_ID, deviceOpList[f2devOpIter].id, 0x04, 1, emptyLoad, 0);
            char text[24] = "";
            frameToString(masterFrame).toCharArray(text, 24);
            Serial.println(frameToReadableString(masterFrame));
            radio.write(&text, sizeof(text));

            F2_SETUP = 0;
            F2_READ = 1;
            F2_WRITE = 0;
          }
          if (F2_READ)
          {
            changeToReceive();
            if (radio.available())
            {
              char received[24] = "";
              radio.read(&received, sizeof(received));
              slaveFrame = stringToFrame(String(received));
              Serial.println(frameToReadableString(slaveFrame));
              if (slaveFrame.slaveId == deviceOpList[f2devOpIter].id && slaveFrame.fun == 0x05)
              {
                Serial.print("Received value: ");
                Serial.println(trimLoadPadding(slaveFrame.load));
                /////////////////////////
                // show data on screen //
                /////////////////////////
                F2_READ = 0;
                F2_WRITE = 1;
              }
            }
          }
          if (F2_WRITE)
          {
            // send ACK
            changeToSend();
            setFrame(masterFrame, DEVICE_ID, deviceOpList[f2devOpIter].id, 0x0C, 1, slaveFrame.load, 0);
            char text[24] = "";
            Serial.print("Sending ack from master: ");
            Serial.println(frameToReadableString(masterFrame));
            frameToString(slaveFrame).toCharArray(text, 24);
            radio.write(&text, sizeof(text));
            F2_SETUP = 1;
            f2devOpIter += 1;
          }
        }
        // 0x06 wysłanie danych z CRC8 (M > S)
        if (deviceOpList[f2devOpIter].nextOp == 6)
        {
          if (F2_SETUP)
          {
            Serial.println("0x06 letsgoo");
            F2_WRITE = 1;
            F2_READ = 0;
            F2_SETUP = 0;
          }
          if (F2_WRITE)
          {
            // send data with CRC
            changeToSend();

            int arraySize = 16;
            char paddedSensorValue[16] = "";
            readSensorToCharTable(paddedSensorValue);

            charsToUints c2u;
            for (int i = 0; i < sizeof(paddedSensorValue); i++) {
              c2u.charVals[i] = paddedSensorValue[i];
            }

            uint8_t crc8val = CRC8.smbus(c2u.uintVals, sizeof(c2u.uintVals));
            Serial.print("crc val: ");
            Serial.println(crc8val);
            setFrame(masterFrame, DEVICE_ID, deviceOpList[f2devOpIter].id, 0x06, 1, paddedSensorValue, crc8val);

            char text[24] = "";

            frameToString(masterFrame).toCharArray(text, 24);
            radio.write(&text, sizeof(text));

            F2_WRITE = 0;
            F2_READ = 1;
          }
          if (F2_READ)
          {
            changeToReceive();
            if (radio.available())
            {
              char received[24] = "";
              radio.read(&received, sizeof(received));
              slaveFrame = stringToFrame(String(received));
              Serial.println(frameToReadableString(slaveFrame));
              if (slaveFrame.slaveId == deviceOpList[f2devOpIter].id && slaveFrame.fun == 0x0C)
              {
                if (DEBUG_INFO)
                {
                  Serial.println("[DEBUG] received ACK from slave");
                }
                F2_WRITE = 0;
                f2devOpIter += 1;
                F2_SETUP = 1;
              }
            }
            if (millis() - sendTimestamp > WAIT_FOR_ACK_TIME)
            {
              if (DEBUG_INFO)
              {
                Serial.print("[DEBUG] ACK ");
                Serial.print(deviceOpList[f2devOpIter].id);
                Serial.println(" not received!");
              }
              Serial.println("waiting for ACK timed out, retrying operation");
              F2_SETUP = 1;
            }
          }
        }
        if (deviceOpList[f2devOpIter].nextOp == 7)
        { // 0x07 żądanie przesyłania danych ze Slave z CRC8 (M > S).
        }
        if (deviceOpList[f2devOpIter].nextOp == 9)
        { // 0x09 wysłanie danych z CRC8 i szyfrowaniem AES128 (M > S)
        }
        if (deviceOpList[f2devOpIter].nextOp == 10)
        { // 0x0A żądanie przesyłania danych ze Slave z CRC8 i szyfrowaniem AES128 (M > S).
        }
        // to nie będzie w deviceOp a zawsze trzeba na to poczekać
        // if (deviceOpList[f2devOpIter].nextOp == 12) { //0x0C potwierdzenie odbioru danych z węzła Master lub Slave.

        //}

        // else: nie rób nic, przejdź do następnej operacji w liście
        // op  eracje zakończone, przejdź do następnej
        //         f2devOpIter += 1;
      }
      else
      {
        f2devOpIter = f2devOpIter + 1;
      }
    }
    else if (IS_MASTER == 0)
    {
      if (IS_BUSY == 0)
      {
        changeToReceive();
        if (radio.available())
        {
          char received[24] = "";
          radio.read(&received, sizeof(received));
          masterFrame = stringToFrame(String(received));
          if (masterFrame.slaveId == DEVICE_ID)
          {
            IS_BUSY = 1;
            Serial.println("received frame for me, busy now");
          }
        }
      }
      else
      {
        if (masterFrame.fun == 0x03)
        {
          if (F2_SETUP)
          {
            Serial.print("Received value: ");
            Serial.println(trimLoadPadding(masterFrame.load));
            changeToSend();
            F2_SETUP = 0;
            F2_READ = 0;
            F2_WRITE = 1;
          }
          if (F2_WRITE)
          {
            // send ACK
            setFrame(slaveFrame, masterFrame.masterId, DEVICE_ID, 0x0C, 1, masterFrame.load, 0);
            char text[24] = "";
            frameToString(slaveFrame).toCharArray(text, 24);
            radio.write(&text, sizeof(text));
            F2_SETUP = 1;
            IS_BUSY = 0;
          }
        }
        if (masterFrame.fun == 0x04)
        {
          if (F2_SETUP)
          {
            // fetch value
            // send value
            changeToSend();
            int arraySize = 16;
            char paddedSensorValue[16] = "";
            readSensorToCharTable(paddedSensorValue);
            char load[16];
            setFrame(slaveFrame, masterFrame.masterId, DEVICE_ID, 0x05, 1, paddedSensorValue, 0);

            char text[24] = "";
            frameToString(slaveFrame).toCharArray(text, 24);
            Serial.println(frameToReadableString(slaveFrame));
            radio.write(&text, sizeof(text));

            F2_SETUP = 0;
            F2_WRITE = 0;
            changeToReceive();
            F2_READ = 1;
            sendTimestamp = millis();
            Serial.println("sent value from slave, 0x04");
          }
          if (F2_READ) // wait for ack
          {
            if (radio.available())
            {
              char received[24] = "";
              radio.read(&received, sizeof(received));
              masterFrame = stringToFrame(String(received));
              Serial.println(frameToReadableString(masterFrame));
              if (masterFrame.fun == 0x0C)
              {
                if (DEBUG_INFO)
                {
                  Serial.println("[DEBUG] received ACK from master");
                }
                F2_SETUP = 1;
                F2_READ = 0;
                IS_BUSY = 0;
              }
            }
          }
        }
      }
      if (masterFrame.fun == 0x06)
      {
        if (F2_SETUP)
        {
          F2_SETUP = 0;
          F2_READ = 1;
          F2_WRITE = 0;
        }
        if (F2_READ)
        {
          charsToUints c2u;
          for (int i = 0; i < sizeof(masterFrame.load); i++) {
            c2u.charVals[i] = masterFrame.load[i];
          }
          uint8_t crc = CRC8.smbus(c2u.uintVals, sizeof(c2u.uintVals));
          if (crc == masterFrame.crc)
          {
            Serial.println("crc is ok.");

            // read data with crc
            Serial.print("Received value: ");
            Serial.println(trimLoadPadding(masterFrame.load));
            // if crc is right:
            F2_READ = 0;
            changeToSend();
            F2_WRITE = 1;
          }
          else
          {
            Serial.println("CRC WRONG :((");
          }
        }
        if (F2_WRITE)
        {
          // send ACK
          setFrame(slaveFrame, masterFrame.masterId, DEVICE_ID, 0x0C, 1, masterFrame.load, 0);
          char text[24] = "";
          frameToString(slaveFrame).toCharArray(text, 24);
          radio.write(&text, sizeof(text));
          F2_SETUP = 1;
          IS_BUSY = 0;
        }
      }
      /*
         0x05
         0x08
         0x0B

         0x0C
      */
    }
    //    else {
    //      Serial.println("ERR, are you a slave or a master?");
    //    }
  }
}

void recvWithEndMarker()
{
  static byte ndx = 0;
  char endMarker = '\n';
  char rc;

  // if (Serial.available() > 0) {
  while (Serial.available() > 0 && newData == false)
  {
    rc = Serial.read();

    if (rc != endMarker)
    {
      receivedChars[ndx] = rc;
      ndx++;
      if (ndx >= numChars)
      {
        ndx = numChars - 1;
      }
    }
    else
    {
      receivedChars[ndx] = '\0'; // terminate the string
      ndx = 0;
      newData = true;
    }
  }
}

void showNewData()
{
  if (newData == true)
  {

    if (DEBUG_INFO)
    {
      Serial.print("[DEBUG] Device received data: ");
      Serial.println(receivedChars);
    }

    ms_choice = receivedChars[0];
    newData = false;
  }
}

void setupRadioReceive()
{
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();
}

void setupRadioSend()
{
  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening();
}

void changeToReceive()
{
  radio.openReadingPipe(0, address);
  radio.startListening();
}

void changeToSend()
{
  radio.openWritingPipe(address);
  radio.stopListening();
}

/*
  Ogólnie historia jest taka, że jak zrobię tablicę pełną (char) 0
  to przy konwersji na stringa on to obcina, więc zrobiłem taką z 255.
  Mam nadzieję, że nie będę tej decyzji żałował.
*/
void setupEmptyTable()
{
  for (int i = 0; i < 16; i++)
  {
    emptyLoad[i] = (char)45;
  }
}

void setupEmptyDevOpTable()
{
  DEVOP buf;
  buf.id = -1;
  for (int i = 0; i <= 25; i++)
  {
    deviceOpList[i] = buf;
  }
}
/*
    Please insert strings in this manner:
    DD-OO,DD-00,DD-00
    DD - device
    OO - operation
*/
void parseOpString(String input)
{
  /*
     I know it could be better, but it's late, okay?
  */
  DEVOP buf;
  String str = input;
  String strs[20];
  int stringCount = 0;
  DEVOPITER = 0;

  while (str.length() > 0)
  {
    int commaIndex = str.indexOf(',');
    if (commaIndex == -1)
    {
      strs[stringCount++] = str;
      break;
    }
    else
    {
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

    deviceOpList[DEVOPITER++] = buf;
  }
}

boolean containsDevice(int deviceId)
{
  for (int i = 0; i <= numOfDevices; i++)
  {
    if (deviceList[i] == deviceId)
    {
      return true;
    }
  }
  Serial.println("");
  return false;
}

void sensorSetup()
{
  pinMode(potPin, INPUT);
}

int readSensor()
{
    return analogRead(potPin);
}

void readSensorToCharTable(char *table)
{
  int sensorValue = readSensor();
  String sensorValueAsString = String(sensorValue);

  while (sensorValueAsString.length() <= 16)
  {
    sensorValueAsString += char(45);
  }

  Serial.println(sensorValueAsString);

  char sensorValueAsCharTable[16] = "";
  sensorValueAsString.toCharArray(sensorValueAsCharTable, 16);

  strcpy(table, sensorValueAsCharTable);
}

String trimLoadPadding(char *table)
{
  String buf = "";
  for (int i = 0; i < 16; i++)
  {
    if (table[i] != char(45))
    {
      buf += table[i];
    }
  }
  return buf;
}

void displaySetup()
{
}
