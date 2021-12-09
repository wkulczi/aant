/*
  RAMKA

  znak
  początku
  komunikacji
  ----------|----------|----------|---------|-------------------|---------|-----
  Preambuła   MasterID    SlaveID   Funkcja   Numer sekwencyjny   Ładunek   CRC
    1B          1B          1B         1B           1B              16B      1B


0x01 – test obecności węzła Slave (wysyła Master).
0x02 – zgłoszenie obecności (S > M).
0x03 – wysłanie danych bez zabezpieczeń (M > S).
0x04 – żądanie przesyłania danych ze Slave bez zabezpieczeń (M > S).
0x05 – przesyłanie danych ze Slave bez zabezpieczeń (S > M).
0x06 – wysłanie danych z CRC8 (M > S).
0x07 – żądanie przesyłania danych ze Slave z CRC8 (M > S).
0x08 – przesłanie danych ze Slave z CRC8 (S > M).
0x09 – wysłanie danych z CRC8 i szyfrowaniem AES128 (M > S).
0x0A – żądanie przesyłania danych ze Slave z CRC8 i szyfrowaniem AES128 (M > S).
0x0B – przesłanie danych ze Slave z CRC8 i szyfrowaniem AES128 (S > M).
0x0C – potwierdzenie odbioru danych z węzła Master lub Slave.

*/

char preambleSymbol = '@';

char charsInitVal[16] = {(char) 0};

FRAME initFrame() {
  FRAME frame;
  frame.preamble = preambleSymbol;
  return frame;
}

void setFrame(FRAME &frame, int masterId, int slaveId, byte fun, int seq, char payload[25], byte crc){
 intToChar i2c;
 i2c.intVal = masterId;

 frame.masterId = i2c.charVal;

 i2c.intVal = slaveId;
 frame.slaveId = i2c.charVal;

 byteToChar b2c;
 b2c.byteVal = fun;
 frame.fun = b2c.charVal;

 frame.seq = '1';
 assignCharTable(frame.load, payload);
 
 b2c.byteVal = crc;
 frame.crc = b2c.charVal;
}

String frameToString(FRAME frame) {
  String frameStr = "";
  frameStr += frame.preamble;
  frameStr += frame.masterId;
  frameStr += frame.slaveId;
  frameStr += frame.fun;
  frameStr += frame.seq;
  frameStr += frame.load;
  frameStr += frame.crc;
  return frameStr;
}

FRAME stringToFrame(String frameStr) {
  FRAME frame;
  String strBuf = "";
  char charBuf[16] = {(char) 0};

  strBuf = frameStr.substring(0, 0);
  strBuf.toCharArray(charBuf, 1);
  frame.preamble = charBuf[0];

  strBuf = frameStr.substring(1, 1);
  strBuf.toCharArray(charBuf, 1);
  frame.masterId = charBuf[0];

  strBuf = frameStr.substring(2, 2);
  strBuf.toCharArray(charBuf, 1);
  frame.slaveId = charBuf[0];

  strBuf = frameStr.substring(3, 3);
  strBuf.toCharArray(charBuf, 1);
  frame.fun = charBuf[0];

  strBuf = frameStr.substring(4, 4);
  strBuf.toCharArray(charBuf, 1);
  frame.seq = charBuf[0];

  strBuf = frameStr.substring(5, 15);
  strBuf.toCharArray(charBuf, 1);
  assignCharTable(frame.load, charBuf);


  strBuf = frameStr.substring(16, 16);
  strBuf.toCharArray(charBuf, 1);
  frame.crc = charBuf[0];

  return frame;
}

void assignCharTable(char *dest, char *from) {
  strcpy(dest, from);
}
