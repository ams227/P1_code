/// This file defines the constants used throughout the program.  For details
/// about the message and file formats that use these constants, please see the
/// README file for this assignment

#pragma once

#include <string>

////////////////////////////////////////////////////////////////
// Below are the definitions needed for Assignment #1
////////////////////////////////////////////////////////////////

/// Maximum length of a user name
const int LEN_UNAME = 32;

/// Maximum length of a user's actual password
const int LEN_PASSWORD = 32;

/// Maximum length of a user's profile file
const int LEN_PROFILE_FILE = 1048576;

/// Request the creation of a new user
const std::string REQ_REG = "REG_";

/// Force the server to stop
const std::string REQ_BYE = "EXIT";

/// Force the server to send all its data to disk
const std::string REQ_SAV = "SAVE";

/// Allow a user to set their profile content
const std::string REQ_SET = "SETP";

/// Allow a user to request a user's profile content
const std::string REQ_GET = "GETP";

/// Get all user names
const std::string REQ_ALL = "ALL_";

/// Response code to indicate that the command was successful
const std::string RES_OK = "OK__";

/// Response code to indicate that the registered user already exists
const std::string RES_ERR_USER_EXISTS = "ERR_USER_EXISTS";

/// Response code to indicate that the client gave a bad username or password
const std::string RES_ERR_LOGIN = "ERR_LOGIN";

/// Response code to indicate that the client request was improperly formatted
const std::string RES_ERR_REQ_FMT = "ERR_REQ_FMT";

/// Response code to indicate that there is no data to send back
const std::string RES_ERR_NO_DATA = "ERR_NO_DATA";

/// Response code to indicate that the user being looked up is invalid
const std::string RES_ERR_NO_USER = "ERR_NO_USER";

/// Response code to indicate that the requested command doesn't exist
const std::string RES_ERR_INV_CMD = "ERR_INVALID_COMMAND";

/// Response code to indicate that the client didn't get as much data as
/// expected
const std::string RES_ERR_XMIT = "ERR_XMIT";

/// Response code to indicate that the server had an internal error, 
/// such as a bad read from a file
const std::string RES_ERR_SERVER = "ERR_SERVER";

/// Response code to indicate that something has not been implemented
const std::string RES_ERR_UNIMPLEMENTED = "ERR_UNIMPLEMENTED";

/// A unique 4-byte code to use as a prefix each time an AuthTable Entry is
/// written to disk.
const std::string AUTHENTRY = "AUTH";