BUILD_TYPE = MULTI_SRC_BIN
BIN_NAME   = loadsuss
LIBS       = pthread stdc++

MAKES ?= ../makes

include $(MAKES)/defs.make

include $(MAKES)/rules.make


