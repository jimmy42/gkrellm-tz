# Timezone plugin for GKrellM
# Copyright (C) 2002--2012 Jiri Denemark
#
# This file is part of gkrellm-tz.
#
# gkrellm-tz is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

VERSION	= "$(shell git describe --tags 2>/dev/null | sed 's/^v//')"
ifeq ($(VERSION), "")
    VERSION	= 0.8
endif

GKRELLM_CFLAGS	= $(shell pkg-config gkrellm --cflags)
GKRELLM_LDFLAGS	= $(shell pkg-config gkrellm --libs)
CFLAGS += $(shell dpkg-buildflags --get CPPFLAGS)
CFLAGS += -fPIC -Wall -Werror -g $(GKRELLM_CFLAGS) -DVERSION=\"$(VERSION)\"
LDFLAGS += -shared $(GKRELLM_LDFLAGS)

OBJS	= list.o config.o gkrellm-tz.o

.PHONY: all clean install

all:
	@echo "Making gkrellm-tz version $(VERSION)"
	@$(MAKE) --no-print-directory gkrellm-tz.so

DEFAULT	:= 0
V_CC	= $(V_CC_$(V))
V_CC_	= $(V_CC_$(DEFAULT))
V_CC_0	= @echo "  CC  " $@;

V_LD	= $(V_LD_$(V))
V_LD_	= $(V_LD_$(DEFAULT))
V_LD_0	= @echo "  LD  " $@;

gkrellm-tz.so: $(OBJS) Makefile
	$(V_LD)$(CC) $(LDFLAGS) $(OBJS) -o $@

gkrellm-tz.o: gkrellm-tz.c $(patsubst %.o,%.h,$(OBJS)) Makefile features.h
	$(V_CC)$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c %.h Makefile features.h
	$(V_CC)$(CC) $(CFLAGS) -c $< -o $@

install: clean all
	install -D -s -m 644 gkrellm-tz.so $(DESTDIR)/usr/lib/gkrellm2/plugins/gkrellm-tz.so

uninstall:
	rm -f $(DESTDIR)/usr/lib/gkrellm2/plugins/gkrellm-tz.so

clean:
	rm -f $(OBJS) gkrellm-tz.so
