enum IOEvent {IO_QUIT, IO_CLICK, IO_NONE};
typedef enum IOEvent IOEvent;

void io_init();
void io_getMouse(int*, int*);
IOEvent io_getEvent();