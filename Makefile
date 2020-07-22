#
# 'make clean'       removes all .o and executable files
# 'make'/'make all'  build all executable files
# 'make aws'         run the aws
# 'make serverA'     run the serverA
# 'make serverB'     run the serverB
# 'make serverC'     run the serverC
# 'make aws_'        build executable file aws
# 'make serverA_'    build executable file serverA
# 'make serverB_'    build executable file serverB
# 'make serverC_'    build executable file serverC
# 'make client_'     build executable file client
#

# define the C compiler to use
CXX = g++

# define any compile-time flags
CXXFLAGS = -Wall -g -std=c++11

.PHONY: clean aws serverA serverB serverC

all:	aws_ serverA_ serverB_ serverC_ client_
		@echo all executable files are made
aws:    
		@./aws

serverA:    
		@./serverA

serverB:    
		@./serverB

serverC:    
		@./serverC

aws_:	aws.o
		$(CXX) $(CXXFLAGS) -o aws aws.o

serverA_:serverA.o
		 $(CXX) $(CXXFLAGS) -o serverA serverA.o

serverB_:serverB.o
		 $(CXX) $(CXXFLAGS) -o serverB serverB.o

serverC_:serverC.o
		 $(CXX) $(CXXFLAGS) -o serverC serverC.o

client_: client.o
		 $(CXX) $(CXXFLAGS) -o client client.o

.c.o:
		 $(CXX) $(CXXFLAGS) -c $< -o $@

clean:
		 $(RM) *.o *~ aws serverA serverB serverC client
