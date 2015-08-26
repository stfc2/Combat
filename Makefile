APP=./bin/moves_combat

SF=	main.o defines.o db_core.o \
	combat_prepare.o combat_process.o combat_finish.o

CC=gcc
CXX=g++
LINKER=g++
COPTS=-std=c++0x -O3 -march=k8
LINKOPTS=-L/usr/lib64/mysql -lmysqlclient -lz

all: $(APP)

$(APP): $(SF)
	$(LINKER) $(COPTS) $(OBJS) $^ $(LINKOPTS) -o $@

clean:
	rm -f $(SF) $(APP)

%.o: %.c
	$(CC) -c $(COPTS) $< -o $@

%.o: %.cc
	$(CXX) -c $(COPTS) $< -o $@

%.o: %.cpp
	$(CXX) -c $(COPTS) $< -o $@
