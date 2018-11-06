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
#include <errno.h>
#include <math.h>

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
#define MAX_REJECTIONS 3
#define TIMEOUT 3
#define START 0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define BCC_OK 4
#define STOP_STATE 5
#define C_0 0x00
#define C_1 0x40
#define ESC 0x7D
#define ESC_AFTER 0x5E
#define ESC_ESC 0x5D
#define RR_0 0x05
#define RR_1 0x85
#define REJ_0 0x01
#define REJ_1 0x81
#define DISC 0x0B
#define START_PACKET 2
#define END_PACKET 3
#define DATA_PACKET 1
#define FILE_SIZE_CODE 0
#define FILE_NAME_CODE 1
#define BUF_SIZE 318
#define FILE_NAME_SIZE 200
#define CONTROL_MESSAGE_LEN 20
#define FRAME_SIZE 6
#define FRAGMENT_SIZE 100
#define PH_SIZE 4
#define TRANSMITTER 1

/*************** Data Link layer ***************/
void alarm_handler();
void disable_alarm();
void send_control_message(int fd, int C);

/* reads control message and sets end_of_UA variable to true
if the message was correctly formated, using the state machine */
void state_machine_UA(int *state, unsigned char *c);

int llopen (int fd, int flag);
int llwrite(int fd, char *message_to_send, int info_frame_size);
int llclose(int fd);

/* reads control message and returns the control character in the
control message; also, uses the state machine to check if the message
is correctly received */
int read_control_message(int fd);

/*************** Application layer ***************/
int send_file(int fd, char* file_name);
off_t fsize(const char *filename);
int create_control_packet(char *control_packet, int packet_type, int file_size, char* file_name);
int encapsulate_data_in_frame(char *message_to_send, char* buffer, int length);
void create_data_packet(char *data_packet, char *buffer, int length, int seq);