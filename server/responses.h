#pragma once

#include <openssl/pem.h>
#include <vector>

#include "../common/constants.h"

#include "storage.h"


/// Respond to an ALL command by generating a list of all the usernames in the
/// Auth table and returning them, one per line.
///
/// @param sd      The socket onto which the result should be written
/// @param storage The Storage object, which contains the auth table
/// @param u       The user name associated with the request
/// @param p       The password associated with the request
/// @param req     The unencrypted contents of the request
///
/// @return false, to indicate that the server shouldn't stop
bool handle_all(int sd, Storage *storage,
                const std::string &u, const std::string &p,
                const std::vector<uint8_t> &req);

/// Respond to a SET command by putting the provided data into the Auth table
///
/// @param sd      The socket onto which the result should be written
/// @param storage The Storage object, which contains the auth table
/// @param u       The user name associated with the request
/// @param p       The password associated with the request
/// @param req     The unencrypted contents of the request
///
/// @return false, to indicate that the server shouldn't stop
bool handle_set(int sd, Storage *storage,
                const std::string &u, const std::string &p,
                const std::vector<uint8_t> &req);

/// Respond to a GET command by getting the data for a user
///
/// @param sd      The socket onto which the result should be written
/// @param storage The Storage object, which contains the auth table
/// @param u       The user name associated with the request
/// @param p       The password associated with the request
/// @param req     The unencrypted contents of the request
///
/// @return false, to indicate that the server shouldn't stop
bool handle_get(int sd, Storage *storage,
                const std::string &u, const std::string &p,
                const std::vector<uint8_t> &req);

/// Respond to a REG command by trying to add a new user
///
/// @param sd      The socket onto which the result should be written
/// @param storage The Storage object, which contains the auth table
/// @param u       The user name associated with the request
/// @param p       The password associated with the request
/// @param req     The unencrypted contents of the request
///
/// @return false, to indicate that the server shouldn't stop
bool handle_reg(int sd, Storage *storage,
                const std::string &u, const std::string &p,
                const std::vector<uint8_t> &req);

/// Respond to a BYE command by returning false, but only if the user
/// authenticates
///
/// @param sd      The socket onto which the result should be written
/// @param storage The Storage object, which contains the auth table
/// @param u       The user name associated with the request
/// @param p       The password associated with the request
/// @param req     The unencrypted contents of the request
///
/// @return true, to indicate that the server should stop, or false on an error
bool handle_bye(int sd, Storage *storage,
                const std::string &u, const std::string &p,
                const std::vector<uint8_t> &req);

/// Respond to a SAV command by persisting the file, but only if the user
/// authenticates
///
/// @param sd      The socket onto which the result should be written
/// @param storage The Storage object, which contains the auth table
/// @param u       The user name associated with the request
/// @param p       The password associated with the request
/// @param req     The unencrypted contents of the request
///
/// @return false, to indicate that the server shouldn't stop
bool handle_sav(int sd, Storage *storage,
                const std::string &u, const std::string &p,
                const std::vector<uint8_t> &req);