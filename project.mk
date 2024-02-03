# This file can be used to set build configuration
# variables.  These variables are defined in a file called 
# "Makefile" that is located next to this one.

# For instructions on how to use this system, see
# https://analog-devices-msdk.github.io/msdk/USERGUIDE/#build-system

#BOARD=FTHR_RevA
# ^ For example, you can uncomment this line to make the 
# project build for the "FTHR_RevA" board.

# **********************************************************

# Add your config here!

# If you have secure version of MCU (MAX32651), set SBT=1 to generate signed binary
# For more information on how sing process works, see
# https://www.analog.com/en/education/education-library/videos/6313214207112.html
SBT=1

# Enable MAXUSB library
LIB_MAXUSB=1

# Enable the SDHC library
LIB_SDHC=1

# Pick the SDHC FF revision, currently selecting the newer version 14 (13 and 14 will build, 15 needs tweaks to compile)
FAT32_DRIVER_DIR = $(SDHC_DRIVER_DIR)/ff13
#FAT32_DRIVER_DIR = $(SDHC_DRIVER_DIR)/ff14
#FAT32_DRIVER_DIR = $(SDHC_DRIVER_DIR)/ff15
