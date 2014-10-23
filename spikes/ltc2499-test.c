#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>

/* Single-Ended Channels Configuration */
#define LTC2499_CH0            0xB0
#define LTC2499_CH1            0xB8
#define LTC2499_CH2            0xB1
#define LTC2499_CH3            0xB9
#define LTC2499_CH4            0xB2
#define LTC2499_CH5            0xBA
#define LTC2499_CH6            0xB3
#define LTC2499_CH7            0xBB
#define LTC2499_CH8            0xB4
#define LTC2499_CH9            0xBC
#define LTC2499_CH10           0xB5
#define LTC2499_CH11           0xBD
#define LTC2499_CH12           0xB6
#define LTC2499_CH13           0xBE
#define LTC2499_CH14           0xB7
#define LTC2499_CH15           0xBF

/* Differential Channels Configuration */
#define LTC2499_P0_N1          0xA0
#define LTC2499_P1_N0          0xA8
#define LTC2499_P2_N3          0xA1
#define LTC2499_P3_N2          0xA9
#define LTC2499_P4_N5          0xA2
#define LTC2499_P5_N4          0xAA
#define LTC2499_P6_N7          0xA3
#define LTC2499_P7_N6          0xAB
#define LTC2499_P8_N9          0xA4
#define LTC2499_P9_N8          0xAC
#define LTC2499_P10_N11        0xA5
#define LTC2499_P11_N10        0xAD
#define LTC2499_P12_N13        0xA6
#define LTC2499_P13_N12        0xAE
#define LTC2499_P14_N15        0xA7
#define LTC2499_P15_N14        0xAF

/* Mode Configuration for High Speed Family */
#define LTC2499_KEEP_PREVIOUS_MODE              0x80
#define LTC2499_KEEP_PREVIOUS_SPEED_RESOLUTION  0x00
#define LTC2499_SPEED_1X                        0x00
#define LTC2499_SPEED_2X                        0x08
#define LTC2499_INTERNAL_TEMP                   0xC0

/* Select rejection frequency - 50, 55, or 60Hz */
#define LTC2499_R50    		0b10010000
#define LTC2499_R50_R60    	0b10000000
#define LTC2499_R60    		0b10100000

#define LTC2499_VREF	    5.0
#define LTC2499_FS_VOLTAGE  LTC2499_VREF/2
#define LTC2499_FS_VAL	    16777216	// 2 ^ 24

#define NET_RESISTOR	    (51200.0 * 2)
#define SEMITEC_B	    3435.0	  // in Kelvin
#define SEMITEC_R25	    10000.0

int main(void) {

   int file;
   int adapter_nr = 1;	/* ADC connected on JI2C2 */
   char filename[20];	
  
   sprintf(filename,"/dev/i2c-%d",adapter_nr);
   if ((file = open(filename,O_RDWR)) < 0) {
      printf("ERROR: file open failed\n");
      return(1);
   }

   int addr = 0x76; /* LTC2499 I2C address */
   if (ioctl(file,I2C_SLAVE,addr) < 0) {
      printf("ERROR: I2C slave select failed\n");      
      return(1);
   }

   char adc_command[2];
   adc_command[0] = LTC2499_P14_N15;			// select channel 0 differential CH14 = IN+  CH15 = IN-
   //adc_command[0] = LTC2499_CH0;			// select channel 0 single ended
   adc_command[1] = LTC2499_R50_R60 | LTC2499_SPEED_1X;	// set to 1X mode with 50 Hz and 60 Hz rejection

   char adc_output[4] = {0};

   uint32_t adc_code;
   int32_t signedvalue;
   int retval;
   uint16_t ncount, maxcount;
   double voltage, current, temperature;
   double selfheating;

   retval = write(file, adc_command, 2);
 
   if(retval != 2) {
      printf("ERROR: I2C write ADC command failed - retval = %d\n", retval);
   }

   maxcount = 2000;

   while(1) {

      ncount = 0;

      while(1) {

         retval = read(file, adc_output, 4);

         if( (retval == 4) || (ncount > maxcount) )
            break;

         ncount++;
 
         usleep(10000); // sleep for 10 msec
      }
   
      if(retval != 4) {

         printf("ERROR: I2C read ADC command failed - retval = %d\n", retval);

      } else {

         printf("conversion time: %d msec\n", ncount * 10);

         adc_code = adc_output[0];
         adc_code = (adc_code << 8) + adc_output[1];
	 adc_code = (adc_code << 8) + adc_output[2];
	 adc_code = (adc_code << 8) + adc_output[3];

         printf("adc_output[3] = %d - adc_output[2] = %d - adc_output[1] = %d - adc_output[0] = %d\n", adc_output[3], adc_output[2], adc_output[1], adc_output[0]);

         if(adc_code == 0xC0000000) {
		
	    //overflow condition Vin >= Vref/2 
	    signedvalue = 0x00FFFFFF;
            printf("overflow condition\n");

         } else if(adc_code == 0x3FFFFFFF) {

	    //underflow condition Vin < -(Vref/2)
	    signedvalue=-0x00FFFFFF;
            printf("underflow condition\n");

         } else { 

            adc_code = (adc_code & 0x7FFFFFC0) >> 6;
	    adc_code = (adc_code & 0x01FFFFFF);		// just to be sure...

            if(adc_code == 0)
               signedvalue = 0;
            else if(adc_code <= 0x00FFFFFF)		// 24 bit with value '1'
	       signedvalue = adc_code;
            else if(adc_code > 0x00FFFFFF)		// sign bit is '1'
               signedvalue = -( (~adc_code & 0x01FFFFFF) + 1);

            voltage = ((double)signedvalue/ LTC2499_FS_VAL) * LTC2499_FS_VOLTAGE;

	    current = (LTC2499_VREF - voltage) / NET_RESISTOR;

            temperature = SEMITEC_B / log( (voltage / current) / (SEMITEC_R25 * exp(-(SEMITEC_B/298.15)) ) ) - 273.15;

            selfheating = pow(voltage, 2) / (0.002 * (voltage/current));

            printf("adc_code = %ld\n", (long int) signedvalue);
            printf("voltage = %f\n", voltage); 
            printf("current = %f\n", current);
	    printf("resistance = %.0f\n", voltage/current);
            printf("temperature = %f\n", temperature); 
            printf("selfheating = %f\n", selfheating); 
            printf("--\n"); 
         }
      }

   }
 
   return(0);
}
