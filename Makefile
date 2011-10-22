BUILD_TYPE = MULTI_SRC_BIN
BIN_NAME   = sussit
LIBS       = pthread stdc++

MAKES ?= ../makes

include $(MAKES)/defs.make

include $(MAKES)/rules.make


