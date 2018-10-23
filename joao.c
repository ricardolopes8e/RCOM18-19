/*Non-Canonical Input Processing*/
/* Transmitter */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define ERR -1
#define FLAG 126 /* 0x7E */
#define A 3 /* 0x03 */
#define SET_C 3 /* 0x03 */
#define UA_C 7 /* 0x07 */
#define SET_SIZE 5
#define UA_SIZE 5
#define MAX_ALARMS 3
#define TIMEOUT 3
#define START 0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define BCC_OK 4
#define STOP_STATE 5
#define C_0 0x00
#define C_1 0x20
#define ESC 0x7D
#define ESC_AFTER 0x5E
#define ESC_ESC 0x5D
#define RR_0 0x05
#define RR_1 0x85
#define REJ_0 0x01
#define REJ_1 0x81
#define DISC 0x0B

volatile int STOP = FALSE;
struct termios oldtio, newtio;
int flag_alarm_active, count_alarm, received, end_of_UA, contor;
char UA_received[UA_SIZE + 1];

void print_hexa(char *str) {
  int j;
  for (j = 0; j < strlen(str); j++) {
    printf("%0xh ", str[j]);  
  }
  printf("\n");
}

/* listens to the alarm */
void alarm_handler() {
  printf("alarm #%d\n3 sec expired; resend SET\n", count_alarm);
  flag_alarm_active = TRUE;
  count_alarm++;
}

void disable_alarm() {
  flag_alarm_active = 0;
  count_alarm = 0;
  alarm(0);
}

/* builds and sends to fd tge SET control message */
void send_control_message(int fd, int C) {
  unsigned char SET_message[SET_SIZE];

  SET_message[0] = FLAG;
  SET_message[1] = A;
  SET_message[2] = C;
  SET_message[3] = A ^ C;
  SET_message[4] = FLAG;

  printf("Transmit: ");
  print_hexa(SET_message);
  write(fd, SET_message, SET_SIZE);
}

void state_machine_UA(int *state, unsigned char *c) {
  switch (*state) {
    case START:
      if (*c == FLAG) {
        *state = FLAG_RCV; /* transition FLAG_RCV */
        UA_received[contor++] = *c;
      }
      break;
    case FLAG_RCV:
      /* A_RCV */
      if (*c == A) {
        *state = A_RCV;
        UA_received[contor++] = *c;
      }
      else {
        if (*c == FLAG)
          *state = FLAG_RCV;
        else {
          memset(UA_received, 0, UA_SIZE + 1);
          *state = START;
        }
      }
      break;
    case A_RCV:
      if (*c == UA_C) {
        *state = C_RCV; /* transition to C_RCV */
        UA_received[contor++] = *c;
      }
      else {
        if (*c == FLAG)
          *state = FLAG_RCV;
        else {
          memset(UA_received, 0, UA_SIZE + 1);
          *state = START;
        }
      }
      break;

    case C_RCV:
	printf("******************* %0Xh",*c);
	printf("--------------------------------%0Xh",UA_C ^A);

      if (*c == UA_C ^ A) {
        *state = BCC_OK;
        UA_received[contor++] = *c;
      }
      else {
        memset(UA_received, 0, UA_SIZE + 1);
        *state = START;
      }
      break;
    
    case BCC_OK:
      if (*c == FLAG) {
        UA_received[contor++] = *c;
        end_of_UA = TRUE;
        disable_alarm();
        printf("UA received: ");
        print_hexa(UA_received);
      }
      else {
        memset(UA_received, 0, UA_SIZE + 1);
        *state = START;
      }
      break;
    }
}
int llopen (int fd) {

    char *buf = (char*) malloc((UA_SIZE + 1) * sizeof(char));
    int i = 0, state;
    char c;

    if (tcgetattr(fd, &oldtio) == ERR) { /* save current port settings */
      perror("tcgetattr error");
      exit(ERR);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 1;   /* inter-character timer value */
    newtio.c_cc[VMIN]  = 0;   /* The read will be satisfied if a single
                              character is read, or TIME is exceeded
                              (t = TIME *0.1 s). If TIME is exceeded,
                              no character will be returned. */
    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == ERR) {
      perror("tcsetattr error");
      exit(ERR);
    }

    printf("New termios structure set\n");

    /**********so far, code from moodle**********/
    /* Sends the SET control message, waits for UA for 3 seconds.
      If it does not receive UA, it resends SET, this operation being
      repeated for 3 times */

    flag_alarm_active = 1;
    while (count_alarm < MAX_ALARMS && flag_alarm_active) {
      send_control_message(fd, SET_C); /* send SET */
      alarm(TIMEOUT);
      flag_alarm_active = 0;
      state = START;

      while (!end_of_UA && !flag_alarm_active) {
      	read(fd, &c, 1);
	buf[i++] = c;
	printf("%0X\n", c);
        state_machine_UA(&state, &c);
      }
    }

    printf("flag alarm %d\n", flag_alarm_active);
    printf("sum %d\n", count_alarm);

    if (count_alarm == MAX_ALARMS && flag_alarm_active) {
      return FALSE;
    } else {
      disable_alarm();
      return TRUE;
    }

/*
    do {
      send_control_message(fd, SET_C); /* send SET
      alarm(TIMEOUT);
      flag_alarm_active = 0;
      state = START;
      while (!end_of_UA && !flag_alarm_active) {
        read(fd, &c, 1);	
        state_machine_UA(&state, &c);
      }
    } while (flag_alarm_active && count_alarm < MAX_ALARMS);
    printf("flag alarm %d\n", flag_alarm_active);
    printf("sum %d\n", count_alarm);
    if (flag_alarm_active && count_alarm == MAX_ALARMS)
      return FALSE;
    else {
      disable_alarm();
      return TRUE;
    }
*/
}

unsigned char read_control_message(int fd) {
  int step = START;
  unsigned char c;
  unsigned char returnValue = ERR;

  while (!flag_alarm_active && step != STOP_STATE) {
    read(fd, &c, 1);
    switch (step) {
    // FLAG
    case START:
      if (c == FLAG)
        step = FLAG_RCV;
      break;
    // A
    case FLAG_RCV:
      if (c == A) {
        step = A_RCV;
      } else if (c == FLAG) {
        step = FLAG_RCV;
      } else {
        step = START;
      }
      break;
    // C
    case A_RCV:
      if (c == RR_0 || c == RR_1 || c == REJ_0 || c == REJ_1 || c == DISC) {
        returnValue = c;
        step = C_RCV;
      } else if (c == FLAG) {
          step = FLAG_RCV;
      } else {
        step = START;
      }
      break;
    // BCC2
    case C_RCV:
      if (c == (A ^ returnValue))
        step = BCC_OK;
      else
        step = START;
      break;

    case BCC_OK:
      if (c == FLAG) {
        alarm(0);
        step = STOP_STATE;
        return returnValue;
      } else
        step = START;
      break;
    }
  }
  return returnValue;
}

void llclose(int fd){
  char c;
  int state;

  state = DISC;
  send_control_message(fd,state);

  read(fd,&c,1);
  state = START;

  while(state != DISC){
    state_machine_UA(&state,&c);
  }

  state = UA_C;

  send_control_message(fd,state);

  tcsetattr(fd,TCSANOW,&oldtio);
}


int llwrite(int fd, char *buffer, int length) {

  char *message_to_send = (char*) malloc((length + 6) * sizeof(char));
  int message_to_send_size, i, seq_num, rejected = FALSE;;

  message_to_send_size = length + 6;
  /* calculate BCC2 */
  unsigned char BCC2;
  unsigned char BCC2_stuffed[2];
  int double_BCC2 = FALSE;

  BCC2 = buffer[0];
  for (i = 1; i < length; i++){
    BCC2 ^= buffer[i];
  }

  if (BCC2 == FLAG) {
    BCC2_stuffed[0] = ESC;
    BCC2_stuffed[1] = ESC_AFTER;
    double_BCC2 = TRUE;
  } else if (BCC2 == ESC){
    BCC2_stuffed[0] = ESC;
    BCC2_stuffed[1] = ESC_ESC;
    double_BCC2 = TRUE;
  }

  /* generate message cine e seq_num aici? Sigur nu trebuie alternate? */
  message_to_send[0] = FLAG;
  message_to_send[1] = A;

  if (seq_num == 0) {
    message_to_send[2] = C_0;
  } else {
    message_to_send[2] = C_1;
  }

  message_to_send[3] = (message_to_send[1] ^ message_to_send[2]);

  int n = 4; /* number of characters already written in the message_to_send */
  for (i = 0; i < length; i++) {
    if (buffer[i] == FLAG) {
      message_to_send_size += 1;
      message_to_send = (unsigned char*) realloc(message_to_send, message_to_send_size);
      message_to_send[n] = ESC;
      message_to_send[n + 1] = ESC_AFTER;
      n += 2;
    } else if (buffer[i] == ESC) {
      message_to_send_size += 1;
      message_to_send = (unsigned char*) realloc(message_to_send, message_to_send_size);
      message_to_send[n] = ESC;
      message_to_send[n + 1] = ESC_ESC;
      n += 2;
    } else {
      message_to_send[n] = buffer[i];
      n += 1;
    }
  }

  if (!double_BCC2) {
    message_to_send[n] == BCC2;
    n += 1;
  } else {
    message_to_send_size += 1;
    message_to_send = (unsigned char*) realloc(message_to_send, message_to_send_size);
    message_to_send[n] = BCC2_stuffed[0];
    message_to_send[n + 1] = BCC2_stuffed[1];
    n += 2;
  }

  message_to_send[n] = FLAG;
  n += 1;

  printf("Message to send:\n");
  print_hexa(message_to_send);

  // Send Message after generating it
  do {

    write(fd, message_to_send, n);

    flag_alarm_active = FALSE;
    alarm(TIMEOUT);

    unsigned char C = read_control_message(fd);

    if ((C == RR_1 && seq_num == 0) || (C == RR_0 && seq_num == 1)) {
      printf("Received RR %x, serie = %d\n", C, seq_num);
      rejected = FALSE;
      count_alarm = 0;
      
      if (seq_num == 0) {
        seq_num = 1;
      } else {
        seq_num = 0;
      }
      alarm(0);
    } else if (C == REJ_0 || C == REJ_1) {
      rejected = TRUE;
      printf("Received rejection %x, serie = %d\n", C, seq_num);
      alarm(0);
    }
  } while ((flag_alarm_active && count_alarm < MAX_ALARMS) || rejected);

  if (count_alarm >= MAX_ALARMS)
    return ERR;

  return (n - 1);
}

int main(int argc, char** argv) {
    int fd,c, res;
    char buf[255];
    int i, sum = 0, speed = 0;
    
    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }


  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */

  fd = open(argv[1], O_RDWR | O_NOCTTY );
  if (fd < 0) {
    perror(argv[1]);
    exit(ERR);
  }

  signal(SIGALRM, alarm_handler);  /* link SIGALRM with alarm_handler function */
  llopen(fd);

  /*
    /*testing
    buf[25] = '\n';
    
    res = write(fd,buf,255);   
    printf("%d bytes written\n", res);
 
    for (i = 0; i < 255; i++) {
      buf[i] = 'a';
    }
  /* 
    O ciclo FOR e as instruções seguintes devem ser alterados de modo a respeitar 
    o indicado no guião 
  
*/ 
  if (tcsetattr(fd, TCSANOW, &oldtio) == ERR) {
    perror("tcsetattr");
    exit(ERR);
  }

  close(fd);
  return 0;
}