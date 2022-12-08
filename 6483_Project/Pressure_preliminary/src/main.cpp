#include "mbed.h"

#define SPI_MOSI PA_7
#define SPI_MISO PA_6
#define SPI_SCLK PA_5

SPI spi(SPI_MOSI, SPI_MISO, SPI_SCLK); // mosi, miso, sclk
DigitalOut cs(PC_3);

uint8_t write_buf[32]; 
uint8_t read_buf[32];
EventFlags flags;
#define SPI_FLAG 1

//The spi.transfer() function requires that the callback
//provided to it takes an int parameter
void spi_callback(int event) {
  //deselect the sensor
  cs=1;
  flags.set(SPI_FLAG);
  }

double getPressure() {
		// Send 0xAA, 0x00, and 0x00 over spi bus to communicate w sensor
      write_buf[0]=0xAA;
	  write_buf[1] = 0x00;
	  write_buf[2] = 0x00;
      // Select the device by seting chip select low
      cs=0;
	  // Asynchronous transfer
      spi.transfer(write_buf,3,read_buf,4,spi_callback,SPI_EVENT_COMPLETE );
      // Wait for spi transfer to complete
      flags.wait_all(SPI_FLAG);
	  // evil bit hack to glue data together
      unsigned long raw_data = ((long)read_buf[1] << 16) + ((long)read_buf[2] << 8) + ((long)read_buf[3]);
	  double pressure = raw_data;
		pressure -= 419430;
    	pressure *= 300;
    	pressure /= (3774874 - 419430);
		return pressure;
}

int main() {
    // Chip must be deselected
    cs = 1;
 
    // Setup the spi for 8 bit data, high steady state clock,
    // second edge capture, with a 1MHz clock rate
    spi.format(8,0);
    spi.frequency(200'000);
 
    while (1) {
	  double p = getPressure();
	  printf("Pressure: %.17g\n",p);
      thread_sleep_for(1'000);

    }
}