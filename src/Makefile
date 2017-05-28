BASE = /opt/androidStudio/androidSDK/ndk-bundle

# 32BIT DEFINES
ROOT32 = $(BASE)/platforms/android-22/arch-arm/usr/
BIN_TOOLS = $(BASE)/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/
CFLAGS32 = -std=gnu99 -O0 -march='armv7-a' --sysroot=$(ROOT32)../ -D__DEBUG__ -fno-tree-loop-distribute-patterns -fvisibility=hidden 
CC = $(BIN_TOOLS)arm-linux-androideabi-gcc
STRIP = $(BIN_TOOLS)arm-linux-androideabi-strip
AS = $(BIN_TOOLS)arm-linux-androideabi-as

# 64BIT DEFINES
ROOT64 = $(BASE)/platforms/android-21/arch-arm64/usr/
BIN_TOOLS64 = $(BASE)/toolchains/aarch64-linux-android-4.9/prebuilt/linux-x86_64/bin/
CFLAGS64 = -std=gnu99 -O0 -march='armv8-a' --sysroot=$(ROOT64)../ -DARM64 -D__DEBUG__  -fno-tree-loop-distribute-patterns -fvisibility=hidden
CC64 = $(BIN_TOOLS64)aarch64-linux-android-gcc
STRIP64 = $(BIN_TOOLS64)aarch64-linux-android-strip
AS64 = $(BIN_TOOLS64)aarch64-linux-android-as

CCC = $(CC)
CFLAGS = $(CFLAGS32)
ROOT = $(ROOT32)

all: main pgch

main: CFLAGS += -g
main: main.o
	$(CCC) -fpic -pie --sysroot=$(ROOT)../ $(CFLAGS) -lc -llog $^ -o main
	
pgch: CFLAGS += -g
pgch: pgch.o
	$(CCC) -fpic -pie --sysroot=$(ROOT)../ $(CFLAGS) -lc -llog $^ -o pgch

%.o : %.c
	$(CCC) -c $(CFLAGS) $< -o $@

deploy:
	adb push pgch /data/local/tmp/test/
	
clean:
	rm pgch main *.o 


