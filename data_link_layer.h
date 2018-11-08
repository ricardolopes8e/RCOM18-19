#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <limits.h>
#include <signal.h>


#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FLAG 126 /* 0x7E */
#define A 3 /* 0x03 */
#define SET_C 3 /* 0x03 */
#define UA_C 7 /* 0x07 */
#define DISC_C 11 /* 0x0B */
#define DISC 0x0B
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
#define ESC 0x7D
#define ESC_AFTER 0x5E
#define ESC_ESC 0x5D
#define RECEIVER 0
#define MAX_ALARMS 3
#define MAX_REJECTIONS 3
#define TIMEOUT 3
#define TRANSMITTER 1
#define RECEIVER 2

void send_control_message(int fd, int C);
int read_control_message(int fd, int control_character);
int read_control_message_writer(int fd);
int check_BCC2(char *message, int message_len);

int llread(int fd, char *buffer);
int llopen(int fd, int flag);
int llclose(int fd);
