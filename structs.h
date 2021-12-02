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
  char charVals[16];
  byte byteVals[16];
} charsToBytes;
