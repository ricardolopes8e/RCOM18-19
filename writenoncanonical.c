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
#define MODEMDEVICE "/dev/ttyS1"
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
    char buf[255],end[255],line[255];
    int i, sum = 0, speed = 0;

    unsigned char SET[5];

    SET[0]=FLAG;
    SET[1]=A;
    SET[2]=0x03;

    SET[3]=SET[1]^SET[2];/*calculate bcc*/

    SET[4]=FLAG;
    
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
    if (fd <0) {perror(argv[1]); exit(-1); }

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

    
    while () {
      printf("Sending SET\n");
      res = write(fd,SET,5);
      printf("%d bytes written\n", res);
      if(res==5){
        printf("SET sent\n");
        exit(0);
      }
    }
    
    sleep(3);

        


    unsigned int state=0;

    while (STOP==FALSE) {       /* loop for input */
      res = read(fd,&buf[i],1);
      //if (buf2[i]=='\0') STOP=TRUE;
      switch (state) {
        case 0:
          if(buf[i]==FLAG)
            state=1;//flag received
          else
            state=0;
          break;
        case 1:
          if(buf[i]==A)
            state=2;//address received
          else if(buf[i]==FLAG)
            state=1;
          else
            state=0;
          break;
        case 2:
          if(buf[i]==0x07)//control UA received
            state=3;
          else if(buf[i]==FLAG)
            state=1;
          else
            state=0;
        case 3:
          if(buf[i]==(A^buf[2]))//bcc received
            state=4;
          else if(buf[i]==FLAG)
            state=1;
          else
            state=0;
        case 4:
          if(buf[i]==FLAG){//bcc received
            exit(1);
          }
          else
            state=0;

      }
      i++;
    }

    //printf("%c\n", buf[0]);
    printf("UA received\n");



      sleep(2);

   
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }




    close(fd);
    return 0;
}
