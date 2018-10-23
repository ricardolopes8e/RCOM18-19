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
#define ESCAPE_STATE 6
#define UA_SIZE 5
#define SET_SIZE 5
#define RR_0 0x05
#define RR_1 0X85
#define REJ_0 0x01
#define REJ_1 0X81
#define escape_character 0x7D
#define C_0 0x00
#define C_1 0x40

volatile int STOP = FALSE;
struct termios oldtio, newtio;

void print_hexa(char *str) {
  int j;
  for (j = 0; j < strlen(str); j++) {
    printf("%0xh ", str[j]);  
  }
  printf("\n");
}

void print_hexa_zero(char *str, int len) {
  int j;
  for (j = 0; j < len; j++) {
    printf("%0xh ", str[j]);  
  }
  printf("\n");
}

void send_control_message(int fd, int C) {
  unsigned char message[UA_SIZE];

  message[0] = FLAG;
  message[1] = A;
  message[2] = C;
  message[3] = A ^ C;
  message[4] = FLAG;

  printf("Transmit: ");
  print_hexa(message);
  write(fd, message, UA_SIZE);
}

/* state machine */
int read_control_message(int fd, int control_character) {
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
        if (c == control_character) {
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
        if (c == (A ^ control_character)) {
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

int check_BCC2(char *message, int message_len) {
  int i;
  unsigned char BCC2 = message[4]; /* start from the information, after BCC1 */

  printf("will xor: ");
  print_hexa_zero(message, message_len);
  printf("from 0 to %d\n", message_len - 2);

  for (i = 1; i < message_len - 1; i++) {
    BCC2 ^= message[i];
  }
  printf("check if %0xh == %0xh\n", BCC2, message[message_len - 1]);
  return BCC2 == message[message_len - 1];
}

int llread(int fd, int *buff_len) {
  int state = START;
  char* message = (char*) malloc((*buff_len + 6) * sizeof(char));
  char c, control_character;
  int seq_num, i = 0, count_read_char = 0;

  while (state != STOP_STATE) {
    read(fd, &c, 1);
	count_read_char += 1;
	printf("char_read = %0xh\n", c);

    switch(state) {
      case START:
        /* FLAG_RCV */
        if (c == FLAG) {
          message[i++] = c;
          state = FLAG_RCV;
        }
        break;

      case FLAG_RCV:
        /* A_RCV */
        if (c == A) {
          state = A_RCV;
          message[i++] = c;
        }
        else if (c == FLAG)
          state = FLAG_RCV;
        else {
	  	  memset(message, *buff_len, 0);
		  i = 0;
          state = START;
        }
        break;

      case A_RCV:
        /* C_RCV */
        if (c == C_0) {
          seq_num = 0;
          state = C_RCV;
          message[i++] = c;
          control_character = c;
        } else if (c == C_1) {
          seq_num = 1;
          state = C_RCV;
          message[i++] = c;
          control_character = c;
        }
        else if (c == FLAG) {
          control_character = c;
          state = FLAG_RCV;
        } else {
		  memset(message, *buff_len, 0);
		  i = 0;
          state = START;
        }
        break;

      case C_RCV:
        /* BCC_OK */
        if (c == (A ^ control_character)) {
          state = BCC_OK;
          message[i++] = c;
        }
        else {
		  memset(message, *buff_len, 0);
		  i = 0;
          state = START;
        }
        break;

      case ESCAPE_STATE:
        if (c == 0x5E) { // 0x7D 0x5E => 0x7E (FLAG)
          ++(*buff_len);
          message = (char*) realloc(message, *buff_len);
          message[*buff_len - 1] = FLAG;
          state = BCC_OK;
        } else if (c == 0x5D) { // 0x7D 0x5D => 0x7D (escape_character)
          ++(*buff_len);
          message = (char*) realloc(message, *buff_len);
          message[*buff_len - 1] = escape_character;
          state = BCC_OK;
        } else {
          perror("Invalid character after escape.");
          return ERR;
        }
        break;

      case BCC_OK:
        if (c == FLAG) {
	  	  message[*buff_len] = c;
          if (check_BCC2(message, *buff_len)) {
            if (seq_num == 0)
              send_control_message(fd, RR_1);
            else
              send_control_message(fd, RR_0);
	    
            state = STOP_STATE;
            printf("Sent RR for seq_num = %d\n", seq_num);
          } else {
            if (seq_num == 0)
              send_control_message(fd, REJ_1);
            else
              send_control_message(fd, REJ_0);
            state = STOP_STATE;
            printf("Sent REJ, seq_num = %d\n", seq_num);
          }
        } else if (c == escape_character) {
          state = ESCAPE_STATE;
        } else {
          ++(*buff_len);
          message = (char*) realloc(message, *buff_len);
          message[*buff_len - 1] = c;
        }
        break;
    }
  }
  printf("count_read_char: %d\n",  count_read_char);
  return count_read_char;
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
    
    if (read_control_message(fd, SET_C)) {
      printf("Received SET\n");
      send_control_message(fd, UA_C);
      printf("Sent UA\n");
    }
}

void llclose(int fd) {
  int state;

  state = DISC_C;
  if (read_control_message(fd, state)) {
	printf("Received DISC\n");
  	send_control_message(fd, state);
  }
	

  state = UA_C;
  if (read_control_message(fd, state))
	printf("Receiver terminated\n");

  tcsetattr(fd, TCSANOW, &oldtio);
}

int is_trailer_message(first, size_first, last, size_last){
  if (size_first != size_last){
    return FALSE;
  } else if (memcmp(first, last, size_first) == 0){
    return true;
  }
  return FALSE;
}

char * remove_header(char *message, char message_size, int * new_size){
  int n = 4;
  char * new_message = (char *)malloc(message_size - 4);
  for (int i = 0; i < message_size; i++){
    new_message[i] = message[n];
    n++;
  }
  *new_size = message_size - 4;
  return new_message;
}

char* name_file(char* message){

  /* not sure about this */

  char L1 = message[2];
  int L2_index = (int) L1 + 4;
  char L2 = message[L2_index];

  char * name = (char *)malloc(L2 + 1);

  for (int i = 0; i < L2; i++){
    name[i] = message[5 + L1 + i];
  }

  name[L2] = '\0';
  return name;

}

off_t file_size (char* message){

  char L1 = message[2];

  /*I'm kinda lost but I'll find a way to do this...tomorrow :) */
}


void receive_file(char* file_name){

  char message_size;
  char size_first_message;
  char size_without_header;
  off_t file_index = 0;



  char* first_message = llread(fd, $size_first_message);

  char* file_name = name_file(first_message);

  off_t file_size = file_size(first_message);


  char* allocated_space = (char *)malloc(file_size);

  while(TRUE){
    message = llread(fd, &message_size);

    if (message_size == 0) continue;

    if (is_trailer_message(first_message, size_first_message, message, message_size) == TRUE) break;

    message = remove_header(message, message_size, &size_without_header);

    memcpy(allocated_space + file_index, message, size_without_header);
    file_index += size_without_header;
  }



  FILE *file = fopen((char *) file_name, "w");
  fwrite((void *) allocated_space, 1, file_size, file);
  fclose(file);

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
  int len = 7;
  llread(fd, &len);  

  llclose(fd);
  
  close(fd);
  return 0;
}
