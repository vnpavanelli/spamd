LIBS=-lboost_system -lpthread -lboost_regex
CXXFLAGS=-O2 -std=c++11

.PHONY: install all

all: spamd

spamd: spamd.o
	$(CXX) $(CXXFLAGS) $(MYFLAGS) $^ -o $@ ${LIBS}

install: spamd
	install -D spamd /opt/spamd/spamd

%.o: %.cpp  $(DEPDIR)/%.d
	$(CXX) $(CXXFLAGS) -c $< -o $@ 


