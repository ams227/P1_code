# Build a client and server from the reference solution, but use the student's
# server/storage.cc and server/sequentialmap.h.

# The executables will have the suffix p1.storage.exe.
EXESUFFIX = p1.storage.exe

# Names for building the client:
CLIENT_MAIN     = client
CLIENT_CXX      = 
CLIENT_COMMON   = 
CLIENT_PROVIDED = client err file net requests

# Names for building the server:
SERVER_MAIN     = server
SERVER_CXX      = my_storage sequentialmap_factories
SERVER_COMMON   = 
SERVER_PROVIDED = server responses parsing err file net

# All warnings should be treated as errors
CXXEXTRA = -Werror

# Pull in the common build rules
include common.mk
