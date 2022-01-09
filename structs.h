struct FRAME {
  char preamble;
  int masterId;
  int slaveId;
  byte fun;
  int seq;
  char load[16];
  char crc; //moze byte, do ustalenia pozniej
};

typedef union {
  int intVal;
  byte byteVal;
} intToByte;

typedef union {
  int intVal;
  char charVal;
} intToChar;

typedef union {
  char charVals[16];
  byte byteVals[16];
} charsToBytes;

typedef union {
  byte byteVal;
  char charVal;
} byteToChar;

struct DEVOP {
  int id;
  int nextOp;
  /*
     maybe will use someday
     int seqnum
  */
};

typedef union {
  char charVals[16];
  uint8_t uintVals[16];
} charsToUints;

typedef union {
  uint8_t uintVal;
  char charVal;
} uintToChar;
