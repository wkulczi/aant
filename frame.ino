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


  jak będziesz wysyłał ładunek to nie kombinuj z konwersją liczby 0-255 na chara tylko po prostu zapisz jako "44" i siema

*/

char preambleSymbol = '@';

char charsInitVal[16] = {(char) 0};

FRAME initFrame() {
  FRAME frame;
  frame.preamble = preambleSymbol;
  return frame;
}

void setFrame(FRAME &frame, int masterId, int slaveId, byte fun, int seq, char payload[16], byte crc) {
  frame.masterId = masterId;
  frame.slaveId = slaveId;
  frame.fun = fun;
  frame.seq = seq;

  assignCharTable(frame.load, payload);

  byteToChar b2c;
  b2c.byteVal = crc;
  frame.crc = b2c.charVal;


}

String frameToString(FRAME frame) {
  String frameStr = "";
  frameStr += frame.preamble;

  intToChar i2c;
  i2c.intVal = frame.masterId;
  frameStr += i2c.charVal;

  i2c.intVal = frame.slaveId;
  frameStr += i2c.charVal;

  byteToChar b2c;
  b2c.byteVal = frame.fun;
  frameStr += b2c.charVal;

  i2c.intVal = frame.seq;
  frameStr += i2c.charVal;

  frameStr += frame.load;
  frameStr += frame.crc;
  return frameStr;
}

//to może kuleć, trzeba sprawdzić
String frameToReadableString(FRAME frame) {
  String frameStr = "";
  frameStr += frame.preamble;
  if (frame.masterId < 10) {
    frameStr += 0;
  }
  frameStr += frame.masterId;
  if (frame.slaveId < 10) {
    frameStr += 0;
  }
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

  frame.preamble = frameStr.charAt(0);

  intToChar i2c;
  i2c.charVal = frameStr.charAt(1);
  frame.masterId = i2c.intVal;

  i2c.charVal = frameStr.charAt(2);
  frame.slaveId = i2c.intVal;

  byteToChar b2c;
  b2c.charVal = frameStr.charAt(3);
  frame.fun = b2c.byteVal;

  i2c.charVal = frameStr.charAt(4);
  frame.seq = i2c.intVal;

  strBuf = frameStr.substring(5, 21);
  strBuf.toCharArray(charBuf, 1);
  assignCharTable(frame.load, charBuf);

  frame.crc = frameStr.charAt(22);

  return frame;
}

void assignCharTable(char *dest, char *from) {
  strcpy(dest, from);
}
