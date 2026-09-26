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
    auth_table->do_with_readonly(user, [&](const AuthTableEntry &entry) { //chekc password
      if (entry.password == pass) {
        authenticated = true;
      }
    });
    if (authenticated) { //return OK for sucess
      return {true, RES_OK, {}};
    }
    return {false, RES_ERR_LOGIN, {}}; //else error
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
    AuthTableEntry person;
    person.username = user;
    person.password = pass;
    person.content = {};
    if (auth_table->insert(user, person))
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
    result_t a = auth(user, pass);
    if (!a.succeeded) return a;
    auth_table->do_with(user, [&](AuthTableEntry &e) {
      e.content = content;
    });
    return {true, RES_OK, {}};
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

    if (!auth_res.succeeded) { //auth failed
      return {false, RES_ERR_LOGIN, {}};
    }

    vector<uint8_t> content;
    bool found = auth_table->do_with_readonly( //not change so readonly
        who, [&](const AuthTableEntry &entry) {
          content = entry.content;
        });

    if (!found) { //could not find user
      return {false, RES_ERR_NO_USER, {}};
    }

    if (content.empty()) { //empty data
      return {false, RES_ERR_NO_DATA, {}};
    }

    return {true, RES_OK, content}; //sucess - return user data
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
    result_t auth_res = auth(user, pass); //authenticate

    if (!auth_res.succeeded) { //auth fail
      return {false, RES_ERR_LOGIN, {}};
    }

    string users;
    auth_table->do_all_readonly( //apply funtion to all entries
        [&](const string username, const AuthTableEntry &) { //grab each user
          users += username;
          users += "\n";
        });

    vector<uint8_t> data(users.begin(), users.end()); //vector ification
    return {true, RES_OK, data};
  }

  

  /// Write the entire Storage object to the file specified by this.filename. To
  /// ensure durability, Storage must be persisted in two steps.  First, it must
  /// be written to a temporary file (this.filename.tmp).  Then the temporary
  /// file can be renamed to replace the older version of the Storage object.
  ///
  /// @return A result tuple, as described in storage.h
  virtual result_t save_file() {
    string temp_filename = filename + ".tmp";
    FILE *storage_file = fopen(temp_filename.c_str(), "wb");//open write binary - fopen needs c str

    if (storage_file == nullptr) { //no file error
      return {false, RES_ERR_SERVER, {}};
    }

    auth_table->do_all_readonly(
        [&](const string &, const AuthTableEntry &entry) { //function for all entries: (& since already contains username) get entry and do..
          fwrite(AUTHENTRY.data(), 1, AUTHENTRY.size(), storage_file); //write authentry bytes to storage (header)

          // get lengths
          uint32_t username_len = entry.username.size();
          uint32_t password_len = entry.password.size();
          uint32_t profile_len = entry.content.size();
          //erite lengths to storage next
          fwrite(&username_len, sizeof(uint32_t), 1, storage_file); //write bytes that are mem adress where length stored
          fwrite(&password_len, sizeof(uint32_t), 1, storage_file);
          fwrite(&profile_len, sizeof(uint32_t), 1, storage_file);

          //write contents
          fwrite(entry.username.data(), 1, username_len, storage_file); //.data for acrual chars in string
          fwrite(entry.password.data(), 1, password_len, storage_file);

          if (profile_len > 0) { //write profile, dont try and write zero bytes if null
            fwrite(entry.content.data(), 1, profile_len, storage_file);
          }

          uint32_t entry_size = AUTHENTRY.size() + ( 3 * sizeof(uint32_t)) + username_len + password_len + profile_len; //check bytes written 
          uint32_t padding = (4 - (entry_size % 4)) % 4; //make divisible by 4 -< how much padding is neeeded?

          for (uint32_t i = 0; i < padding; ++i) { //write needed padding
            uint8_t zero = 0;
            fwrite(&zero, 1, 1, storage_file);
          }
        });

    fclose(storage_file);
    rename(temp_filename.c_str(), filename.c_str()); //rename temp file to final file name

    return {true, RES_OK, {}};
  }

  /// Populate the Storage object by loading this.filename.  Note that load()
  /// begins by clearing the maps, so that when the call is complete, exactly
  /// and only the contents of the file are in the Storage object.
  ///
  /// @return A result tuple, as described in storage.h.  Note that a
  ///         non-existent file is not an error.
  virtual result_t load_file() {
    FILE *storage_file = fopen(filename.c_str(), "rb");

    if (storage_file == nullptr) { //missing file not error accoring to assignment so return anyway, no error
      return {true, "File not found: " + filename, {}};
    }

    auth_table->clear(); //clear current table before reading

    while (true) { //read all enttied til end of file
      char header[4];
      size_t header_read = fread(header, 1, 4, storage_file);

      if (header_read == 0) { //nothing left - EOF
        break;
      }

      if (header_read != 4) { //didnt read all 4 bytes: ended halfwhy through entry -> error
        fclose(storage_file);
        return {false, RES_ERR_SERVER, {}};
      }

      if (memcmp(header, AUTHENTRY.data(), 4) != 0) { //check if same as exoected format for header
        fclose(storage_file);
        return {false, RES_ERR_SERVER, {}};
      }

      //read lengths - make some variables to hold them
      uint32_t username_len;
      uint32_t password_len;
      uint32_t profile_len;

      if (fread(&username_len, sizeof(uint32_t), 1, storage_file) != 1 || //read 4 bytes into username_len
          fread(&password_len, sizeof(uint32_t), 1, storage_file) != 1 || //read 4 bytes ubti password_len
          fread(&profile_len, sizeof(uint32_t), 1, storage_file) != 1) { //read 4 bytes into profile_len
        fclose(storage_file);
        return {false, RES_ERR_SERVER, {}};
      }

      if (username_len > LEN_UNAME || //check lengths are expected
          password_len > LEN_PASSWORD ||
          profile_len > LEN_PROFILE_FILE) {
        fclose(storage_file);
        return {false, RES_ERR_SERVER, {}};
      }

      string username(username_len, '\0'); //make string of proper username size
      if (username_len > 0 && fread(&username[0], 1, username_len, storage_file) != username_len) { //fill th eprepped bytes
        fclose(storage_file);
        return {false, RES_ERR_SERVER, {}};
      }

      string password(password_len, '\0'); //make string for password
      if (password_len > 0 && fread(&password[0], 1, password_len, storage_file) != password_len) { //gte password
        fclose(storage_file);
        return {false, RES_ERR_SERVER, {}};
      }

      vector<uint8_t> profile(profile_len); //profile not text, so vector
      if (profile_len > 0 && fread(profile.data(), 1, profile_len, storage_file) != profile_len) { 
        fclose(storage_file);
        return {false, RES_ERR_SERVER, {}};
      }


      size_t entry_size = AUTHENTRY.size() + ( 3 * sizeof(uint32_t)) + username_len + password_len + profile_len; //calculate current entry size
      size_t padding = (4 - (entry_size % 4)) % 4; //calculate padding

      for (size_t i = 0; i < padding; ++i) {
        int c = fgetc(storage_file);

        if (c == EOF) { //file ended early
          fclose(storage_file);
          return {false, RES_ERR_SERVER, {}};
        }

        if (c != 0) { //padding isnt proper padding
          fclose(storage_file);
          return {false, RES_ERR_SERVER, {}};
        }
      }

      //reconstruct the auth table entry from read file
      AuthTableEntry entry;
      entry.username = username;
      entry.password = password;
      entry.content = profile;

      if (!auth_table->insert(username, entry)) { //insert with user as key and entrey as value - error if something breaks
        fclose(storage_file);
        return {false, RES_ERR_SERVER, {}};
      }
    }

    fclose(storage_file);

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
