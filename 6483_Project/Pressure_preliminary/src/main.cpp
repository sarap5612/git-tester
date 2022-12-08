#include "mbed.h"


#define I2C_SDA PB_11
#define I2C_SCL PB_10
uint8_t addr = 0x18;

I2C i2c(I2C_SDA,I2C_SCL);

int getPressure();


int main() {

	while(1) {
		// put your main code here, to run repeatedly:
		// get measurement
		int reading = getPressure();
		// print measurement
		printf("Pressure: %d\n", reading);
		// wait for 1sec between pressure readings
		thread_sleep_for(1000);
	}
}

int getPressure() {
	char cmd[3];
	cmd[0] = 0xAA;
	cmd[1] = 0x00;
	cmd[2] = 0x00;
	char data[4];
	// To store individual bytes
	int err_code = i2c.write(0x30, cmd, 3, false); // send bytes
	if (err_code != 0) {
		printf("Error code: %d", err_code);
		}
	thread_sleep_for(10); // wait for the measurement for 5 ms
	int err_code_read = i2c.read(0x31, data, 4, false); // request status + 3 data bytes
	//evil bit hack to glue the data together
	unsigned long press_counts = ((long)data[1]<<16) + ((long)data[2]<<8) + ((long)data[3]);
	printf("Status: %d\n",data[0]);
	printf("Pressure counts: %d\n",data[1]);
	printf("Pressure counts: %d\n",data[2]);
	printf("Pressure counts: %d\n",data[3]);
	// do math from datasheet
	unsigned long pressure = press_counts;
  pressure -= 419430;
  pressure *= 300;
  pressure /= (3774874-419430); 
  return pressure;
}