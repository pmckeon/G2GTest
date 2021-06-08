# program executable file name.
PROG = G2GTest
# compiler
CC = sdcc
# linker
LD = sdcc
# delete command
RM = del /f
# Compiler flags go here.
CFLAGS = -MMD -c -mz80 --peep-file peep-rules.txt
# Linker flags go here.
LDFLAGS = -mz80 --no-std-crt0 --data-loc 0xC000
CRT0 = crt0_sms.rel
SMSLIB = SMSlib_GG.lib
OBJS = $(PROG).rel
DEPS = $(OBJS:.rel=.d)

$(PROG).gg: $(PROG).ihx
	ihx2sms $< $@

$(PROG).ihx: $(OBJS)
	-$(LD) -o $@ $(LDFLAGS) $(CRT0) $^ $(SMSLIB)

%.rel: %.c
	$(CC) $(CFLAGS) $<

.PHONY: clean
clean:
	-$(RM) *.gg *.ihx *.asm $(OBJS) $(DEPS)

-include $(DEPS)