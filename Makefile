.PHONY: clean all print-vars showIR
.SILENT: FORCE

FILE := tests/if_return
PASSES := -hello

# ENTRYPOINT

all: print-vars ${FILE}.final.bc

showIR: all
	vim -O ${FILE}.ll ${FILE}.mem2reg.ll ${FILE}.final.ll

diffIR: all
	diff -y -W 175 ${FILE}.mem2reg.ll ${FILE}.final.ll | less

print-vars:
	# ####################################################
	# FILE = ${FILE} -> tests/${FILE}.mila
	# PASSES = ${PASSES}
	# ####################################################


# MY_PASSES

${FILE}.final.bc: ${FILE}.mem2reg.bc passes/build/libMyPasses.so FORCE
	opt -load=passes/build/libMyPasses.so ${PASSES} -o ${FILE}.final.bc ${FILE}.mem2reg.bc
	llvm-dis ${FILE}.final.bc

passes/build/libMyPasses.so: passes/build/Makefile passes/src/MyPasses.cc
	cd passes/build && make

passes/build/Makefile: passes/build passes/CMakeLists.txt
	cd passes/build && cmake ..

passes/build:
	mkdir passes/build


# MEM2REG

${FILE}.mem2reg.bc: ${FILE}.bc 
	opt -mem2reg -o ${FILE}.mem2reg.bc ${FILE}.bc 
	llvm-dis ${FILE}.mem2reg.bc


# FRONTEND

${FILE}.bc: build/mila+ ${FILE}.mila
	build/mila+ ${OPT} --emit ${FILE}.bc ${FILE}.mila
	llvm-dis ${FILE}.bc

build/mila+: build/Makefile src/*
	cd build && make

build/Makefile: build CMakeLists.txt
	cd build && cmake .. 

build:
	mkdir build

# CLEAN


clean:
	-rm tests/*.bc
	-rm tests/*.ll
	-rm build/mila+
	-rm passes/build/libMyPasses.so
	-rm -rf build
	-rm -rf passes/build

FORCE:
	# ALWAYS REBUILD
