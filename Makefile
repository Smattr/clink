# FIXME: Beat libclang sufficiently into submission to replace this with CMake

SOURCES = CXXParser.cpp CXXParser.h Database.cpp Database.h FauxTerm.h \
  main.cpp Options.h Parser.h Symbol.h UI.h UICurses.cpp UICurses.h UILine.cpp \
  UILine.h util.h Vim.cpp Vim.h

CXX ?= c++
CXXFLAGS += -std=c++11 -g -W -Wall -Wextra -O3
LDFLAGS += -lclang -lsqlite3 -lreadline -lncurses

default: clink

clink: ${SOURCES}
	${CXX} ${CXXFLAGS} $(filter %.cpp,$^) -o $@ ${LDFLAGS}

clean:
	rm clink
