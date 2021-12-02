/*
  RAMKA

  znak
  początku
  komunikacji
  ----------|----------|----------|---------|-------------------|---------|-----
  Preambuła   MasterID    SlaveID   Funkcja   Numer sekwencyjny   Ładunek   CRC
    1B          1B          1B         1B           1B              16B      1B


*/

char preambleSymbol = '@';

char charsInitVal[16] = {(char) 0};


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
