CC = gcc
CFLAGS = -pedantic -Wall -Wextra -std=c90 -O2
LDFLAGS = -W1,--strip-all

TARGET = texture

SRCDIR = src
OBJDIR = obj
BINDIR = bin

SRCS = $(wildcard $(SRCDIR)/*.c)
INCS = $(wildcard $(SRCDIR)/*.h)
OBJS = $(SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
DEPS = $(OBJS:$(OBJDIR)/%.o=$(OBJDIR)/%.d)

$(BINDIR)/$(TARGET): $(OBJS)
	@$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

$(OBJS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	@$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)

$(DEPS): $(OBJDIR)/%.d : $(SRCDIR)/%.c
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

.PHONY: clean
clean:
	rm -f $(OBJS)
	rm -f $(DEPS)
	rm -f $(BINDIR)/$(TARGET)
