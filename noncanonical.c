/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FLAG 126 /* 0x7E */
#define A 3 /* 0x03 */
#define SET_C 3 /* 0x03 */
#define UA_C 7 /* 0x07 */
#define DISC_C 11 /* 0x0B */
#define FALSE 0
#define TRUE 1
#define ERR -1
#define START 0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define BCC_OK 4
#define STOP_STATE 5
#define UA_SIZE 5
#define SET_SIZE 5


volatile int STOP = FALSE;
struct termios oldtio, newtio;

void print_hexa(char *str) {
  int j;
  for (j = 0; j < strlen(str); j++) {
    printf("%0xh ", str[j]);  
  }
  printf("\n");
}

/* state machine */
int read_control_message(int fd) {
  int state = START;
  char *buffer = (char*) malloc((SET_SIZE + 1) * sizeof(char));
  char c;
  int i = 0;

  while (state != STOP_STATE) {
    read(fd, &c, 1);
    printf("Character received: %0xh\n", c);

    switch (state) {
      case START:
        /* FLAG_RCV */
        if (c == FLAG) {
          buffer[i++] = c;
          state = FLAG_RCV;
        }
        break;

      case FLAG_RCV:
        /* A_RCV */
        if (c == A) {
          state = A_RCV;
          buffer[i++] = c;
        }
        else if (c == FLAG)
          state = FLAG_RCV;
        else {
          memset(buffer, 0, SET_SIZE + 1);
          i = 0;
          state = START;
        }
        break;

      case A_RCV:
        /* C_RCV */
        if (c == SET_C) {
          state = C_RCV;
          buffer[i++] = c;
        }
        else if (c == FLAG)
          state = FLAG_RCV;
        else {
          memset(buffer, 0, SET_SIZE + 1);
          i = 0;
          state = START;
        }
        break;

      case C_RCV:
        /* BCC_OK */
        if (c == (A ^ SET_C)) {
          state = BCC_OK;
          buffer[i++] = c;
        }
        else {
          memset(buffer, 0, SET_SIZE + 1);
          i = 0;
          state = START;
        }
        break;

      case BCC_OK:
        /* FLAG_RCV */
        if (c == FLAG) {
          printf("Final flag received. Message received completely\n");
          state = STOP_STATE;
          buffer[i++] = c;
        } else {
          memset(buffer, 0, SET_SIZE + 1);
          i = 0;
          state = START;
        }
        break;
      }
  }

  /* print received buffer */
  printf("Received: ");
  print_hexa(buffer);
  return TRUE;
}

void send_control_message(int fd, int C) {
  unsigned char UA[UA_SIZE];

  UA[0] = FLAG;
  UA[1] = A;
  UA[2] = C;
  UA[3] = A ^ C;
  UA[4] = FLAG;

  printf("Transmit: ");
  print_hexa(UA);
  write(fd, UA, UA_SIZE);
}

int llopen(int fd) {

  if (tcgetattr(fd, &oldtio) == ERR) { /* save current port settings */
      perror("tcgetattr");
      exit(ERR);
    }

    /* initialize with 0 the memory for the newtio structure */
    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 1;   /* inter-character timer value */
    newtio.c_cc[VMIN]  = 0;  /* The read will be satisfied if a single
                              character is read, or TIME is exceeded
                              (t = TIME *0.1 s). If TIME is exceeded,
                              no character will be returned. */
    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == ERR) {
      perror("tcsetattr error");
      exit(ERR);
    }
    printf("New termios structure set\n");

    /*********** until here the code is provided on moodle ***********/
    
    if (read_control_message(fd)) {
      printf("Received SET\n");
      send_control_message(fd, UA_C);
      printf("Sent UA\n");
    }
}

void llclose(int fd){
  char c;
  int state;

  read(fd,&c,1);
  state = DISC_C;
  state_machine_UA(&state,&c);

  send_control_message(fd, state);

  read(fd,&c,1);
  state = UA_C;
  state_machine_UA(&state,&c);

  tcsetattr(fd, TCSANOW, &oldtio);

}

int main(int argc, char** argv) {
  int fd, c, res;
  char buf[255];

  if ((argc < 2) || 
	   ((strcmp("/dev/ttyS0", argv[1])!=0) && 
	    (strcmp("/dev/ttyS1", argv[1])!=0) )) {
    printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
    exit(1);
  }


/*
  Open serial port device for reading and writing and not as controlling tty
  because we don't want to get killed if linenoise sends CTRL-C.
*/

  fd = open(argv[1], O_RDWR | O_NOCTTY);
  if (fd <0) {
    perror(argv[1]);
    exit(ERR);
  }

  llopen(fd);

  /****************************************
  
  while (STOP==FALSE) {       /* loop for input
    res = read(fd,buf,255);   /* returns after 5 chars have been input
    buf[res]=0;               /* so we can printf...
    printf(":%s:%d\n", buf, res);
    if (buf[0]=='z') STOP=TRUE;
  }
/* 
  O ciclo WHILE deve ser alterado de modo a respeitar o indicado no guião 
*/

  tcsetattr(fd, TCSANOW, &oldtio);
  close(fd);
  return 0;
}
