CC = mpiicc
CPP = mpiicpc
INC1 = ../../boost/
INC2 = ../src_common/
INC3 = ../src_common/minisat2-2_Jan2013/
INCDIRS = -I${INC1} -I${INC2} -I${INC3}
CFLAGS = -O3 -std=c99 -lz -lm -Wall -Wmissing-prototypes -Wstrict-prototypes -Wsign-compare -D NDEBUG -D _MPI
CPPFLAGS = -O3 ${INCDIRS} -D __STDC_LIMIT_MACROS -D __STDC_FORMAT_MACROS -D _MPI

pd-sat: pd-sat.o dminisat.o dminisat_solver.o mpi_base.o mpi_solver.o mpi_predicter.o addit_func.o minisat22_wrapper.o Solver.o SimpSolver.o System.o Options.o
	${CPP} ${CPPFLAGS} pd-sat.o dminisat.o dminisat_solver.o mpi_base.o mpi_solver.o mpi_predicter.o addit_func.o minisat22_wrapper.o Solver.o SimpSolver.o System.o Options.o -o pd-sat
	
Options.o: ../src_common/minisat2-2_Jan2013/minisat/utils/Options.cc
	${CPP} ${CPPFLAGS} ../src_common/minisat2-2_Jan2013/minisat/utils/Options.cc -c
	
System.o: ../src_common/minisat2-2_Jan2013/minisat/utils/System.cc
	${CPP} ${CPPFLAGS} ../src_common/minisat2-2_Jan2013/minisat/utils/System.cc -c

SimpSolver.o: ../src_common/minisat2-2_Jan2013/minisat/simp/SimpSolver.cc
	${CPP} ${CPPFLAGS} ../src_common/minisat2-2_Jan2013/minisat/simp/SimpSolver.cc -c

Solver.o: ../src_common/minisat2-2_Jan2013/minisat/core/Solver.cc
	${CPP} ${CPPFLAGS} ../src_common/minisat2-2_Jan2013/minisat/core/Solver.cc -c
	
minisat22_wrapper.o: ../src_common/minisat22_wrapper.cpp
	${CPP} ${CPPFLAGS} ../src_common/minisat22_wrapper.cpp -c

addit_func.o: ../src_common/addit_func.cpp
	${CPP} ${CPPFLAGS} ../src_common/addit_func.cpp -c
	
dminisat_solver.o: ../src_common/dminisat_solver.c
	${CC} ${CFLAGS} ../src_common/dminisat_solver.c -c
	
dminisat.o: ../src_common/dminisat.c 
	${CC} ${CFLAGS} ../src_common/dminisat.c -c

mpi_predicter.o: mpi_predicter.cpp 
	${CPP} ${CPPFLAGS} mpi_predicter.cpp -c	

mpi_solver.o: mpi_solver.cpp 
	${CPP} ${CPPFLAGS} mpi_solver.cpp -c
	
mpi_base.o: mpi_base.cpp 
	${CPP} ${CPPFLAGS} mpi_base.cpp -c
	
pd-sat.o: pd-sat.cpp
	${CPP} ${CPPFLAGS} pd-sat.cpp -c
	
clean:
	rm -rf *.o
	rm pd-sat
	clear