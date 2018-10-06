/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h> 
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h> 

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define FLAG          0x7E
#define A             0x03
#define SET           0x03
#define UA_CTRL       0x07

volatile int STOP=FALSE;

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];

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

    if (fd <0) {
      perror(argv[1]); 
      exit(-1); 
    }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 5;   /* blocking read until 5 chars received */



  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) próximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");

    int count = 0;

    while (STOP == FALSE) {       /* loop for input */
      
      res = read(fd, &buf[i], 1);                                      
      printf("%c\n", buf[i]);

      if (buf[i] == '\0') {
		    printf("Received end of transmission\n");
		    STOP = TRUE;
	    }
    }

 	  printf("Transmit the string back\n");
    printf("%s\n", buf);
	 
	  res = write(fd, buf, i);
	
    int state = 0;

    while (1) {

      read(fd,&buf[i],1);

      switch(state){

        case 0:
          if (buf[i] == FLAG)
            state = 1;
          else
            state = 0;
          break;
        case 1:
          if(buf[i] == A)
            state = 2;
          else if(buf[i] == FLAG)
            state = 1;
          else
            state = 0;
          break;
        case 2:
          if(buf[i] == SET)
            state = 3;
          else if (buf[i] == FLAG)
            state = 1;
          else
            state = 0;
          break;
        case 3:
          if(buf[i] == A^SET)
            state = 4;
          else
            state = 1;
          case 4:
            if(buf[i] == FLAG){
              exit();
            }
            else
              state = 0;
            break;
      }
      i++;
    }

    unsigned char UA[5];

    UA[0] = FLAG;
    UA[1] = A;
    UA[2] = UA_CTRL;
    UA[3] = A^UA_CTRL;
    UA[4] = FLAG;

    while(){
      printf("Sending UA...\n");
      res = write(fd,UA,5);
      printf("%d bytes written\n",res);

      if(res == 5){
        printf("UA done!\n");
        exit(0);
      }
    }

    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;

  }



