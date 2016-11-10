#include <stdint.h>


#define NV4_CHANNEL1 1
#define NV4_CHANNEL2 2
#define NV4_CHANNEL3 3
#define NV4_CHANNEL4 4
#define NV4_CHANNEL5 5
#define NV4_CHANNEL6 6
#define NV4_CHANNEL7 7
#define NV4_CHANNEL8 8
#define NV4_CHANNEL9 9
#define NV4_CHANNEL10 10
#define NV4_CHANNEL11 11
#define NV4_CHANNEL12 12
#define NV4_CHANNEL13 13
#define NV4_CHANNEL14 14
#define NV4_CHANNEL15 15
#define NV4_CHANNEL16 16

/* MCU -> NV4 */
#define NV4_INHIBIT 130
#define NV4_UNINHIBIT 150
#define NV4_ENABLE_ESCROW 170
#define NV4_DISABLE_ESCROW 171
#define NV4_ACCEPT_ESCROW 172
#define NV4_REJECT_ESROW 173
#define NV4_ENABLE_ALL 184
#define NV4_DISABLE_ALL 185

/* NV4 -> MCU */
#define NV4_ACCEPT 0
#define NV4_NOT_RECOGNISED 20
#define NV4_RUNNING_SLOW 30
#define NV4_STRIM_ATTEMPT 40
#define NV4_FRAUD_REJECT 50
#define NV4_STACKER_FULL 60
#define NV4_ESCROW_ABORT 70
#define NV4_NOTE_TAKEN 80
#define NV4_BUSY 120
#define NV4_NOT_BUSY 121
#define NV4_CMD_ERROR 255

typedef void (*nv4_cmd_rcv)(uint8_t cmd, uint8_t value);
static nv4_cmd_rcv cmd_rcv_cb;

uint8_t nv4_loop();
void nv4_init(nv4_cmd_rcv cb); // Init using H/W serial
void nv4_init(nv4_cmd_rcv cb, uint8_t rx, uint8_t tx); // Init using SoftSerial on given pins
void nv4_inhibit(uint8_t channel);
void nv4_uninhibit(uint8_t channel);
void nv4_enable_escrow();
void nv4_disable_escrow();
void nv4_accept_escrow();
void nv4_reject_escrow();
void nv4_enable_all();
void nv4_disable_all();
