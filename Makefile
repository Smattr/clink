# FIXME: Beat libclang sufficiently into submission to replace this with CMake

SOURCES = CXXParser.cpp CXXParser.h Database.cpp Database.h main.cpp Parser.h \
  Symbol.h UI.h UILine.cpp UILine.h

CXX ?= c++
CXXFLAGS += -std=c++11 -g -W -Wall -Wextra -O3
LDFLAGS += -lclang -lsqlite3 -lreadline

default: clink

clink: ${SOURCES}
	${CXX} ${CXXFLAGS} $(filter %.cpp,$^) -o $@ ${LDFLAGS}

clean:
	rm clink
