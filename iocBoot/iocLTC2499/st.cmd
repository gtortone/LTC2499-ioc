#!../../bin/linux-arm/LTC2499
#
# for single ended channels:
# C 	channel
# -------------
# 0 -> CH0,  1 -> CH1,   2 -> CH2,   3 -> CH3,   4 -> CH4,   5 -> CH5,   6 -> CH6,   7 -> CH7,   8 -> CH8,
# 9 -> CH9, 10 -> CH10, 11 -> CH11, 12 -> CH12, 13 -> CH13, 14 -> CH14, 15 -> CH15
#
# for differential channels:
# C 	channel
# -------------
# 0 -> P0N1,   1 -> P1N0,    2 -> P2N3,    3 -> P3N2,    4 -> P4N5,    5 -> P5N4,    6 -> P6N7,  7 -> P7N6,  8 -> P8N9, 
# 9 -> P9N8,  10 -> P10N11, 11 -> P11N10, 12 -> P12N13, 13 -> P13N12, 14 -> P14N15, 15 -> P15N14
#
# for internal temperature sensor
# C 	channel
# -------------
# 16 -> internal temperature sensor

< envPaths

cd ${TOP}

## Register all support components
dbLoadDatabase "dbd/LTC2499.dbd"
LTC2499_registerRecordDeviceDriver pdbbase

var dbBptNotMonotonic 1
dbLoadRecords("dbd/semitec103at2.dbd")

## configure module I2C bus number, I2C module address, ADC VREF
epicsEnvSet("VREF","5.0")
epicsEnvSet("HALFVREF","2.5")
devLTC2499config(1, 0x76, $(VREF))

## Load record instances
#
# !!!! channel 0 and channel 1 (DIFF) reserved for noise floor histogram plotting 
# !!!! channel 14 and channel 15 (DIFF) reserver for RTD 
#
# example:
#
dbLoadRecords("db/LTC2499.db","P=LTC2499,D=ADC channel,C=2,M=SE")

cd ${TOP}/iocBoot/${IOC}
iocInit
