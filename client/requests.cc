#include <cassert>
#include <cstring>
#include <iostream>
#include <openssl/rand.h>
#include <sys/socket.h> // Added
#include <vector>

#include "../common/constants.h"
#include "../common/contextmanager.h"
#include "../common/file.h"
#include "../common/net.h"

#include "requests.h"

using namespace std;

/// Use this instead of constructing empty vector args to send_cmd
vector<uint8_t> empty_vec;

/// Add the size of a value to a vector as a little-endian 4-byte value 
///
/// @param res  The vector to add to
/// @param t    The thing whose size should be added
///
/// @tparam T   The type of t
template <class T> void add_size(vector<uint8_t> &res, T t) {
  uint32_t size = t.size(); // Gets the container
  res.push_back(size & 0xFF); // gets size
  res.push_back((size >> 8) & 0xFF); // takes the bytes and shifts
  res.push_back((size >> 16) & 0xFF); // take the next byte and shift
  res.push_back((size >> 24) & 0xFF); // take the next byte and shift
}

/// Add the contents of an iterable to a vector
///
/// @param res  The vector to add to
/// @param t    The thing to add
///
/// @tparam T   The type of t
template <class T> void add_it(vector<uint8_t> &res, T t) {
  for (char c : t) { // adding using iteration like python -> for c in t
    res.push_back(c);
  }
}

/// If a buffer consists of RES_OK.bbbb.d+, where `.` means concatenation, bbbb
/// is an 4-byte binary integer and d+ is a string of characters, write the
/// bytes (d+) to a file
///
/// @param buf      The buffer holding a response
/// @param filename The name of the file to write
void send_result_to_file(const vector<uint8_t> &buf, const string &filename) {
  if (buf.size() < 8 || memcmp(buf.data(), RES_OK.data(), 4) != 0) return; // the status must be OK and there needs to be 8 bytes or more
  uint32_t data_len = 0; // This is the payload length
  memcpy(&data_len, buf.data() + 4, sizeof(data_len)); // The memory reads the 4 bytes after its status
  if (data_len != buf.size() - 8) return; // The data has to match the payload size exactly
  write_file(filename, vector<uint8_t>(buf.begin() + 8, buf.end()), 0); // It writes the payload to the file
}

/// Send a message to the server, using the common format for messages,
/// then take the response from the server and return it.
///
/// @param sd       The open socket descriptor for communicating with the server
/// @param cmd      The command that is being sent
/// @param user     The username for the request
/// @param password The password for the request
/// @param msg      The contents
///
/// @return a vector with the result, or an empty vector on error
vector<uint8_t> send_cmd(int sd, const string &cmd,
                         const string &user, const string &password,
                         const vector<uint8_t> &msg) {
  assert(sd);
  assert(cmd.length() > 0);
  assert(user.length() > 0);
  assert(password.length() > 0);

  vector<uint8_t> req;
  add_it(req, cmd);
  add_size(req, user);
  add_size(req, password);
  add_size(req, msg);
  add_it(req, user);
  add_it(req, password);
  add_it(req, msg);

  if (!send_reliably(sd, req)) return {};
  shutdown(sd, SHUT_WR); // So the server doesn't run forever
  return reliable_get_to_eof(sd);
}

/// req_reg() sends the REG command to register a new user
///
/// @param sd      The open socket descriptor for communicating with the server
/// @param user    The name of the user doing the request
/// @param pass    The password of the user doing the request
void req_reg(int sd, const string &user, const string &pass,
             const string &) {
  assert(sd); // Sees if SD is 0 or not
  assert(user.length() > 0); // Ensures that user length is greater than 0
  assert(pass.length() > 0); // Ensure that password length is greater than 0
  auto res = send_cmd(sd, REQ_REG, user, pass, empty_vec); // calles the reg with user pass and the empty one and returns to res
  if (res.size() >= 4) { // Only looks at the response if the byte is greater than 4
    if (memcmp(res.data(), RES_OK.data(), 4) == 0) {
      // Status is text but rest is binary
      cout << string(res.begin(), res.begin() + 4) << endl;
    } else {
      // Eveyrthing is  ASCII
      string s(res.begin(), res.end());
      size_t end = 0;
      while (end < s.size() && s[end] >= 32 && s[end] < 127) end++;
      s = s.substr(0, end);
      cout << s << endl;
    }
  }
}

/// req_bye() writes a request for the server to exit.
///
/// @param sd      The open socket descriptor for communicating with the server
/// @param user    The name of the user doing the request
/// @param pass    The password of the user doing the request
void req_bye(int sd, const string &user, const string &pass,
             const string &) {
  assert(sd); // Sees if SD is 0 or not
  assert(user.length() > 0); // Ensures that user length is greater than 0
  assert(pass.length() > 0); // Ensure that password length is greater than 0
  auto res = send_cmd(sd, REQ_BYE, user, pass, empty_vec); // Sends bye command and has the user password and empty over to server and tells the server to shut down
  if (res.size() >= 4) { // Only looks at the response if the byte is greater than 4
    if (memcmp(res.data(), RES_OK.data(), 4) == 0) {
      // Status is text but rest is binary
      cout << string(res.begin(), res.begin() + 4) << endl;
    } else {
      // Eveyrthing is  ASCII
      string s(res.begin(), res.end());
      size_t end = 0;
      while (end < s.size() && s[end] >= 32 && s[end] < 127) end++;
      s = s.substr(0, end);
      cout << s << endl;
    }
  }
}

/// req_sav() writes a request for the server to save its contents
///
/// @param sd      The open socket descriptor for communicating with the server
/// @param user    The name of the user doing the request
/// @param pass    The password of the user doing the request
void req_sav(int sd, const string &user, const string &pass,
             const string &) {
  assert(sd); // Sees if SD is 0 or not
  assert(user.length() > 0); // Ensures that user length is greater than 0
  assert(pass.length() > 0); // Ensure that password length is greater than 0
  auto res = send_cmd(sd, REQ_SAV, user, pass, empty_vec); // gets the user password and saves to socket retunr to empty_vec
  if (res.size() >= 4) { // Only looks at the response if the byte is greater than 4
    if (memcmp(res.data(), RES_OK.data(), 4) == 0) {
      // Status is text but rest is binary
      cout << string(res.begin(), res.begin() + 4) << endl;
    } else {
      // Eveyrthing is  ASCII
      string s(res.begin(), res.end());
      size_t end = 0;
      while (end < s.size() && s[end] >= 32 && s[end] < 127) end++;
      s = s.substr(0, end);
      cout << s << endl;
    }
  }
}

/// req_set() sends the SET command to set the content for a user
///
/// @param sd      The open socket descriptor for communicating with the server
/// @param user    The name of the user doing the request
/// @param pass    The password of the user doing the request
/// @param setfile The file whose contents should be sent
void req_set(int sd, const string &user, const string &pass, const string &setfile) {
  assert(sd); // Sees if SD is 0 or not
  assert(user.length() > 0); // Ensures that user length is greater than 0
  assert(pass.length() > 0); // Ensure that password length is greater than 0
  assert(setfile.length() > 0);
  vector<uint8_t> contents = load_entire_file(setfile); // gets the content from the files
  auto res = send_cmd(sd, REQ_SET, user, pass, contents); // Sets command with User Password and File to send to the server and it returns as res
  if (res.size() >= 4) { // Only looks at the response if the byte is greater than 4
    if (memcmp(res.data(), RES_OK.data(), 4) == 0) {
      // Status is text but rest is binary
      cout << string(res.begin(), res.begin() + 4) << endl;
    } else {
      // Eveyrthing is  ASCII
      string s(res.begin(), res.end());
      size_t end = 0;
      while (end < s.size() && s[end] >= 32 && s[end] < 127) end++;
      s = s.substr(0, end);
      cout << s << endl;
    }
  }
}

/// req_get() requests the content associated with a user, and saves it to a
/// file called <user>.file.dat.
///
/// @param sd      The open socket descriptor for communicating with the server
/// @param user    The name of the user doing the request
/// @param pass    The password of the user doing the request
/// @param getname The name of the user whose content should be fetched
void req_get(int sd, const string &user, const string &pass,
             const string &getname) {
  assert(sd); // Sees if SD is 0 or not
  assert(user.length() > 0); // Ensures that user length is greater than 0
  assert(pass.length() > 0); // Ensure that password length is greater than 0
  assert(getname.length() > 0);
  vector<uint8_t> block;
  add_it(block, getname);
  auto res = send_cmd(sd, REQ_GET, user, pass, block);
  send_result_to_file(res, getname + ".file.dat"); // Send the result to the file
  if (res.size() >= 4) { // Only looks at the response if the byte is greater than 4
    if (memcmp(res.data(), RES_OK.data(), 4) == 0) {
      // Status is text but rest is binary
      cout << string(res.begin(), res.begin() + 4) << endl;
    } else {
      // Eveyrthing is  ASCII
      string s(res.begin(), res.end());
      size_t end = 0;
      while (end < s.size() && s[end] >= 32 && s[end] < 127) end++;
      s = s.substr(0, end);
      cout << s << endl;
    }
  }
}

/// req_all() sends the ALL command to get a listing of all users, formatted
/// as text with one entry per line.
///
/// @param sd      The open socket descriptor for communicating with the server
/// @param user    The name of the user doing the request
/// @param pass    The password of the user doing the request
/// @param allfile The file where the result should go
void req_all(int sd, const string &user, const string &pass,
             const string &allfile) {
  assert(sd); // Sees if SD is 0 or not
  assert(user.length() > 0); // Ensures that user length is greater than 0
  assert(pass.length() > 0); // Ensure that password length is greater than 0
  assert(allfile.length() > 0);
  auto res = send_cmd(sd, REQ_ALL, user, pass, empty_vec);
  send_result_to_file(res, allfile); // Send the result to file
  if (res.size() >= 4) { // Only looks at the response if the byte is greater than 4
    if (memcmp(res.data(), RES_OK.data(), 4) == 0) {
      // Status is text but rest is binary
      cout << string(res.begin(), res.begin() + 4) << endl;
    } else {
      // Eveyrthing is  ASCII
      string s(res.begin(), res.end());
      size_t end = 0;
      while (end < s.size() && s[end] >= 32 && s[end] < 127) end++;
      s = s.substr(0, end);
      cout << s << endl;
    }
  }
}