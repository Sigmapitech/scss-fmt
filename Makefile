CC ?= gcc

OUT := scss-fmt

CFLAGS += $(shell cat warning_flags.txt)
CFLAGS += -O2
CFLAGS += -iquote src

.PHONY: all
all: $(OUT)

$(OUT): scss-fmt.c
	$(LINK.c) -o $@ $^ $(LDLIBS)

.PHONY: fclean
fclean:
	$(RM) $(OUT)

.PHONY: re
.NOTPARALLEL: re
re: fclean all

PREFIX ?= /usr
BINDIR := $(PREFIX)/bin

.PHONY: install
install:
	mkdir -p $(BINDIR)
	install -Dm577 $(OUT) -t $(BINDIR)
