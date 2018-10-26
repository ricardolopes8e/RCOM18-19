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
#define MAX_DATA_SIZE 300
#define FRAGMENT_SIZE 100
