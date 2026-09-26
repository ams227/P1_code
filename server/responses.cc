#include <string>
#include <iostream>
#include <cassert>
#include <cstring>
#include "../common/constants.h"
#include "../common/net.h"
#include "responses.h"

using namespace std;
/// Add the size of a value to a vector as a 4-byte value
///
/// @param res  The vector to add to
/// @param t    The thing whose size should be added
///
/// @tparam T   The type of t
template <class T> void add_size(vector<uint8_t> &res, T t) {
  uint32_t size = t.size();           // length as a 32-bit int
  res.push_back(size & 0xFF);         // byte 0
  res.push_back((size >> 8) & 0xFF);  // byte 1
  res.push_back((size >> 16) & 0xFF); // byte 2
  res.push_back((size >> 24) & 0xFF); // byte 3
  }

/// Add the contents of an iterable to a vector
///
/// @param res  The vector to add to
/// @param t    The thing to add
///
/// @tparam T   The type of t
template <class T> void add_it(vector<uint8_t> &res, T t) {
  for (auto c : t) // for c in t you have to push_back or push each element onto res
    res.push_back(c);
}

/// Concatenate a string and a vector of content, in a format that can be sent
/// to the client as a single message.  Most often, this involves a message of
/// RES_OK and content that was returned from a hash table.
///
/// @param msg     A string message to send to the client
/// @param content A vector of content to send to the client
///
/// @return a vector with the correct concatenation of msg and content
vector<uint8_t> build_res(const string &msg, const vector<uint8_t> &content) {
  vector<uint8_t> res;
  add_it(res, msg); // adds msg to res
  add_size(res, content); // adds content to res
  add_it(res, content); // adds content to res
  return res;
}

/// Send a message format error
///
/// @param sd  The socket onto which the result should be written
///
/// @return false, to indicate that the server shouldn't stop
bool send_err_msg_format(int sd) {
  send_reliably(sd, RES_ERR_REQ_FMT);
  return false;
}

/// Extract a string from a vector
///
/// @param it     An iterator to the extraction point
/// @param count  The number of characters to extract
/// @return The extracted string
string extract_string(vector<uint8_t>::const_iterator &it,
                             size_t count) {
  string result(it, it + count); // Copy bytes into a string
  it += count;                   // Advance the iterator past the bytes just read
  return result;
}

/// Extract a size (uint32_t) from a vector
/// @param it An iterator to the extraction point
/// @return The extracted uint32_t
uint32_t extract_size(vector<uint8_t>::const_iterator &it) {
  uint32_t res = 0;
  memcpy(&res, &(*it), sizeof(uint32_t));
  it += sizeof(uint32_t);
  return res;
}

/// Extract a vector from a vector
/// @param it     An iterator to the extraction point
/// @param count  The number of bytes to extract
/// @return The extracted vector
vector<uint8_t> extract_vec(vector<uint8_t>::const_iterator &it,
                                   size_t count) {
  vector<uint8_t> result(it, it + count);
  it += count;
  return result;
}

/// Respond to an ALL command by generating a list of all the usernames in the
/// Auth table and returning them, one per line.
///
/// @param sd      The socket onto which the result should be written
/// @param storage The Storage object, which contains the auth table
/// @param u       The user name associated with the request
/// @param p       The password associated with the request
/// @param req     The contents of the request
///
/// @return false, to indicate that the server shouldn't stop
bool handle_all(int sd, Storage *storage,
                const std::string &u, const std::string &p,
                const vector<uint8_t> &req) {
  assert(sd);
  assert(storage);
  assert(u.length() > 0);
  assert(p.length() > 0);
  Storage::result_t r = storage->get_all_users(u, p);
  auto resp = build_res(r.msg, r.data);
  send_reliably(sd, resp);
  return false;
}

/// Respond to a SET command by putting the provided data into the Auth table
///
/// @param sd      The socket onto which the result should be written
/// @param storage The Storage object, which contains the auth table
/// @param u       The user name associated with the request
/// @param p       The password associated with the request
/// @param req     The contents of the request
///
/// @return false, to indicate that the server shouldn't stop
bool handle_set(int sd, Storage *storage,
                const std::string &u, const std::string &p,
                const vector<uint8_t> &req) {
  assert(sd);
  assert(storage);
  assert(u.length() > 0);
  assert(p.length() > 0);
  Storage::result_t r = storage->set_user_data(u, p, req); // Storage pointer points to the storage
  auto resp = build_res(r.msg, r.data); // This is the build_res helper being called
  send_reliably(sd, resp); // This writes the resp to the buffer socket using the send_reliably function in common
  return false;
}

/// Respond to a GET command by getting the data for a user
///
/// @param sd      The socket onto which the result should be written
/// @param storage The Storage object, which contains the auth table
/// @param u       The user name associated with the request
/// @param p       The password associated with the request
/// @param req     The contents of the request
///
/// @return false, to indicate that the server shouldn't stop
bool handle_get(int sd, Storage *storage,
                const std::string &u, const std::string &p,
                const vector<uint8_t> &req) {
  assert(sd);
  assert(storage);
  assert(u.length() > 0);
  assert(p.length() > 0);
  string w(req.begin(), req.end()); // gets the message
  Storage::result_t r = storage->get_user_data(u, p, w); // Storage pointer points to the storage
  auto resp = build_res(r.msg, r.data); // This is the build_res helper being called
  send_reliably(sd, resp); // This writes the resp to the buffer socket using the send_reliably function in common
  return false;
}

/// Respond to a REG command by trying to add a new user
///
/// @param sd      The socket onto which the result should be written
/// @param storage The Storage object, which contains the auth table
/// @param u       The user name associated with the request
/// @param p       The password associated with the request
/// @param req     The contents of the request
///
/// @return false, to indicate that the server shouldn't stop
bool handle_reg(int sd, Storage *storage,
                const std::string &u, const std::string &p,
                const vector<uint8_t> &req) {
  assert(sd);
  assert(storage);
  assert(u.length() > 0);
  assert(p.length() > 0);
  Storage::result_t r = storage->add_user(u, p);
  auto resp = build_res(r.msg, r.data);
  send_reliably(sd, resp);
  return false;
}

/// Respond to a BYE command by returning false, but only if the user
/// authenticates
///
/// @param sd      The socket onto which the result should be written
/// @param storage The Storage object, which contains the auth table
/// @param u       The user name associated with the request
/// @param p       The password associated with the request
/// @param req     The contents of the request
///
/// @return true, to indicate that the server should stop, or false on an error
bool handle_bye(int sd, Storage *storage,
                const std::string &u, const std::string &p,
                const vector<uint8_t> &req) {
  assert(sd);
  assert(storage);
  assert(u.length() > 0);
  assert(p.length() > 0);
  Storage::result_t r = storage->auth(u, p);
  auto resp = build_res(r.msg, r.data);
  send_reliably(sd, resp);
  return r.succeeded;
}

/// Respond to a SAV command by persisting the file, but only if the user
/// authenticates
///
/// @param sd      The socket onto which the result should be written
/// @param storage The Storage object, which contains the auth table
/// @param u       The user name associated with the request
/// @param p       The password associated with the request
/// @param req     The contents of the request
///
/// @return false, to indicate that the server shouldn't stop
bool handle_sav(int sd, Storage *storage,
                const std::string &u, const std::string &p,
                const vector<uint8_t> &req) {
  assert(sd);
  assert(storage);
  assert(u.length() > 0);
  assert(p.length() > 0);
  Storage::result_t r = storage->save_file();
  auto resp = build_res(r.msg, r.data);
  send_reliably(sd, resp);
  return false;
}
