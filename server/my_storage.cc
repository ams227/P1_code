#include <cassert>
#include <cstdio>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

#include "../common/constants.h"
#include "../common/contextmanager.h"
#include "../common/err.h"

#include "authtableentry.h"
#include "map.h"
#include "map_factories.h"
#include "storage.h"

using namespace std;

/// MyStorage is the student implementation of the Storage class
class MyStorage : public Storage {
  /// The map of authentication information, indexed by username
  Map<string, AuthTableEntry> *auth_table;

  /// The name of the file from which the Storage object was loaded, and to
  /// which we persist the Storage object when save() is invoked
  const string filename;

public:
  /// Construct an empty object and specify the file from which it should be
  /// loaded.  To avoid exceptions and errors in the constructor, the act of
  /// loading data is separate from construction.
  ///
  /// @param fname   The name of the file to use for persistence
  /// @param buckets The number of buckets in the hash table
  MyStorage(const std::string &fname, size_t buckets)
      : auth_table(authtable_factory(buckets)), filename(fname) {}

  /// Destructor for the storage object.
  virtual ~MyStorage() {
    delete auth_table;
  }

  /// Authenticate a user
  ///
  /// @param user The name of the user who made the request
  /// @param pass The password for the user, used to authenticate
  ///
  /// @return A result tuple, as described in storage.h
  result_t auth(const string &user, const string &pass) {
    assert(user.length() > 0);
    assert(pass.length() > 0);
    bool authenticated = false;
    auth_table->do_with_readonly(user, [&](const AuthTableEntry &entry) { //checks the password
      if (entry.password == pass) {
        authenticated = true;
      }
    });
    if (authenticated) { //return OK for sucess
      return {true, RES_OK, {}};
    }
    return {false, RES_ERR_LOGIN, {}}; //else there is an error if password fails
  }

  /// Create a new entry in the Auth table.  If the user already exists, return
  /// an error.  Otherwise, save an entry with the username, password, and a zero-byte content.
  ///
  /// @param user The user name to register
  /// @param pass The password to associate with that user name
  ///
  /// @return A result tuple, as described in storage.h
  virtual result_t add_user(const string &user, const string &pass) {
    assert(user.length() > 0);
    assert(pass.length() > 0);
    AuthTableEntry person; // entry of person
    person.username = user; // username
    person.password = pass; // password
    person.content = {};
    if (auth_table->insert(user, person)) // inserts the person into the whole data
      return {true, RES_OK, {}};
    return {false, RES_ERR_USER_EXISTS, {}};
  }

  /// Set the data bytes for a user, but do so if and only if the password
  /// matches
  ///
  /// @param user    The name of the user whose content is being set
  /// @param pass    The password for the user, used to authenticate
  /// @param content The data to set for this user
  ///
  /// @return A result tuple, as described in storage.h
  virtual result_t set_user_data(const string &user, const string &pass,
                                 const vector<uint8_t> &content) {
    assert(user.length() > 0);
    assert(pass.length() > 0);
    result_t a = auth(user, pass); // verifies the pair
    if (!a.succeeded) return a; // if it doesnt work return a
    auth_table->do_with(user, [&](AuthTableEntry &e) { // looking up the data and updating their records
      e.content = content;
    });
    return {true, RES_OK, {}}; // reports the success
  }

  /// Return a copy of the user data for a user, but do so only if the password
  /// matches
  ///
  /// @param user The name of the user who made the request
  /// @param pass The password for the user, used to authenticate
  /// @param who  The name of the user whose content is being fetched
  ///
  /// @return A result tuple, as described in storage.h.  Note that "no data" is
  ///         an error
  virtual result_t get_user_data(const string &user, const string &pass,
                                 const string &who) {
    assert(user.length() > 0);
    assert(pass.length() > 0);
    assert(who.length() > 0);
    result_t auth_res = auth(user, pass); //autenticate user
    if (!auth_res.succeeded) { // This means that it faled
      return {false, RES_ERR_LOGIN, {}};
    }
    vector<uint8_t> content;
    bool found = auth_table->do_with_readonly( // reading the data of the user using read
        who, [&](const AuthTableEntry &entry) {
          content = entry.content;
        });
    if (!found) { // if the user us not found
      return {false, RES_ERR_NO_USER, {}};
    }
    if (content.empty()) { //empty data
      return {false, RES_ERR_NO_DATA, {}};
    }
    return {true, RES_OK, content}; // returns success if the user data is found
  }

  /// Return a newline-delimited string containing all of the usernames in the
  /// auth table
  ///
  /// @param user The name of the user who made the request
  /// @param pass The password for the user, used to authenticate
  ///
  /// @return A result tuple, as described in storage.h
  virtual result_t get_all_users(const string &user, const string &pass) {
    assert(user.length() > 0);
    assert(pass.length() > 0);
    result_t auth_res = auth(user, pass); //authenticate the user
    if (!auth_res.succeeded) { // The authentication failed
      return {false, RES_ERR_LOGIN, {}}; // returns an error logging in
    }
    string users;
    auth_table->do_all_readonly( // apply funtion to all entries
        [&](const string username, const AuthTableEntry &) { // this part is looking at all the users
          users += username + "\n";
        });
    vector<uint8_t> data(users.begin(), users.end()); // This part is looking at all the users
    return {true, RES_OK, data}; // returns data
  }

  /// Write the entire Storage object to the file specified by this.filename. To
  /// ensure durability, Storage must be persisted in two steps.  First, it must
  /// be written to a temporary file (this.filename.tmp).  Then the temporary
  /// file can be renamed to replace the older version of the Storage object.
  ///
  /// @return A result tuple, as described in storage.h
  virtual result_t save_file() {
    string temp_filename = filename + ".tmp"; // temp file name
    FILE *storage_file = fopen(temp_filename.c_str(), "wb"); // open write binary
    if (storage_file == nullptr) { // no file error
      return {false, RES_ERR_SERVER, {}};
    }
    auth_table->do_all_readonly([&](const string &, const AuthTableEntry &entry) { // go through all entries
      fwrite(AUTHENTRY.data(), 1, AUTHENTRY.size(), storage_file); // write header
      uint32_t username_len = entry.username.size(); // username length
      uint32_t password_len = entry.password.size(); // password length
      uint32_t profile_len = entry.content.size(); // profile length
      fwrite(&username_len, sizeof(uint32_t), 1, storage_file); // write username length
      fwrite(&password_len, sizeof(uint32_t), 1, storage_file); // write password length
      fwrite(&profile_len, sizeof(uint32_t), 1, storage_file); // write profile length
      fwrite(entry.username.data(), 1, username_len, storage_file); // write username
      fwrite(entry.password.data(), 1, password_len, storage_file); // write password
      if (profile_len > 0) { // only write if not empty
        fwrite(entry.content.data(), 1, profile_len, storage_file); // write profile
      }
      uint32_t entry_size = AUTHENTRY.size() + (3 * sizeof(uint32_t)) + username_len + password_len + profile_len; // total size
      uint32_t padding = (4 - (entry_size % 4)) % 4; // make divisible by 4
      for (uint32_t i = 0; i < padding; ++i) { // write padding zeros
        uint8_t zero = 0;
        fwrite(&zero, 1, 1, storage_file);
      }
    });
    fclose(storage_file); // close file
    rename(temp_filename.c_str(), filename.c_str()); // rename temp to real
    return {true, RES_OK, {}};
  }

  /// Populate the Storage object by loading this.filename.  Note that load()
  /// begins by clearing the maps, so that when the call is complete, exactly
  /// and only the contents of the file are in the Storage object.
  ///
  /// @return A result tuple, as described in storage.h.  Note that a
  ///         non-existent file is not an error.
  virtual result_t load_file() {
    FILE *storage_file = fopen(filename.c_str(), "rb"); // open read binary
    if (storage_file == nullptr) { // missing file not an error
      return {true, "File not found: " + filename, {}};
    }
    auth_table->clear(); // clear table before loading
    while (true) { // read until EOF
      char header[4]; // header buffer
      size_t header_read = fread(header, 1, 4, storage_file); // read header
      if (header_read == 0) { // nothing left
        break;
      }
      if (header_read != 4) { // partial header
        fclose(storage_file);
        return {false, RES_ERR_SERVER, {}};
      }
      if (memcmp(header, AUTHENTRY.data(), 4) != 0) { // wrong header
        fclose(storage_file);
        return {false, RES_ERR_SERVER, {}};
      }
      uint32_t username_len; // username length
      uint32_t password_len; // password length
      uint32_t profile_len; // profile length
      if (fread(&username_len, sizeof(uint32_t), 1, storage_file) != 1 || // read username length
          fread(&password_len, sizeof(uint32_t), 1, storage_file) != 1 || // read password length
          fread(&profile_len, sizeof(uint32_t), 1, storage_file) != 1) { // read profile length
        fclose(storage_file);
        return {false, RES_ERR_SERVER, {}};
      }
      if (username_len > LEN_UNAME || // check username length
          password_len > LEN_PASSWORD || // check password length
          profile_len > LEN_PROFILE_FILE) { // check profile length
        fclose(storage_file);
        return {false, RES_ERR_SERVER, {}};
      }
      string username(username_len, '\0'); // make username string
      if (username_len > 0 && fread(&username[0], 1, username_len, storage_file) != username_len) { // read username
        fclose(storage_file);
        return {false, RES_ERR_SERVER, {}};
      }
      string password(password_len, '\0'); // make password string
      if (password_len > 0 && fread(&password[0], 1, password_len, storage_file) != password_len) { // read password
        fclose(storage_file);
        return {false, RES_ERR_SERVER, {}};
      }
      vector<uint8_t> profile(profile_len); // make profile vector
      if (profile_len > 0 && fread(profile.data(), 1, profile_len, storage_file) != profile_len) { // read profile
        fclose(storage_file);
        return {false, RES_ERR_SERVER, {}};
      }
      size_t entry_size = AUTHENTRY.size() + (3 * sizeof(uint32_t)) + username_len + password_len + profile_len; // total size
      size_t padding = (4 - (entry_size % 4)) % 4; // calculate padding
      for (size_t i = 0; i < padding; ++i) { // skip padding
        int c = fgetc(storage_file);
        if (c == EOF) { // file ended early
          fclose(storage_file);
          return {false, RES_ERR_SERVER, {}};
        }
        if (c != 0) { // padding not zero
          fclose(storage_file);
          return {false, RES_ERR_SERVER, {}};
        }
      }
      AuthTableEntry entry; // build entry
      entry.username = username; // set username
      entry.password = password; // set password
      entry.content = profile; // set profile
      if (!auth_table->insert(username, entry)) { // insert into table
        fclose(storage_file);
        return {false, RES_ERR_SERVER, {}};
      }
    }
    fclose(storage_file); // close file
    return {true, "Loaded: " + filename, {}};
  }
};

/// Create an empty Storage object and specify the file from which it should be
/// loaded.  To avoid exceptions and errors in the constructor, the act of
/// loading data is separate from construction.
///
/// @param fname   The name of the file to use for persistence
/// @param buckets The number of buckets in the hash table
Storage *storage_factory(const std::string &fname, size_t buckets) {
  return new MyStorage(fname, buckets);
}