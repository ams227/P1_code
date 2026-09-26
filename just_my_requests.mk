# Build a client and server using only the student's requests.cc

# The executables will have the suffix .exe
EXESUFFIX = exe

# Names for building the client:
CLIENT_MAIN     = client
CLIENT_CXX      = requests
CLIENT_COMMON   = 
CLIENT_PROVIDED = client err file net

# Names for building the server
SERVER_MAIN     = server
SERVER_CXX      = 
SERVER_COMMON   = 
SERVER_PROVIDED = server responses parsing sequentialmap_factories \
                    err file net my_storage

# Pull in the common build rules
include common.mk
