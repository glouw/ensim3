# single translation unit build system
#
# move inactive "cold" headers to pch.hh and "hot" under-active-development
# headers to main.cc. headers dependencies are automatically tracked.

TARGET = ensim3
CXX = clang++
LDFLAGS = -lSDL2
WFLAGS = -Wall -Wextra -Wpedantic -Wnon-virtual-dtor

MODE = 2

CXXFLAGS = -std=c++23 -fno-rtti

ifeq ($(MODE), 0)
CXXFLAGS += -O0 -g0
endif

ifeq ($(MODE), 1)
CXXFLAGS += -fsanitize=address,undefined -g
endif

ifeq ($(MODE), 2)
CXXFLAGS += -Ofast -march=native
endif

ifeq ($(MODE), 3)
CXXFLAGS += -DPERF -Ofast -march=native
endif

define compile
	$(CXX) -MM -MT $@ $< -MF $@.d
	$(CXX) $(WFLAGS) $(CXXFLAGS) -o $@ $< $(1)
endef

$(TARGET): main.cc pch.hh.gch tags
	$(call compile,$(LDFLAGS))

pch.hh.gch: pch.hh Makefile
	$(call compile,)

tags: $(wildcard *.hh) $(wildcard *.cc)
	ctags --language-force=C++ *.hh *.cc

-include $(TARGET).d pch.hh.gch.d

clean:
	rm -f $(TARGET) *.d *.gch log tags
