#define C_0 0x00
#define C_1 0x40
#define MAX_DATA_SIZE 300
#define FRAGMENT_SIZE 100

int is_trailer_message(char* first,int size_first, char* last, int size_last); //receive_file
int compare_messages(char*previous, int previous_size, char* new, int size_new); //receive_file
char * remove_header(char *message, char message_size, int * new_size, int *info_len); //receive_file
void name_file(char* message, char *name); //receive_file
off_t size_of_file(char* message); //receive_file
void receive_file(int fd);
