#include "mouse.h"
#include "timer.h"

void mouse_print_packet(unsigned char packet[]){
	printf("B1=0x%x ", packet[0]);
	printf("B2=0x%x ", packet[1]);
	printf("B3=0x%x ", packet[2]);
	printf("LB=%u ", packet[0] & BIT(0));
	printf("MB=%u ", (packet[0] & BIT(2))>>2);
	printf("RB=%u ", (packet[0] & BIT(1))>>1);
	printf("XOV=%u ", (packet[0] & BIT(6))>>6);
	printf("YOV=%u ", (packet[0] & BIT(7))>>7);
	printf("X=%d ", ((packet[0] & BIT(4)) ? TWOSCOMPLEMENT(packet[1]) : packet[1]));
	printf("Y=%d\n\n", ((packet[0] & BIT(5)) ? TWOSCOMPLEMENT(packet[2]) : packet[2]));

}

int test_packet(unsigned short cnt){
	int r;
	int irq_set = mouse_subscribe_int();

	if(irq_set == -1) //Failed subscription
		return -1;

	int ipc_status;
	message msg;

	unsigned short number_of_packets = 0; //number of full packets processed
	unsigned short counter = 0;
	unsigned char packet[3];
	long byte; //Auxiliar variable that will store each byte read

	if(mouse_write_byte(ENABLE_MOUSE_DATA_REPORTING) == -1)
			return -1;

	while(number_of_packets < cnt) {
		/* Get a request message. */
		if ( (r = driver_receive(ANY, &msg, &ipc_status)) != 0 ) {
			printf("driver_receive failed with: %d", r);
			continue;
		}
		if (is_ipc_notify(ipc_status)) { /* received notification */
			switch (_ENDPOINT_P(msg.m_source)) {
			case HARDWARE: /* hardware interrupt notification */
				if (msg.NOTIFY_ARG & irq_set) { /* subscribed interrupt */
					sys_inb(OUT_BUF, &byte);
					if(counter == 0){
						if(byte & BIT(3)) //Valid first byte (or at least we hope so)
							packet[counter++] = (unsigned char)byte;
						else{
							printf("Invalid first byte, trying again\n");
							continue;
						}
					}
					else{
						packet[counter++] = (unsigned char)byte;
						if(counter > 2){
							mouse_print_packet(packet);
							counter = 0;
							number_of_packets++;
						}
					}
				}
				break;
			default:
				break; /* no other notifications expected: do nothing */
			}
		} else { /* received a standard message, not a notification */
			/* no standard messages expected: do nothing */
		}
	}

	if(mouse_write_byte(DISABLE_MOUSE_DATA_REPORTING) == -1)
			return -1;

	return mouse_unsubscribe_int();
}

int test_async(unsigned short idle_time) {
	int r;
	int irq_set = mouse_subscribe_int();

	if(irq_set == -1) //Failed subscription
		return -1;

    int timer_irq_set = timer_subscribe_int();
    if(timer_irq_set == -1) //Failed timer subscription
    	 return -1;

	int ipc_status;
	message msg;

	unsigned short counter = 0;
	unsigned char packet[3];
	long byte; //Auxiliar variable that will store each byte read

	unsigned int idle_counter = 0;

	if(mouse_write_byte(ENABLE_MOUSE_DATA_REPORTING) == -1)
			return -1;

	while(idle_counter/60 < idle_time) {
		/* Get a request message. */
		if ( (r = driver_receive(ANY, &msg, &ipc_status)) != 0 ) {
			printf("driver_receive failed with: %d", r);
			continue;
		}
		if (is_ipc_notify(ipc_status)) { /* received notification */
			switch (_ENDPOINT_P(msg.m_source)) {
			case HARDWARE: /* hardware interrupt notification */
				if (msg.NOTIFY_ARG & irq_set) { /* subscribed interrupt */
					sys_inb(OUT_BUF, &byte);
					idle_counter = 0; //Mouse has caused an interrupted so idle time is reset
					if(counter == 0){
						if(byte & BIT(3)) { //Valid first byte (or at least we hope so)
							packet[counter++] = (unsigned char)byte;
						}
						else{
							printf("Invalid first byte so invalid packet, trying again\n\n");
							continue;
						}
					}
					else{
						packet[counter++] = (unsigned char)byte;
						if(counter > 2) {
							mouse_print_packet(packet);
							counter = 0;
						}
					}
				}
				else if(msg.NOTIFY_ARG & timer_irq_set) {
					idle_counter++;
					if(idle_counter % 60 == 0)
						printf("No mouse action detected in the last second. %ds until program exits.\n", idle_time-idle_counter/60);
				}
				break;
			default:
				break; /* no other notifications expected: do nothing */
			}
		} else { /* received a standard message, not a notification */
			/* no standard messages expected: do nothing */
		}
	}

	if(mouse_write_byte(DISABLE_MOUSE_DATA_REPORTING) == -1)
			return -1;

	if(mouse_unsubscribe_int() == -1 || timer_unsubscribe_int() == -1)
		return -1;
	else
		return 0;
}

void mouse_print_config(unsigned char packet[]){
	printf("Current mouse configuration:\n");
	printf("Mode: %s", (packet[0] & BIT(6)) ? "Remote (polled)\n" : "Stream\n");
	printf("Data reporting is %s", (packet[0] & BIT(5)) ? "enabled\n" : "disabled\n");
	printf("Scaling is %s", (packet[0] & BIT(4)) ? "2:1\n" : "1:1\n");
	printf("Middle button is %s", (packet[0] & BIT(2)) ? "pressed\n" : "released\n");
	printf("Right button is %s", (packet[0] & BIT(1)) ? "pressed\n" : "released\n");
	printf("Left button is %s", (packet[0] & BIT(0)) ? "pressed\n" : "released\n");
	printf("Resolution: %d counts per mm\n", BIT(packet[1])); //packet[1] returns 0,1,2 or 3. The corresponding value is 2^value. BIT(n) does that
	printf("Sample rate: %d Hz\n\n", packet[2]);
}

int test_config(void) {
	if(mouse_subscribe_int() == -1) //Failed subscription
		return -1;

	unsigned short counter = 0;
	unsigned char packet[3];
	long byte; //Auxiliar variable that will store each byte read
	if(mouse_write_byte(ENABLE_MOUSE_DATA_REPORTING) == -1)
				return -1;
	if(mouse_write_byte(GET_MOUSE_CONFIG) == -1)
				return -1;

	while(counter < 3){
		sys_inb(OUT_BUF, &byte);
		packet[counter++] = (unsigned char)byte;
		tickdelay(micros_to_ticks(DELAY_US));
	}

	mouse_print_config(packet);

	if(mouse_write_byte(DISABLE_MOUSE_DATA_REPORTING) == -1)
					return -1;

	return mouse_unsubscribe_int();
}


int test_gesture(short length) {
	int r;
	int irq_set = mouse_subscribe_int();

	if(irq_set == -1) //Failed subscription
		return -1;

	int ipc_status;
	message msg;

	unsigned short number_of_packets = 0; //number of full packets processed
	unsigned short counter = 0;
	unsigned char packet[3];
	long byte; //Auxiliar variable that will store each byte read

	int mouse_event = 0;
	rbstate previousrb = ISDOWN;

	short desired_length = length;
	/* y_variation should be initialized with 0, but if we do so initial variation in case of negative 'length' is -256 (?!)
	 * initialized as 256 seems to be working for both cases...
	 */
	short y_variation = 256;

	state st = INIT; //Variable that will hold machine state


	if(mouse_write_byte(ENABLE_MOUSE_DATA_REPORTING) == -1)
		return -1;

	while(st != COMP) {
		/* Get a request message. */
		if ( (r = driver_receive(ANY, &msg, &ipc_status)) != 0 ) {
			printf("driver_receive failed with: %d", r);
			continue;
		}
		if (is_ipc_notify(ipc_status)) { /* received notification */
			switch (_ENDPOINT_P(msg.m_source)) {
			case HARDWARE: /* hardware interrupt notification */
				if (msg.NOTIFY_ARG & irq_set) { /* subscribed interrupt */
					sys_inb(OUT_BUF, &byte);
					if(counter == 0){
						if(byte & BIT(3)) //Valid first byte (or at least we hope so)
							packet[counter++] = (unsigned char)byte;
						else{
							printf("Invalid first byte, trying again\n");
							continue;
						}
					}
					else{
						packet[counter++] = (unsigned char)byte;
						if(counter > 2){
							mouse_print_packet(packet);
							counter = 0;
							mouse_event = 1;
						}
					}
				}
				break;
			default:
				break; /* no other notifications expected: do nothing */
			}
		} else { /* received a standard message, not a notification */
			/* no standard messages expected: do nothing */
		}
		if(mouse_event){
			mouse_event = 0;
			if(!(packet[0] & BIT(1)) && previousrb == ISDOWN){//Right button was pressed but got released
				previousrb = ISUP;
				mouse_event_handler(&st, RUP, &y_variation, desired_length);
			}
			else if((packet[0] & BIT(1)) && previousrb == ISDOWN){ //Right button was and is pressed

				if(length > 0) {
					if(((packet[0] & BIT(4)) && ABS_VALUE(TWOSCOMPLEMENT(packet[1])) > MAX_X_TOLERANCE) || ((packet[0] & BIT(5)) && ABS_VALUE(TWOSCOMPLEMENT(packet[2])) > MAX_Y_TOLERANCE)) { //If deltaX < 0 e ABS(deltaX) > MAX_X_TOLERANCE or same for y
						y_variation = 0;
					}
					else if(packet[0] & BIT(5))
						y_variation += TWOSCOMPLEMENT(packet[2]);
					else
						y_variation += packet[2];

				}
				else { //length < 0
					if((((packet[0] & BIT(4)) == 0) && ABS_VALUE(packet[1]) > MAX_X_TOLERANCE) || (((packet[0] & BIT(5)) == 0) && ABS_VALUE(packet[2]) > MAX_Y_TOLERANCE)) {
						y_variation = 0;
					}
					else if(packet[0] & BIT(5))
						y_variation += TWOSCOMPLEMENT(packet[2]);
					else
						y_variation += packet[2];
				}

			mouse_event_handler(&st, MOVE, &y_variation, desired_length);
			}
			else if((packet[0] & BIT(1)) && previousrb == ISUP){ //Right button was released but got pressed
				previousrb = ISDOWN;
				mouse_event_handler(&st, RDOWN, &y_variation, desired_length);
			}
		}
	}

	if(mouse_write_byte(DISABLE_MOUSE_DATA_REPORTING) == -1)
			return -1;

	return mouse_unsubscribe_int();
}
