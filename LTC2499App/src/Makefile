TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = LTC2499
# LTC2499.dbd will be created and installed
DBD += LTC2499.dbd
DBD += semitec103at2.dbd

# LTC2499.dbd will be made up from these files:
LTC2499_DBD += base.dbd

# Include dbd files from all support applications:
LTC2499_DBD += LTC2499dev.dbd
LTC2499_DBD += semitec103at2.dbd

# Add all the support libraries needed by this IOC
#LTC2499_LIBS += xxx

# LTC2499_registerRecordDeviceDriver.cpp derives from LTC2499.dbd
LTC2499_SRCS += LTC2499_registerRecordDeviceDriver.cpp
LTC2499_SRCS += devLTC2499.c
#LTC2499_SRCS += drvLTC2499.c

# Build the main IOC entry point on workstation OSs.
LTC2499_SRCS_DEFAULT += LTC2499Main.cpp
LTC2499_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#LTC2499_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
LTC2499_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

