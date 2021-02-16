# For TMote Sky (emulated in Cooja) use the following target
ifndef TARGET
	TARGET = sky
else
	# For Zolertia Firefly (testbed) use the following target and board
	# Don't forget to make clean when you are changing the board
	ifeq ($(TARGET), zoul)
		BOARD	?= firefly
		LDFLAGS += -specs=nosys.specs # For Zolertia Firefly only
	endif
endif

DEFINES=PROJECT_CONF_H=\"project-conf.h\"
CONTIKI_PROJECT = app

PROJECT_SOURCEFILES += sched_collect.c

# Tools for testbed experiments to set node IDs and estimate node duty cycle
PROJECTDIRS += tools
PROJECT_SOURCEFILES += simple-energest.c
PROJECT_SOURCEFILES += deployment.c

all: $(CONTIKI_PROJECT)


CONTIKI_WITH_RIME = 1
CONTIKI ?= ../../contiki
include $(CONTIKI)/Makefile.include
