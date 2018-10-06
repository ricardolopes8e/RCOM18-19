
#define SET 000 00011
#define DISC 000 01011
#define UA 000 00111
#define FLAG 01111110
#define ADDSTOR 00000011
#define ADDRTOS 00000001

struct applicationLayer {
  int fileDescriptor;
  int status;
}

struct linkLayer {
  char port[20];
  int baudRate;
  unsigned int sequenceNumber;
  unsigned int timeout;
  unsigned int numTransmissions;

  char frame[MAX_SIZE];
}


int openReceiver(char *port) {

  int fd;

  printf("openReceiver done!\n");
  return fd;
}

int openSender(char* port){
  int fd


  printf("openSender done!\n");
  return fd;

}

int llopen(char* port, int whoIsCalling){
 printf("llopen\n");
 if (whoIsCalling == TRANSMITTER){
  openSender(port);
 } else if (whoIsCalling == RECEIVER){
  openReceiver(port);
 } else {
  return -1;
 }
}

int llwrite(int fd, char* buffer, int length){

}

int llread(int fd, char* buffer){

}

char* createTrama(int whoIsCalling, char* port, int control, int BCC){

  char* trama = (char *)malloc(512);
  strcat (trama, FLAG);

  if (whoIsCalling == SENDER){
    strcat(trama, ADDSTOR);
  } else if (whoIsCalling == RECEIVER){
    strcat(trama, ADDRTOS);
  } else {
    printf("Error: whoIsCalling\n");
    return -1;
  }

  if(control == SET || control == DISC || control == UA || control)

 


}




int openSerialPort(char *port, int whoIsCalling) {
  int fd;
  char serialPort[10] = "/dev/ttyS";
  strcat(serialPort, port);

  if ((strcmp("/dev/ttyS0", serialPort) != 0) &&
      (strcmp("/dev/ttyS1", serialPort) != 0)) {
    printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
    exit(1);
  }

  fd = open(serialPort, O_RDWR | O_NOCTTY);

  if (fd < 0) {
    perror(serialPort); 
    exit(-1); 
  }

  if (tcgetattr(fd, &oldtio) == -1) { /* save current port settings */
    perror("tcgetattr");
    exit(-1);
  }

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;

  if (whoIsCalling == SENDER) {
    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN] = 5;
  } else if (whoIsCalling == RECEIVER) {
    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN] = 5;
  }

  tcflush(fd, TCIOFLUSH);

  if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }

  printf("Serial Port %s opened!", serialPort)

  return fd;
}




