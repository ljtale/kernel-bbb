# ljtale starts
# replace the compiled module with one support universal driver, keep the
# original in case things are messed up
obj-$(CONFIG_EEPROM_AT24)	+= at24.o
# obj-$(CONFIG_EEPROM_AT24)	+= at24-uni.o
# ljtale ends 
obj-$(CONFIG_EEPROM_AT25)	+= at25.o
obj-$(CONFIG_EEPROM_LEGACY)	+= eeprom.o
obj-$(CONFIG_EEPROM_MAX6875)	+= max6875.o
obj-$(CONFIG_EEPROM_93CX6)	+= eeprom_93cx6.o
obj-$(CONFIG_EEPROM_93XX46)	+= eeprom_93xx46.o
obj-$(CONFIG_EEPROM_DIGSY_MTC_CFG) += digsy_mtc_eeprom.o
CFLAGS_at24.o += -DDEBUG
