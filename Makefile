#avoid messing around with / when variables are not set
GUROBI_HOME ?= .

CC			= g++
CFLAGS		= -g -m64 -Wall -std=c++17 -I $(GUROBI_HOME)/include -pthread
LDFLAGS		= -L $(GUROBI_HOME)/lib -lgurobi_c++ -lm -lfftw3 -lstdc++fs -lpthread

CD_DIR 			= CD

# Use `PANDORA=1 make` for gurobi8 build.
ifeq ($(strip $(PANDORA)),)
	BUILDDIR	= build9
	LDFLAGS 	+= -lgurobi81
else
	CC 			= g++-7
	CFLAGS 		+= -DPANDORA
	LDFLAGS 	+= -lgurobi81
	BUILDDIR	= build8
endif

VISULDFLAGS		= -lm
SIMULDFLAGS		= -lm
OPTLIB_BUILDDIR	= $(BUILDDIR)/optlib
SRCDIR		= src
OPTLIB_SRCDIR	= $(SRCDIR)/optlib
SRC			= $(wildcard $(OPTLIB_SRCDIR)/*.cpp)
OBJ			= $(SRC:$(OPTLIB_SRCDIR)/%.cpp=$(OPTLIB_BUILDDIR)/%.o)
DEP			= $(OBJ:%.o=%.d)
BIN			= opt
VISUBIN			= visu
SIMUBIN			= simu
RETARGET_BIN			= config_retarget
CONFIG_BEAUTIFIERBIN			= config_beautify
BINMAIN		= $(SRCDIR)/main.cpp
VISUBINMAIN		= $(SRCDIR)/visu.cpp
SIMUBINMAIN		= $(SRCDIR)/simu.cpp
RETARGET_BINMAIN		= $(SRCDIR)/config_retarget.cpp
CONFIG_BEAUTIFIERBINMAIN		= $(SRCDIR)/config_beautify.cpp
VISU_OBJ	= $(OPTLIB_BUILDDIR)/visualizer.o $(OPTLIB_BUILDDIR)/coordinates.o $(OPTLIB_BUILDDIR)/csv_tools.o $(OPTLIB_BUILDDIR)/simplexoid.o $(OPTLIB_BUILDDIR)/config.o $(OPTLIB_BUILDDIR)/linear_interpolation.o $(OPTLIB_BUILDDIR)/saft.o $(OPTLIB_BUILDDIR)/reader.o $(OPTLIB_BUILDDIR)/exception.o $(OPTLIB_BUILDDIR)/compute_all_cells.o $(OPTLIB_BUILDDIR)/stop_watch.o
SIMU_OBJ	= $(OPTLIB_BUILDDIR)/coordinates.o $(OPTLIB_BUILDDIR)/csv_tools.o $(OPTLIB_BUILDDIR)/config.o $(OPTLIB_BUILDDIR)/reader.o $(OPTLIB_BUILDDIR)/exception.o

TEST_DIR	= src/tests
TEST_BIN	= test_runner
TEST_SRC	= $(wildcard $(TEST_DIR)/*.h)
TEST_SRCGEN	= test_runner.cpp
TEST_OBJ = $(filter-out $(BUILDDIR)/main.o,$(OBJ))

.PHONY: build opt visu simu docs vtest dtest ddtest testbuild test beautify_config

build: opt visu simu beautify_config retarget_config

opt: $(BUILDDIR)/$(BIN)

visu: $(BUILDDIR)/$(VISUBIN)

simu: $(BUILDDIR)/$(SIMUBIN)

retarget_config: $(BUILDDIR)/$(RETARGET_BIN)

beautify_config: $(BUILDDIR)/$(CONFIG_BEAUTIFIERBIN)

docs:
	doxygen

vtest: build $(BUILDDIR)/$(TEST_BIN)
	valgrind ./$(BUILDDIR)/$(TEST_BIN)

dtest: build $(BUILDDIR)/$(TEST_BIN)
	gdb ./$(BUILDDIR)/$(TEST_BIN)

ddtest: build $(BUILDDIR)/$(TEST_BIN)
	gdb --ex r --ex bt ./$(BUILDDIR)/$(TEST_BIN)

testbuild: build $(BUILDDIR)/$(TEST_BIN)

test: build $(BUILDDIR)/$(TEST_BIN)
	./$(BUILDDIR)/$(TEST_BIN)

$(BUILDDIR)/$(TEST_BIN): $(BUILDDIR)/$(TEST_SRCGEN) $(TEST_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILDDIR)/$(TEST_SRCGEN): $(TEST_SRC)
	cxxtestgen --error-printer -o $(BUILDDIR)/$(TEST_SRCGEN) $^

run: build
	./$(BUILDDIR)/$(BIN)

all: build pdf examples


$(BUILDDIR)/$(BIN): $(OBJ) $(BINMAIN)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILDDIR)/$(SIMUBIN): $(SIMUBINMAIN) $(SIMU_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(SIMULDFLAGS)

$(BUILDDIR)/$(RETARGET_BIN): $(RETARGET_BINMAIN) $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILDDIR)/$(CONFIG_BEAUTIFIERBIN): $(CONFIG_BEAUTIFIERBINMAIN) $(SIMU_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(SIMULDFLAGS)

$(BUILDDIR)/$(VISUBIN): $(VISUBINMAIN) $(VISU_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(VISULDFLAGS)

-include $(DEP)
$(OPTLIB_BUILDDIR)/%.o: $(OPTLIB_SRCDIR)/%.cpp
	$(CC) $(CFLAGS) -MMD -o $@ -c $<

clean:
	rm -fr $(OPTLIB_BUILDDIR)/*.o $(BUILDDIR)/$(BIN) $(BUILDDIR)/$(TEST_SRCGEN) $(BUILDDIR)/$(TEST_BIN)
	mkdir -p $(BUILDDIR) $(OPTLIB_BUILDDIR)
