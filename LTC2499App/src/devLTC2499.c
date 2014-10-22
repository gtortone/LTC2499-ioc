#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <epicsExport.h>
#include <epicsThread.h>
#include <epicsMutex.h>
#include <dbAccess.h>
#include <devSup.h>
#include <recGbl.h>
#include <alarm.h>
#include <errlog.h>

#include <aiRecord.h>
#include <mbboRecord.h>
#include <iocsh.h>
#include "devLTC2499.h"

static long init_device(int phase);

static long init_ai_record(aiRecord *pai);
static long read_ai(aiRecord *pai);

static long init_mbbo_record(mbboRecord *pmbbo, int pass);
static long write_mbbo(mbboRecord *pmbbo);

struct busConfig {

   int adapter_nr;              // I2C bus number
   int adapter_addr;            // I2C address
   double vref;			// ADC voltage reference
   char filename[20];		// I2C device file name
   int fd;			// I2C device file descriptor
};

struct channelConfig {

   int channel;			// channel number
   char mode[8];		// SE, DIFF
   int word;			// word to configure ADC 
   int resolution;		// 24 or 30 bits
};

struct {

   long num;
   DEVSUPFUN  report;
   DEVSUPFUN  init;
   DEVSUPFUN  init_record;
   DEVSUPFUN  get_ioint_info;
   DEVSUPFUN  read_ai;
   DEVSUPFUN  special_linconv;

} devAiLTC2499 = {

   6,
   NULL,
   init_device,
   init_ai_record,
   NULL,
   read_ai,
   NULL
};
struct {

   long            number;
   DEVSUPFUN       report;
   DEVSUPFUN       init;
   DEVSUPFUN       init_record;
   DEVSUPFUN       get_ioint_info;
   DEVSUPFUN       write_mbbo;

} devMbboLTC2499 = {

   5,
   NULL,
   NULL,
   init_mbbo_record,
   NULL,
   write_mbbo 
};

// global vars for rejection and speed config words
static uint8_t g_rejection, g_speed;

// global vars for I2C bus number, LTC2499 I2C address and ADC VREF
static struct busConfig i2cbus;

// mutex to guarantee exclusive access between thread for different timeperiod
epicsMutexId mutex;

// device configuration

int devLTC2499config(int busno, int addr, double vref) {

    i2cbus.adapter_nr = busno;
    i2cbus.adapter_addr = addr;
    i2cbus.vref = vref;
    return(0);
}

static const iocshArg configArg0 = {"i2cbusno", iocshArgInt};
static const iocshArg configArg1 = {"i2caddr", iocshArgInt};
static const iocshArg configArg2 = {"vref", iocshArgDouble};

static const iocshArg *configArgs[] = {
    &configArg0, &configArg1, &configArg2
};

static const iocshFuncDef configFuncDef = {
    "devLTC2499config", 3, configArgs
};

static void devLTC2499configCallFunc(const iocshArgBuf *args) {

    devLTC2499config(args[0].ival, args[1].ival, args[2].dval);
}

static void devLTC2499Registrar() {

    iocshRegister(&configFuncDef, devLTC2499configCallFunc);
}

epicsExportRegistrar(devLTC2499Registrar);

// device functions

static long init_device(int phase) {

   if(phase == 0) {

      mutex = epicsMutexCreate();
 
      sprintf(i2cbus.filename, "/dev/i2c-%d", i2cbus.adapter_nr);
      if ((i2cbus.fd = open(i2cbus.filename, O_RDWR)) < 0) {
         errlogPrintf("ERROR: file %s open failed\n", i2cbus.filename);
         return(S_dev_noDevSup);
      }
   }   

   return(0);
}

static long init_ai_record(aiRecord *pai) {
 
   struct channelConfig* chconf;
   int retval;


   chconf = malloc(sizeof(struct channelConfig));
   if(!chconf){
     recGblRecordError(S_db_noMemory, (void*)pai, "devAiLTC2499: failed to allocate private struct");
     return S_db_noMemory;
   }
 
   retval = sscanf(pai->inp.value.instio.string, "%d:%[^:]:%d", &chconf->channel, chconf->mode, &chconf->resolution);

   if(retval != 3) {

      recGblRecordError(S_db_badField, (void*)pai, "devAiLTC2499: illegal input parameters format");
   }

   if( (strcasecmp(chconf->mode, "diff") == 0) && (chconf->channel >= 0) && (chconf->channel <= 15) ) {

      chconf->word = chan_diff_config[chconf->channel];
   
   } else if( (strcasecmp(chconf->mode, "se") == 0) && (chconf->channel >= 0) && (chconf->channel <= 15) ) {

      chconf->word = chan_se_config[chconf->channel];
   
   } else if(chconf->channel == 16) {	// internal temperature sensor

      chconf->word = LTC2499_INTERNAL_TEMP;

   } else {

      recGblRecordError(S_db_badField, (void*)pai, "devAiLTC2499: illegal input parameters values");
   }

   if( (chconf->resolution != 30) && (chconf->resolution != 24) )
      recGblRecordError(S_db_badField, (void*)pai, "devAiLTC2499: illegal input parameters values");

   pai->dpvt = chconf;
   return(0); 
}

static long read_ai(aiRecord *pai) {

   struct channelConfig* chconf = pai->dpvt;

   char adc_command[2];
   char adc_output[4] = {0};
   uint32_t adc_code;
   int32_t signedvalue = 0;
   int fd = i2cbus.fd;

   struct i2c_msg msg[2];
   struct i2c_rdwr_ioctl_data rdwr;

   int maxcount, ncount;

   if(chconf->word == LTC2499_INTERNAL_TEMP) {

      adc_command[0] = LTC2499_CH0;	// fake channel
      adc_command[1] = chconf->word | g_rejection;

   } else {

      adc_command[0] = chconf->word;
      adc_command[1] = g_rejection | g_speed;
   }

   msg[0].addr= 0x76;
   msg[0].flags = 0;
   msg[0].len = 2;
   msg[0].buf = adc_command;

   msg[1].addr= 0x76;
   msg[1].flags = I2C_M_RD;
   msg[1].len = 4;
   msg[1].buf = adc_output;

   rdwr.nmsgs = 2;
   rdwr.msgs = msg;

   maxcount = 200;

   // void read in order to skip converted sample of previous channel
   epicsMutexLock(mutex);
   ncount = 0;
   while(ncount < maxcount) {

      if (ioctl(fd, I2C_RDWR, &rdwr) < 0) {

         ncount++;
         usleep(10000); // sleep for 10 msec

      } else break;
   }

   ncount = 0;
   while(ncount < maxcount) {

      if (ioctl(fd, I2C_RDWR, &rdwr) < 0) {
     
         ncount++;
         usleep(10000); // sleep for 10 msec
      
      } else break;
   }
   epicsMutexUnlock(mutex);

   if (ncount == maxcount) {

      pai->udf = TRUE;
      recGblRecordError(S_db_badField, (void*)pai, "devAiLTC2499: bus transfer error");

   } else {
 
      pai->udf = FALSE;

      adc_code = adc_output[0];
      adc_code = (adc_code << 8) + adc_output[1];
      adc_code = (adc_code << 8) + adc_output[2];
      adc_code = (adc_code << 8) + adc_output[3];
 
      if(chconf->resolution == 30) {		// start of 30 bit conversion

         if(adc_code == 0xC0000000) {
 
            //overflow condition Vin >= Vref/2 
            signedvalue = 0x3FFFFFFF;
    
         } else if(adc_code == 0x3FFFFFFF) {
 
            //underflow condition Vin < -(Vref/2)
            signedvalue=-0x3FFFFFFF;
       
         } else {
   
            if( (adc_code == 0) || (adc_code == 0x80000000) )
            signedvalue = 0;
            else if(adc_code > 0x80000000)             
               signedvalue = adc_code & 0x3FFFFFFF;
            else if(adc_code < 0x80000000)              // sign bit is '1'
               signedvalue = -( (~adc_code & 0x3FFFFFFF) + 1);
         }
      }						// end of 30 bit conversion
      
      if(chconf->resolution == 24) {		// start of 24 bit conversion

         if(adc_code == 0xC0000000) {
 
            //overflow condition Vin >= Vref/2 
            signedvalue = 0x00FFFFFF;
 
         } else if(adc_code == 0x3FFFFFFF) {
 
            //underflow condition Vin < -(Vref/2)
            signedvalue=-0x00FFFFFF;
 
         } else {
   
            adc_code = (adc_code & 0x7FFFFFC0) >> 6;
            adc_code = (adc_code & 0x01FFFFFF);         // just to be sure...
 
            if(adc_code == 0)
               signedvalue = 0;
            else if(adc_code <= 0x00FFFFFF)             // 24 bit with value '1'
               signedvalue = adc_code;
            else if(adc_code > 0x00FFFFFF)              // sign bit is '1'
               signedvalue = -( (~adc_code & 0x01FFFFFF) + 1);
         }
      } 					// end of 24 bit conversion

      pai->rval = signedvalue;
   } 
 
   return 0;
}

static long init_mbbo_record(mbboRecord *pmbbo, int pass) {

   pmbbo->udf = FALSE;
   return(0);
}
 
static long write_mbbo(mbboRecord *pmbbo) {

   int retval;
   char param[16];

   retval = sscanf(pmbbo->name, "%*[^:]:%*[^:]:%s", param); 

   if(retval != 1) {

      errlogPrintf("ERROR: field name not correct\n");      
      pmbbo->udf = TRUE;
      return(S_db_badField);
   }

   pmbbo->udf = FALSE;

   if(strcasecmp(param, (char *) "speed") == 0) {

      g_speed = pmbbo->rval;
   
   } else if(strcasecmp(param, (char *) "rejection") == 0) {
     
     g_rejection = pmbbo->rval;
   
   } else {
  
      errlogPrintf("ERROR: parameter name not correct\n");      
      pmbbo->udf = TRUE;
      return(S_db_badField);
   }
      
   return(0);
}

epicsExportAddress(dset,devAiLTC2499);
epicsExportAddress(dset,devMbboLTC2499);
