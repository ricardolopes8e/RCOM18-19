/*Non-Canonical Input Processing*/
/* Transmitter */

//#include "writer.h"
#include "data_link_layer.h"
#include "application_layer.h"

#define bcc1ErrorPercentage 0
#define bcc2ErrorPercentage 0

volatile int STOP = FALSE;
struct termios oldtio, newtio;
int flag_alarm_active, count_alarm, received, end_of_UA, contor;
int seq_num = 0, packet_number = 0;
unsigned char UA_received[UA_SIZE + 1];

 unsigned char *change_BCC2( unsigned char *packet, int sizePacket)
{
   unsigned char *copia = ( unsigned char *)malloc(sizePacket);
  memcpy(copia, packet, sizePacket);
  int r = (rand() % 100) + 1;
  if (r <= bcc2ErrorPercentage)
  {
    int i = (rand() % (sizePacket - 5)) + 4;
     unsigned char randomLetter = ( unsigned char)('A' + (rand() % 26));
    copia[i] = randomLetter;
    printf("Modifiquei BCC2\n");
  }
  return copia;
}

 unsigned char *change_BCC1( unsigned char *packet, int sizePacket)
{
   unsigned char *copia = ( unsigned char *)malloc(sizePacket);
  memcpy(copia, packet, sizePacket);
  int r = (rand() % 100) + 1;
  if (r <= bcc1ErrorPercentage)
  {
    int i = (rand() % 3) + 1;
     unsigned char randomLetter = ( unsigned char)('A' + (rand() % 26));
    copia[i] = randomLetter;
    printf("Modifiquei BCC1\n");
  }
  return copia;
}


void print_hexa(unsigned char *str) {
  int j;
  for (j = 0; j < strlen(str); j++) {
    printf("%0xh ", str[j]);
  }
  printf("\n");
}

void print_hexa_zero(unsigned char *str, int len) {
  int j;
  for (j = 0; j < len; j++) {
    printf("%0xh ", str[j]);
  }
  printf("\n");
}

/* listens to the alarm */
void alarm_handler() {
  printf("alarm number %d\n3 sec expired\n", count_alarm);
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
        printf("State Machine UA OK!\n");

      }
      else {
        memset(UA_received, 0, UA_SIZE + 1);
        *state = START;
      }
      break;
    }


}
int llopen (int fd, int flag) {

  unsigned char *buf = (unsigned char*) malloc((UA_SIZE + 1) * sizeof(unsigned char));
  int state;
  unsigned char c;

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

  newtio.c_cc[VTIME] = 1;   /* inter-unsigned character timer value */
  newtio.c_cc[VMIN]  = 0;   /* The read will be satisfied if a single
                            unsigned character is read, or TIME is exceeded
                            (t = TIME *0.1 s). If TIME is exceeded,
                            no unsigned character will be returned. */
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

  flag_alarm_active = TRUE;
  while (count_alarm < MAX_ALARMS && flag_alarm_active) {
    send_control_message(fd, SET_C); /* send SET */
    alarm(TIMEOUT);
    flag_alarm_active = FALSE;
    state = START;

    while (!end_of_UA && !flag_alarm_active) {
    	read(fd, &c, 1);

	    state_machine_UA(&state, &c);
    }
  }



  if (count_alarm == MAX_ALARMS && flag_alarm_active) {
    printf("LLOPEN returning false.\n");
    return FALSE;
  } else {
    disable_alarm();
    printf("LLOPEN OK!\n");
    return TRUE;
  }



}
/* reads control message and returns the control unsigned character in the
control message; also, uses the state machine to check if the message
is correctly received */
int read_control_message_writer(int fd) {
  int step = START;
  unsigned char c;
  int returnValue = ERR;

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

/* closes the connection between sender and receiver */
int llclose(int fd){
  unsigned char c;
  int control_char, state;

  control_char = DISC;
  send_control_message(fd,control_char);

  control_char = 0;

  read(fd,&c,1);
  state = START;
  while(state != STOP){


    state_machine_UA(&state, &c);
    printf("State is: %d\n", state);
  }

  printf("[llclose]State after while is: %d\n", state);

  control_char = UA_C;

  send_control_message(fd,control_char);

  tcsetattr(fd,TCSANOW,&oldtio);
  printf("Sender terminated!\n.");
  return TRUE;
}

/* gets the file size in bytes if a given file */
off_t fsize(const unsigned char *filename) {
    struct stat st;

    if (stat(filename, &st) == 0)
        return st.st_size;

    fprintf(stderr, "Cannot determine size of %s: %s\n",
            filename, strerror(errno));

    return ERR;
}


int create_control_packet(unsigned char *control_packet, int packet_type,
                          int file_size, unsigned char* file_name) {
  unsigned char buffer[BUF_SIZE];
  int bytes_written = 5;
  double L1;
  long L2;
  int pos = 0;

  /* C - packet_type, start or end */
  memset(buffer, 0, BUF_SIZE);
  sprintf(buffer, "%d", packet_type);
  strcat(control_packet, buffer);
  pos++;



  /* file size parameter code */
  memset(buffer, 0, BUF_SIZE);
  sprintf(buffer, "%d", FILE_SIZE_CODE);
  strcat(control_packet, buffer);
  pos++;


  /* file_size parameter lenght in bytes */
  memset(buffer, 0, BUF_SIZE);
  L1 = floor(log10(abs(file_size))) + 1; /* size in octets of file_size */
  sprintf(buffer, "%0.0f", L1);
  strcat(control_packet, buffer);
  pos++;


  /* file_size parameter value */
  memset(buffer, 0, BUF_SIZE);
  sprintf(buffer, "%d", file_size);
  strcat(control_packet, buffer);
  bytes_written += L1;
  pos += L1;


  /* file_name parameter code */
  memset(buffer, 0, BUF_SIZE);
  sprintf(buffer, "%d", FILE_NAME_CODE);
  strcat(control_packet, buffer);
  pos++;



  /* file_name parameter length in bytes */
  memset(buffer, 0, BUF_SIZE);
  L2 = strlen(file_name);
  control_packet[pos] = L2;
  pos++;

  /* file_name parameter */
  strcat(control_packet, file_name);
  bytes_written += L2;

  return bytes_written;
}

/* creates an I frame with the data from a data packet
  (packet header + file fragment)
  returns the number of bytes in the frame */
int encapsulate_data_in_frame(unsigned char *message_to_send, unsigned char* buffer, int length) {

  int message_to_send_size, i;

  /* calculate BCC2 */
  unsigned char BCC2;
  unsigned char BCC2_stuffed[2];
  int double_BCC2 = FALSE;

  BCC2 = buffer[0];
  for (i = 1; i < length; i++)
    BCC2 ^= buffer[i];

  //printf("BCC2 = %0xh\n", BCC2);

  if (BCC2 == FLAG) {
    BCC2_stuffed[0] = ESC;
    BCC2_stuffed[1] = ESC_AFTER;
    double_BCC2 = TRUE;
  } else if (BCC2 == ESC){
    BCC2_stuffed[0] = ESC;
    BCC2_stuffed[1] = ESC_ESC;
    double_BCC2 = TRUE;
  }

  /* generate message */
  message_to_send[0] = FLAG;
  message_to_send[1] = A;

  if (seq_num == 0) {
    message_to_send[2] = C_0;
    //printf("Entered if\n");
  } else {
    message_to_send[2] = C_1;
  }

  message_to_send[3] = (message_to_send[1] ^ message_to_send[2]);

  int n = 4; /* number of unsigned characters already written in the message_to_send */
  message_to_send_size = length + FRAME_SIZE;
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
    message_to_send[n] = BCC2;
    n += 1;
  } else {
    message_to_send_size += 1;
    message_to_send = (unsigned char*) realloc(message_to_send, message_to_send_size);
    message_to_send[n] = BCC2_stuffed[0];
    message_to_send[n + 1] = BCC2_stuffed[1];
    n += 2;
  }
  message_to_send[n] = FLAG;

  return n + 1;
}

/* sends the information contained in buffer */
int llwrite(int fd, unsigned char *message_to_send, int info_frame_size) {

  int rejected = FALSE, rejected_count = 0, try_count = 0;
  do {
    try_count++;

    printf("Try number: %d\n", try_count);
	
	unsigned char* copia;

    copia = change_BCC1(message_to_send, info_frame_size); 
    copia = change_BCC2(message_to_send, info_frame_size); 

		write(fd, copia, info_frame_size);

		flag_alarm_active = FALSE;
		alarm(TIMEOUT);

		int C = read_control_message_writer(fd);

    if ((C == RR_1 && seq_num == 0) || (C == RR_0 && seq_num == 1)) {
      if (C == RR_0) printf("Received RR_0\n");
      if (C == RR_1) printf("Received RR_1\n");

      rejected = FALSE;
      count_alarm = 0;

      if (seq_num == 0) {
        seq_num = 1;
      } else {
        seq_num = 0;
      }
      alarm(0);

    } else if (C == REJ_0 || C == REJ_1) {
      printf("Sequence Number = %d \n", seq_num);
      rejected_count++;

      rejected = TRUE;


      if (C == REJ_0) printf("RECEIVED REJ_0\n");
      if (C == REJ_1) printf("RECEIVED REJ_1\n");
      if (C == ERR) printf("RECEIVED ERR\n");
      printf("Rejected Count: %d\n", rejected_count);
      alarm(0);

    }

  } while ((flag_alarm_active && (count_alarm < MAX_ALARMS)) ||
           (rejected && (rejected_count < MAX_REJECTIONS)));

  if (count_alarm >= MAX_ALARMS)
    return ERR;

  printf("Number of bytes written: %d \n", info_frame_size);
  return info_frame_size;
}

void create_data_packet(unsigned char *data_packet, unsigned char *buffer, int length, int seq) {
  data_packet[0] = DATA_PACKET;
  data_packet[1] = seq % 255;
  data_packet[2] = length / 256; // L2
  data_packet[3] = length - 256 * data_packet[2]; // L1
  memcpy(data_packet + 4, buffer, length);
}

int send_file(int fd, unsigned char* file_name) {

  FILE *fp;
  unsigned char *control_packet = (unsigned char*) malloc(CONTROL_MESSAGE_LEN * sizeof(unsigned char));
  unsigned char *data_packet = (unsigned char*) malloc((FRAGMENT_SIZE + 4) * sizeof(unsigned char));
  int control_packet_len, data_packet_len, bytes_read, end_of_file = FALSE, seq = 0;
  unsigned char buffer[FRAGMENT_SIZE];
  int bytes_after_framing;

  int info_frame_size = FRAGMENT_SIZE + PH_SIZE + FRAME_SIZE;
  unsigned char *message_to_send = (unsigned char*) malloc(info_frame_size * sizeof(unsigned char));

  /* create control packet: START packet */
  printf("File to send: size of %s is %li bytes\n", file_name, fsize(file_name));
  control_packet_len = create_control_packet(control_packet, START_PACKET,
                                             fsize(file_name), file_name);

  printf("Send start control packet: %s of size = %d\n", control_packet, control_packet_len);

  /* put control_packet into I frames */
  memset(message_to_send, 0, info_frame_size);
  bytes_after_framing = encapsulate_data_in_frame(message_to_send, control_packet, control_packet_len);



  llwrite(fd, message_to_send, bytes_after_framing);
  packet_number++;
  seq++;

  /* open file to transmit in read mode */
  fp = fopen(file_name, "rb");
  if (fp == NULL) {
	printf("File does not exist\n");
    exit(-1);
  }

  while (!end_of_file) {

    memset(buffer, 0, FRAGMENT_SIZE);
    bytes_read = fread(buffer, 1, FRAGMENT_SIZE, fp);

    if (bytes_read < FRAGMENT_SIZE) {
      end_of_file = TRUE;
    }

	/* create data_packet */
    memset(data_packet, 0, FRAGMENT_SIZE + PH_SIZE);
    create_data_packet(data_packet, buffer, bytes_read, seq);
	data_packet_len = bytes_read + PH_SIZE;

	/* frame the data_packet into a I frame */
	memset(message_to_send, 0, info_frame_size);
    bytes_after_framing = encapsulate_data_in_frame(message_to_send, data_packet, data_packet_len);

    int r = llwrite(fd, message_to_send, bytes_after_framing);
    packet_number++;
	  seq++;

    if (r == ERR) {
      printf("Max number of alarms reached\n");
      return ERR;
    }
  }

  /* create END control_packet */
  memset(control_packet, 0, control_packet_len);
  create_control_packet(control_packet, END_PACKET, fsize(file_name), file_name);

  /* encapsulate into I frame */
  memset(message_to_send, 0, info_frame_size);
  bytes_after_framing = encapsulate_data_in_frame(message_to_send, control_packet, control_packet_len);
  printf("Send STOP control packet: %s of size = %d\n", control_packet, control_packet_len);
  llwrite(fd, message_to_send, bytes_after_framing);
  packet_number++;

  fclose(fp);
  return 0;
}


int main(int argc, unsigned char** argv) {
    int fd;
    //int i, sum = 0, speed = 0;

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
  llopen(fd, TRANSMITTER);


  send_file(fd,"pinguim.gif");
  llclose(fd);

  close(fd);
  return 0;
}
