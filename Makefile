TARGET = game_categories_light
OBJS = main.o logger.o category.o io.o scePaf.o clearcache.o gcpatches.o multims.o redirects.o
LIBS = -lpspsystemctrl_user -lpspsysmem_user -lpsprtc

CFLAGS = -Os -G0 -Wall -fno-pic -fshort-wchar
# -DKPRINTF_ENABLED
ASFLAGS = $(CFLAGS)

BUILD_PRX = 1
PRX_EXPORTS = exports.exp

USE_USER_LIBS = 1
USE_USER_LIBC = 1

PSP_FW_VERSION=620

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build_prx.mak