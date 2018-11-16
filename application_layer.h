#include <errno.h>
#include <math.h>

#define C_0 0x00
#define C_1 0x40
#define MAX_DATA_SIZE 300
#define FRAGMENT_SIZE 500
#define START_PACKET 2
#define END_PACKET 3
#define DATA_PACKET 1
#define FILE_SIZE_CODE 0
#define FILE_NAME_CODE 1
#define BUF_SIZE 318
#define FILE_NAME_SIZE 200
#define CONTROL_MESSAGE_LEN 20
#define FRAME_SIZE 6
#define PH_SIZE 4


int is_trailer_message(unsigned char* first,int size_first, unsigned char* last, int size_last); //receive_file
int compare_messages(unsigned char*previous, int previous_size, unsigned char* new, int size_new); //receive_file
unsigned char * remove_header(unsigned char *message, unsigned char message_size, int * new_size, int *info_len); //receive_file
void name_file(unsigned char* message, unsigned char *name); //receive_file
off_t size_of_file(unsigned char* message); //receive_file
void receive_file(int fd);
