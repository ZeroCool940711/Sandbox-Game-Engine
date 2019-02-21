ASMS =
MY_LIBS := unit_test_lib $(MY_LIBS)
INSTALL_ALL_CONFIGS = 1
INSTALL_DIR = $(MF_ROOT)/tests
USE_CPPUNITLITE2 = 1

ifdef LIB_NAME
	BIN = $(LIB_NAME)_test
	NO_EXTRA_LIBS = 1
	USE_PYTHON =
else
	BIN := $(BIN)_test
endif


include $(MF_ROOT)/bigworld/src/server/common/common.mak

all::

run:
	$(OUTPUTFILE)

test:
ifdef QUIET_BUILD
	cd `dirname $(OUTPUTFILE)` && $(OUTPUTFILE)
else
	cd `dirname $(OUTPUTFILE)` && $(OUTPUTFILE) --verbose
endif
