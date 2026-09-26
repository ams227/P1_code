#include <cstring>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include "../common/constants.h"
#include "../common/contextmanager.h"
#include "../common/err.h"
#include "../common/net.h"

#include "parsing.h"
#include "responses.h"

using namespace std;

/// Extract a string from a vector
///
/// @param it     An iterator to the extraction point
/// @param count  The number of characters to extract
/// @return The string
string extract_string(vector<uint8_t>::iterator &it, size_t count) {
  string result(it, it + count);   // copy count bytes into a string
  it += count;                     // advance the iterator
  return result;
}


/// Extract a size (uint32_t) from a vector
/// @param it An iterator to the extraction point
/// @return The uint32_t extracted
uint32_t extract_size(vector<uint8_t>::iterator &it) {
  uint32_t res = 0;
  memcpy(&res, &(*it), sizeof(uint32_t)); //copy memory block the same size as a 32 bit unsigned integer from locaton of iterator into res 
  it += sizeof(uint32_t); //increment
  return res;
}

/// When a new client connection is accepted, this code will run to figure out
/// what the client is requesting, and to dispatch to the right function for
/// satisfying the request.
///
/// @param sd      The socket on which communication with the client takes place
/// @param storage The Storage object with which clients interact
///
/// @return true if the server should halt immediately, false otherwise
bool parse_request(int sd, Storage *storage) {
  vector<uint8_t> request = reliable_get_to_eof(sd); // This gets the bytes that the client sent
  if (request.size() < 16) return false; // You need at least 16 bytes for this
  auto it = request.begin(); // Iterating through the buffer
  string cmd(it, it + 4); // This reads the command in 4 byte commands
  it = it + 4;
  if (request.end() - it < 12) return false; // Need 12 more bytes for the ulen plen and blen
  uint32_t u_len = extract_size(it); // The U len stated above with needing teh full 16 bytes (username)
  uint32_t p_len = extract_size(it); // The P len stated above with needing teh full 16 bytes (password)
  uint32_t b_len = extract_size(it); // The B len stated above with needing teh full 16 bytes (bytes)
  size_t remaining = request.end() - it; // Calculates the remaining 
  if (u_len + p_len + b_len != remaining) return false;
  string u = extract_string(it, u_len); // Extracts the username
  string p = extract_string(it, p_len); // Extract the password
  vector<uint8_t> data(it, it + b_len); // This part extracts
  it += b_len; // it = it + b_len (Length of the binary data)
  if (cmd == REQ_REG) return handle_reg(sd, storage, u, p, data); // This registers the new user
  if (cmd == REQ_BYE) return handle_bye(sd, storage, u, p, data); // This logs off or disconnects the user
  if (cmd == REQ_SAV) return handle_sav(sd, storage, u, p, data); // This saves the data
  if (cmd == REQ_SET) return handle_set(sd, storage, u, p, data); // This sets a value in the storage
  if (cmd == REQ_GET) return handle_get(sd, storage, u, p, data); // Get a value
  if (cmd == REQ_ALL) return handle_all(sd, storage, u, p, data); // Get all the values
  return false;
}
