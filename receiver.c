/*Non-Canonical Input Processing*/

#include "data_link_layer.h"
#include "application_layer.h"

volatile int STOP = FALSE;
struct termios oldtio, newtio;
int seq_num_to_send = 0;
int last_seq = 0;

void send_control_message(int fd, int C)
{
    unsigned char message[UA_SIZE];

    message[0] = FLAG;
    message[1] = A;
    message[2] = C;
    message[3] = A ^ C;
    message[4] = FLAG;

    write(fd, message, UA_SIZE);
}

/* state machine */
int read_control_message(int fd, int control_character)
{
    int state = START;
    char* buffer = (char*)malloc((SET_SIZE + 1) * sizeof(char));
    char c;
    int i = 0;

    while (state != STOP_STATE) {
        read(fd, &c, 1);

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
            } else if (c == FLAG)
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
            } else if (c == FLAG)
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
            } else {
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
    return TRUE;
}

int check_BCC2(char* message, int message_len)
{
    int i;
    char BCC2 = message[4]; /* start from the information, after BCC1 */

    for (i = 5; i < message_len - 2; i++) {
        BCC2 ^= message[i];
    }

    return BCC2 == message[message_len - 2];
}

int llread(int fd, char* buffer)
{
    int state = START;
    char c, control_character;
    int rej = 0, count_read_char = 0;

    while (state != STOP_STATE) {
        read(fd, &c, 1);
        count_read_char += 1;
    
        switch (state) {
        case START:
            /* FLAG_RCV */
            if (c == FLAG) {
                buffer[count_read_char - 1] = c;
                state = FLAG_RCV;
            }
            break;

        case FLAG_RCV:
            /* A_RCV */
            if (c == A) {
                state = A_RCV;
                buffer[count_read_char - 1] = c;
            } else if (c == FLAG)
                state = FLAG_RCV;
            else {
                memset(buffer, 0, MAX_DATA_SIZE);
                count_read_char = 0;
                state = START;
            }
            break;

        case A_RCV:
            /* C_RCV */
            if (c == C_0) {

                seq_num_to_send = 1;
                state = C_RCV;
                buffer[count_read_char - 1] = c;
                control_character = c;
            } else if (c == C_1) {

                seq_num_to_send = 0;
                state = C_RCV;
                buffer[count_read_char - 1] = c;
                control_character = c;
            } else if (c == FLAG) {
                control_character = c;
                state = FLAG_RCV;
            } else {
                memset(buffer, 0, MAX_DATA_SIZE);
                count_read_char = 0;
                state = START;
            }
            break;

        case C_RCV:
            /* BCC_OK */
            if (c == (A ^ control_character)) {
                state = BCC_OK;
                buffer[count_read_char - 1] = c;
            } else {
                memset(buffer, 0, MAX_DATA_SIZE);
                count_read_char = 0;
                state = START;
            }
            break;

        case ESCAPE_STATE:
            if (c == 0x5E) { /* 0x7D 0x5E => 0x7E (FLAG) */
                count_read_char--;
                buffer[count_read_char - 1] = FLAG;
                state = BCC_OK;
            } else if (c == 0x5D) { /* 0x7D 0x5D => 0x7D (escape_character) */
                count_read_char--;
                buffer[count_read_char - 1] = escape_character;
                state = BCC_OK;
            } else {
                perror("Invalid character after escape.");
                return ERR;
            }
            break;

        case BCC_OK:
            if (c == FLAG) {
                buffer[count_read_char - 1] = c;
                if (check_BCC2(buffer, count_read_char)) {
                    if (seq_num_to_send == 1) {
                        send_control_message(fd, RR_1);
                        printf("Sent RR_1\n");
                    } else {
                        send_control_message(fd, RR_0);
                        printf("Sent RR_0\n");
                    }

                    state = STOP_STATE;

                } else {
                    rej = TRUE;
                    if (seq_num_to_send == 0) {
                        send_control_message(fd, REJ_0);
                        printf("Sent REJ_0\n");
                    }

                    else {
                        send_control_message(fd, REJ_1);
                        printf("Sent REJ_1\n");
                    }
                    state = STOP_STATE;
                }
            } else if (c == escape_character) {
                state = ESCAPE_STATE;
            } else {
                buffer[count_read_char - 1] = c;
            }
            break;
        }
    }

    if (rej)
        return 0;
    return count_read_char;
}

int llopen(int fd, int flag)
{
    int command_char, reply_char;
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

    newtio.c_cc[VTIME] = 1; /* inter-character timer value */
    newtio.c_cc[VMIN] = 0; /* The read will be satisfied if a single
                              character is read, or TIME is exceeded
                              (t = TIME *0.1 s). If TIME is exceeded,
                              no character will be returned. */
    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == ERR) {
        perror("tcsetattr error");
        exit(ERR);
    }
    printf("New termios structure set\n");

    if (flag == RECEIVER) {
        command_char = SET_C;
        reply_char = UA_C;
    } else {
        printf("[llopen]Receiver started with wrong flag\n");
        return ERR;
    }
    if (read_control_message(fd, command_char)) {
        printf("Received SET\n");
        send_control_message(fd, reply_char);
        printf("Sent UA\n");
    }

    printf("LLOPEN done!");
    return fd;
}

int llclose(int fd)
{
    int state;

    state = DISC_C;
    if (read_control_message(fd, state)) {
        printf("Received DISC\n");
        send_control_message(fd, state);
    } else {
        return ERR;
    }

    state = UA_C;
    if (read_control_message(fd, state)) {
        printf("Receiver terminated\n");
    } else {
        return ERR;
    }

    tcsetattr(fd, TCSANOW, &oldtio);
    return TRUE;
}

int is_trailer_message(char* first, int size_first, char* last, int size_last)
{
    if (size_first != size_last || last[0] != '3') {
        return FALSE;
    } else {
        int i;
        for (i = 1; i < size_first - 2; i++) {
            if (first[i] != last[i]) {
                return FALSE;
            }
        }
        return TRUE;
    }
    return FALSE;
}

int compare_messages(char* previous, int previous_size, char* new, int size_new)
{
    if (previous_size != size_new) {
        return 1;
    } else {
        int i;
        for (i = 0; i < size_new; i++) {
            if (previous[i] != new[i]) {
                return 1;
            }
        }
    }

    return 0;
}

/*Remove Header, Frame and Trailer*/
char* remove_header(char* message, char message_size, int* new_size, int* info_len)
{
    int L1, L2, i;
    char* new_message = (char*)malloc(message_size);

    L2 = message[6];
    L1 = message[7];
    *info_len = L1 + 256 * L2;
    
    for (i = 0; i < *info_len; i++) {
        new_message[i] = message[i + 8];
    }

    *new_size = *info_len;

    return new_message;
}

void name_file(char* message, char* name)
{
    memset(name, 0, 100);

    int L1 = message[2] - '0';
    int L2_index = L1 + 4;
    int L2 = message[L2_index];
    int i;

    for (i = 0; i < L2; i++) {
        name[i] = message[5 + L1 + i];
    }

    name[L2] = '\0';
}

off_t size_of_file(char* message)
{

    int i, L1 = message[2] - '0';
    printf("L1: %d\n", L1);
    off_t dec = 0;
    char* new = (char*)malloc(100 * sizeof(char));

    strncat(new, message + 3, L1);
    new[L1] = '\0';
    
    for (i = 0; i < strlen(new); i++)
        dec = dec * 10 + (new[i] - '0');

    if (dec >= (ULONG_MAX - 2)) {
        return 0;
    }
    return dec;
}

void receive_file(int fd)
{

    int message_size, size_first_message, size_without_header, info_len = 0;
    int previous_size, n = 4, i, compared;
    FILE* file_out;
    char* first_message = (char*)malloc((MAX_DATA_SIZE) * sizeof(char));
    char* new_message = (char*)malloc((MAX_DATA_SIZE) * sizeof(char));
    char* message = (char*)malloc((MAX_DATA_SIZE) * sizeof(char));
    char* previous_message = (char*)malloc((MAX_DATA_SIZE) * sizeof(char));
    char* message_to_compare = (char*)malloc((MAX_DATA_SIZE) * sizeof(char));
    char* file_name = (char*)malloc(100 * sizeof(char));

    size_first_message = llread(fd, first_message);

    for (i = 0; i < size_first_message; i++) {
        new_message[i] = first_message[n];
        n++;
    }
    size_first_message -= 4;

    name_file(new_message, file_name);
    off_t file_size = size_of_file(new_message);

    if (file_size == 0) {
        printf("File not found on the writer side.\n");
        exit(ERR);
    }

    file_out = fopen(file_name, "wb+");
    int packet_number = 1;
    while (TRUE) {
        memset(message, 0, FRAGMENT_SIZE);
        message_size = llread(fd, message);

        if (packet_number > 1) {
            compared = compare_messages(previous_message, previous_size, message, message_size);
        }

        if (message_size == 0 || message_size == ERR || compared == 0) {
            if (compared == 0) {
                printf("Same messages!!!\n");
            }
            printf("REJECTED PACKAGE\n");
            continue;
        }

        memset(previous_message, 0, FRAGMENT_SIZE);
        memcpy(previous_message, message, message_size);
        previous_size = message_size;

        int n = 4;
        int i = 0;
        for (i = 0; i < message_size; i++) {
            message_to_compare[i] = message[n];
            n++;
        }
        int message_to_compare_size = message_size - 4;

        if (is_trailer_message(new_message, size_first_message, message_to_compare, message_to_compare_size)) {
            printf("Trailer Message!\n");
            break;
        }

        message = remove_header(message, message_size, &size_without_header, &info_len);
        
        fwrite(message, 1, info_len, file_out);
        packet_number++;
    }

    fclose(file_out);
}

int main(int argc, char** argv)
{
    int fd;

    if ((argc < 2) || ((strcmp("/dev/ttyS0", argv[1]) != 0) && (strcmp("/dev/ttyS1", argv[1]) != 0))) {
        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
        exit(1);
    }

    /*
  Open serial port device for reading and writing and not as controlling tty
  because we don't want to get killed if linenoise sends CTRL-C.
*/

    fd = open(argv[1], O_RDWR | O_NOCTTY);
    if (fd < 0) {
        perror(argv[1]);
        exit(ERR);
    }

    llopen(fd, RECEIVER);

    receive_file(fd);

    llclose(fd);

    close(fd);
    return 0;
}