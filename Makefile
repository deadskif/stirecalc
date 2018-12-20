CC 		= gcc
CFLAGS  = -Wall -Wextra -Werror -pedantic -std=c11 -O2 
LDFLAGS = -O2
LDLIBS 	= -lm
LEX     = flex
YACC    = bison -y
YFLAGS  = -d

# From GNU Coding standarts
# 7.2.3 Variables for Specifying Commands
# 7.2.4 DESTDIR: Support for Staged Installs
# 7.2.5 Variables for Installation Directories

prefix 		?=	/usr/local
exec_prefix ?=	$(prefix)
bindir		?=	$(exec_prefix)/bin
datarootdir ?=  $(prefix)/share
includedir	?=	$(prefix)/include
docdir		?=  $(datarootdir)/doc/$(PACKET)
libdir		?=  $(exec_prefix)/lib
srcdir		?=	$(CURDIR)

INSTALL_PROGRAM ?= $(INSTALL)
INSTALL_DATA 	?= $(INSTALL) -m 644


OBJS = stirecalc.o oem_tires.o #strhelpers.o
DEPS = $(OBJS:.o=.d)

.SUFFIXES: .y .l

all: stirecalc
#stc2: lex.o yacc.o

%.o : %.c $(MAKEFILE_LIST)
	$(CC) $(CFLAGS) -c $< -o $@

%.d: %.c $(MAKEFILE_LIST)
	$(CC) -E -MM -MQ $(<:.c=.o) $(CFLAGS) $< -o $@

stirecalc: $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

clean:
	rm $(OBJS) $(DEPS) stirecalc

ifneq ($(MAKECMDGOALS),clean)
include $(DEPS)
endif
