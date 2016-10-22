
#define BIT(n) (0x01<<(n))
#define IRQ_KBD 1
#define DELAY_US 20000
#define STAT_REG 0x64
#define IN_BUF 0x60
#define OUT_BUF 0x60
#define GET_LSB 0x01
#define ESC_BREAK 0x81
#define OBF BIT(0)
#define IBF BIT(1)
#define TIMEOUT BIT(6)
#define PARITY BIT(7)
#define OK 0
#define NTRIES 5

int kbd_scan_loop();
int kbd_subscribe_int(int *hookid);
int kbd_unsubscribe_int(int *hookid);
unsigned long kbd_read_code();