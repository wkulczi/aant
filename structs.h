struct FRAME {
  char preamble;
  char masterId;
  char slaveId;
  char fun;
  char seq;
  char load[16];
  char crc;
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

struct DEVICE {
  int id;
  int nextOp;
  /*
     maybe will use someday
     int seqnum
  */
};
