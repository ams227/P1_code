# Assignment #1: Creating an On-Line Service

The purpose of this assignment is to demonstrate good C/C++ programming skills and using system calls to create an on-line service

## Do This First

Immediately after pulling `p1` from Github, you should 
rename the folder solutions_docker to solutions if you are using docker, or 
rename solutions_sunlab to solutions if you are using sunlab.
Then type `chmod +x solutions/*.exe`.  This command only needs to be run once.  
It will make sure that certain files from the assignment have their executable bit set.

## Assignment Overview

During the warm-up assignment, you reviewed many concepts:

- How to load a file from disk as a byte stream
- How to send byte streams to disk as new files
- How to send and receive bytes over the network

In this assignment, you will start putting those ideas together to build an
on-line service.

As with any service, you will make two separate programs: a client and a server.
The server will manage a "directory" of users.  Each user can store up to 1MB of data as their "profile file".

The focus of this assignment is on writing the code to implement a given communication protocol between the server and the client, specifically:

- Users should not be able to make unauthorized accesses.  You will need to
  implement a registration mechanism by which users can get an account on the
  system. A user will have permission to read any other user's profile file, and
  to get a list of all user names.  A user will also have permission to change
  her/his own profile file.
- Communication between the client and server must follow a specific protocol for sending
  data between the client and the server.
- You will need to implement a rudimentary form of persistence where the client 
  can instruct the server to save all of its data to disk.  This is not a great long-term 
  strategy, but it will be sufficient for now... and it makes it easier to test your code
  and make sure it is behaving correctly.

## The Communication Protocol

The main source of complexity and difficulty in this assignment relates to
understanding the communication protocol and implementing it correctly.  In the
following subsections you can find a discussion of the basic connection
strategy and the message request/response formats.

### Connections

Every interaction between a client and server begins with the client initiating
a connection to the server and sending a request message.  The server sends a
response message, and then closes the connection.  To keep things simple, there
is no re-use of connections.  The server should send a response to every message
it receives.

### Types of Requests and Responses

In the discussion below, the `.` operator indicates concatenation (for example,
`"A"."B" == "AB"`).  `@0` is the binary representation of the number 0 in 32
bits. `len(x)` indicates the length of `x`, as a 32-bit binary value. 
`pad0(x,y)` indicates that `x` should be padded to a length of `y` bytes by appending (binary) zeroes.

#### Request: `REG_`

- Purpose:  Attempt to register a new user
- Arguments:
  - @u -- The requested user name (max length 32 bytes)
  - @p -- The requested password (max length 32 bytes)
- Request Format: `"REG_".len(@u).len(@p).@0.@u.@p`
- Response Formats:
  - `OK__`  -- The request succeeded
  - `@err`  -- The server was unable to process the request
- Errors (`@err`):
  - `ERR_USER_EXISTS` -- The requested user already exists
  - `ERR_REQUEST_FMT` -- The arguments to the request were not valid

#### Request `EXIT`

- Purpose:  Force the server to stop.  This can only be requested by a valid
  user.  Note that a real server should never let an arbitrary client cause it
  to stop.  This is a convenience request to help with debugging, testing, and
  grading.
- Arguments:
  - @u -- The user's name (max length 32 bytes)
  - @p -- The user's password (max length 32 bytes)
- Request Format: `"EXIT".len(@u).len(@p).@0.@u.@p`
- Response Formats:
  - `OK__`  -- The request succeeded
  - `@err`  -- The server was unable to process the request
- Errors (`@err`):
  - `ERR_LOGIN`       -- `@u` is not a valid user
  - `ERR_LOGIN`       -- `@p` is not the correct password for user `@u`
  - `ERR_REQUEST_FMT` -- The arguments to the request were not valid

#### Request `SAVE`

- Purpose: Force the server to send all its data to disk.  This can only be
  requested by a valid user.  Note that a real server should never let an
  arbitrary client cause it to save data.  This is a convenience request 
  to help with debugging, testing, and grading.
- Arguments:
  - @u -- The user's name (max length 32 bytes)
  - @p -- The user's password (max length 32 bytes)
- Request Format: `"SAVE".len(@u).len(@p).@0.@u.@p`
- Response Formats:
  - `OK__`  -- The request succeeded
  - `@err`  -- The server was unable to process the request
- Errors (`@err`):
  - `ERR_LOGIN`       -- `@u` is not a valid user
  - `ERR_LOGIN`       -- `@p` is not the correct password for user `@u`
  - `ERR_REQUEST_FMT` -- The arguments to the request were not valid

#### Request `SETP`

- Purpose: Update a user's "profile file".  Note that a user can only change
  their own profile file.
- Arguments:
  - @u -- The user's name (max length 32 bytes)
  - @p -- The user's password (max length 32 bytes)
  - @f -- The bytes of data representing the new "profile file" (max length 1MB)
- Temporaries:
  - `@b = len(@f).@f`
- Request Format: `"SETP".len(@u).len(@p).len(@b).@u.@p.@b`
- Response Formats:
  - `OK__`  -- The request succeeded
  - `@err`  -- The server was unable to process the request
- Errors (`@err`):
  - `ERR_LOGIN`       -- `@u` is not a valid user
  - `ERR_LOGIN`       -- `@p` is not the correct password for user `@u`
  - `ERR_REQUEST_FMT` -- The arguments to the request were not valid

#### Request `GETP`

- Purpose: Get a user's "profile file".  Note that a user can request the
  profile file for any user.
- Arguments
  - @u -- The user's name (max length 32 bytes)
  - @p -- The user's password (max length 32 bytes)
  - @w -- The name of the user whose file is being requested (max length 32
    bytes)
- Temporaries:
  - `@b = len(@w).@w`
  -  @k -- The profile file for user @w, if @w is a valid user
- Request Format: `"GETP".len(@u).len(@p).len(@b).@u.@p.@b`
- Response Formats:
  - `OK__.len(@k).@k` -- The request succeeded
  - `@err`  -- The server was unable to process the request
- Errors (`@err`):
  - `ERR_LOGIN`       -- `@u` is not a valid user
  - `ERR_LOGIN`       -- `@p` is not the correct password for user `@u`
  - `ERR_NO_USER`     -- `@w` is not a valid user
  - `ERR_NO_DATA`     -- `@w` has a null profile file
  - `ERR_REQUEST_FMT` -- The arguments to the request were not valid

#### Request `ALL_`

- Purpose: Get a newline-delimited list of the names of all the users.  The list
  will not be sorted, and it will not have a trailing newline.
- Arguments
  - @u -- The user's name (max length 32 bytes)
  - @p -- The user's password (max length 32 bytes)
- Temporaries:
  - @l -- The newline-delimited list of user names
- Request Format: `"ALL_".len(@u).len(@p).@0.@u.@p`
- Response Formats:
  - `OK__.len(@l).@l` -- The request succeeded
  - `@err`  -- The server was unable to process the request
- Errors (`@err`):
  - `ERR_LOGIN`       -- `@u` is not a valid user
  - `ERR_LOGIN`       -- `@p` is not the correct password for user `@u`
  - `ERR_REQUEST_FMT` -- The arguments to the request were not valid

## Authentication

Your server is required to implement a very simple password-based authentication.
In this assignment, we will use a non secure authentication by storing the passwords 
as clear text in the server storage. In the next programming assignment, we will
implement a more robust authentication by saving passwords in a hashed form.

## The File Format

Your server will have a single hash table that needs to be persisted to a file:
the "authentication table".  To persist the table to a file, each entry needs to
be written to the file.  The format of each entry as follows:

- Header -- `AUTH` (4 bytes)
- Lengths -- `len(@u).len(@pass).len(@profile)`
- Contents -- `@u.@pass.@profile`
- Padding -- Up to three `0` characters, to ensure that the entry's length is
  divisible by 4.

Notes:

- All data should be written as binary
- If the `@profile` is null, then the length should still be written as `0`.

## Getting Started

Your git repository contains a significant portion of the code for the final
solution to this assignment.  The `p1/common/` folder has variants of many of
the useful functions that we created for assignment 0.  The `p1/client/` folder
has code that is specific to the client, and the `p1/server/` folder has code
that is specific to the server.  You should be able to type `make` from the
`p1/` folder to build all of the code, and you should be able to run the basic
tests for the assignment by typing the following from the `p1` folder:

```bash
python3 scripts/p1.py
```

Of course, none of the tests will pass yet.  It will print some errors, and then
the script will crash.

I do not recommend starting by writing code.  Instead, start by making sure you
understand the communication protocol and storage format.  Try to understand how
commands flow through the client, to the server, and how responses flow back.
You can use the solution executables to help in this endeavor.

## Tips and Reminders

The code that we provide makes use of a number of C++ features.  You should be
sure you understand the way we use `std::vector`.  In particular, you should
know the difference between a declaration like `std::vector<uint8_t> v(1024)`
and the two-instruction sequence `std::vector<uint8_t> v; v.reserve(1024)`.
Also, note that our use of `std::vector` can introduce unnecessary copying,
which should be optimized out in an advanced implementation.  On the other hand,
some copying is necessary. Our use of `const vector<uint8_t>&` parameters helps
to show you where copying needs to happen.

While the collections (`std::vector`, `std::list`, etc.) use the heap, if you
are using them well, you shouldn't have to directly interact with the heap very
much in this assignment.  The reference solution calls `new` only four times,
and two of those times are for managing command-line arguments.  There are five
calls to `delete`, but we provide three of them for you.  Still, you are
expected to ensure your program does not have memory leaks, and to use
`valgrind` to be sure.

As you read through the details of this assignment, keep in mind that subsequent
assignments will build upon this... so as you craft a solution, be sure to think
about how to make it maintainable.  C++ has many features that make it easy to
extend your code.  The `auto` keyword is one.  In-line initialization of arrays
is another.

**Start Early**.  Just reading the code and understanding what is happening
takes time.  If you start reading the assignment early, you'll give yourself
time to think about what is supposed to be happening, and that will help you to
figure out what you will need to do.

When it comes time to implement functions in the client and server, you will
probably want to proceed in the following order: REG, SAV, BYE, ALL, SET,
GET.  Note that the test script accepts optional `CLIENT`, `SERVER`, and
`VERBOSE` arguments.  The first two will switch to using the solution
executables for the client and server, respectively.  The `VERBOSE` argument
will cause the script to print more information about what it is doing, so that
you can issue the same server and client commands from separate screen sessions
in a single terminal.

As always, please be careful about not committing unnecessary files into your
repository (object, executable, and temporary files).

Your programs should never require keyboard input. In particular, the client
should get all its parameters from the command line.

## Grading

The `scripts` folder has four python scripts.  These scripts use specialized 
`Makefiles` that integrate some of the solution code with some of your code, 
so that you can get partial credit when certain portions of your code work.  
Note that these `Makefiles` are very strict: they will crash on any compiler warning.  
You should *not* turn off these warnings; they are there to help you.  
If your code does not compile with these scripts, you will not receive any credit.  
If you have partial solutions and you do not know how to coerce the code into being 
warning-free, you should ask the professor.

Passing the tests is one component of your assignment grade.
You will have a code review meeting with a grader who will ask you
to explain parts of your code and how you would expand some functions.

Please be sure to use your tools well.  For example, Visual Studio Code (and
emacs, and vim) have features that auto-format your code.  You should use these
features, so that your code is legible.  It is likely that auto-formatting will
help you spot bugs in your code.

Broadly speaking, there are three graded components:

- Are requests being built properly?
- Are parsing and responses written correctly?
- Is persistence implemented correctly?

Remember that the scripts can take VERBOSE, SERVER, and CLIENT arguments.
VERBOSE causes scripts to print output that will be useful when you try to run
the client and server manually.  SERVER and CLIENT cause the scripts to use the
reference solution for the server and client, respectively.  You should use
these to be sure that your implementations are correct.

## Notes About the Reference Solution

In the following subsections, you will find some details about the reference
solution.  Note that you should not change any files other than the six that
are listed below.

### `server/parsing.cc`

In the reference solution, the `parse_request()` function has about 100 lines of
code.  It employs some nice code-saving techniques, like using an array of
functions when dispatching to the right command:

```c++
  // Iterate through possible commands, pick the right one, run it
  vector<string> s = {REQ_REG, REQ_BYE, REQ_SAV, REQ_SET, REQ_GET, REQ_ALL};
  decltype(handle_reg) *cmds[] = {handle_reg, handle_bye, handle_sav,
                                  handle_set, handle_get, handle_all};
  for (size_t i = 0; i < s.size(); ++i)
    if (cmd == s[i])
      return cmds[i](sd, storage, aes_ctx, ablock);
```

### `client/requests.cc`

In the reference solution, the file is about 275 lines long.  My code also has 
this function, which is very useful for simplifying `client_get()` and `client_all()`:

```c++
/// If a buffer consists of RES_OK.bbbb.d+, where `.` means concatenation, bbbb
/// is a 4-byte binary integer and d+ is a string of characters, write the bytes
/// (d+) to a file
///
/// @param buf      The buffer holding a response
/// @param filename The name of the file to write
void send_result_to_file(const vector<uint8_t> &buf, const string &filename);
```

The reference solution also has two functions that handle assembling the
client's request.  You are not required to write helper functions, but you will
probably find that they make it easier to build the client requests. Here are
their declarations.  Each has less than 5 lines in its body.  The templates make
it easy to use the same functions with `std::vector` and `std::string`
arguments:

```c++
/// Add the size of a value to a vector as a 4-byte value
///
/// @param res  The vector to add to
/// @param t    The thing whose size should be added
///
/// @tparam T   The type of t
template <class T> void add_size(vector<uint8_t> &res, T t);

/// Add the contents of an iterable to a vector
///
/// @param res  The vector to add to
/// @param t    The thing to add
///
/// @tparam T   The type of t
template <class T> void add_it(vector<uint8_t> &res, T t);
```

The most important helper function is this one.  You'll probably want to
implement something similar:

```c++
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
                         const vector<uint8_t> &msg);
```

The reference implementation is about 80 lines.  You won't want to write it at
first... figure out how to do REG and SET, and then you'll know what needs to go
in it.  Note that the function will help with REG, BYE, SAV, SET, GET, and ALL.
For example, here's the body of `req_get()` when the helper functions are
done:

```c++
  vector<uint8_t> block;
  add_size(block, getname);
  add_it(block, getname);
  auto res = send_cmd(sd, REQ_GET, user, pass, block);
  send_result_to_file(res, getname + ".file.dat");
```

(Yes, just 5 lines... the hard work is in `send_cmd` and `send_result_to_file`).

### `server/responses.cc`

This file has one function for each of the requests that a client can make.
Each function receives data, validates it, sends a request to the
`Storage` object, and then transmits a response to the client.

You may want to write a few helper functions here, too, especially for
extracting the parts of a request.  The reference solution takes about 230 lines
of code to complete this file.  There is a fair bit of copy-and-paste from one
command to the next.

### `server/my_storage.cc`

In the reference solution, this file is about 330 lines (this count includes the
150 lines that are given in the handout).  `save()` and `load()` take the
most code by far, because file I/O is tedious in C++.  The other methods are
relatively straightforward.  You should not need to add any additional fields to
the `MyStorage` class.

### `server/sequentialmap.h`

In subsequent assignments, you will need to implement your own concurrent hash
map.  In this assignment, we use the `SequentialMap` template as a wrapper
around `std::vector`.  This may seem cumbersome, but it means that later on, your
code in `my_storage.cc` won't have to change: you will just replace the sequential
map with a concurrent hash map that has the same interface.

If you are not familiar with `std::vector`, `http://cppreference.com/` is an
excellent resource.  Google will usually send you directly to the correct page.
Note that some of the methods of `SequentialMap` won't be used in this
assignment.  If you figure them out now, then the second assignment will be
easier.  If you skip them, you will not lose points, but you will miss out on
the opportunity to be more prepared for the rest of the semester.

## Collaboration and Getting Help

You may work in teams of 2 for this assignment.  If you are working in a team, you will be prompted to create a team on Classroom50 when you accept the assignment.
I recommend naming your group using the following format: p1-[lehigh-username1]-[lehigh-username2] Working in a team means you should **pair program** for the entire assignment.  After all, your goal is to learn together, not to each learn half the material.

If you require help, you may seek it from any of the following sources:

- The professors and TAs, via office hours or Piazza
- The Internet, as long as you use the Internet as a read-only resource.

It is not appropriate to share code with past or current CSE 303 students,
unless you are sharing code with your teammate.

If you are familiar with `man` pages, please note that the easiest way to find a
man page is via Google.  For example, typing `man printf` will probably return
`https://linux.die.net/man/3/printf` as one of the first links.  It is fine to
use Google to find man pages.

StackOverflow is a wonderful tool for professional software engineers.  It is a
horrible place to ask for help as a student.  You should feel free to use
StackOverflow, but only as a *read only* resource.  In this class, you should
**never** need to ask a question on StackOverflow.

## Deadline

You should be done with this assignment before 11:59 PM on September 21st, 2026.
Please be sure to `git commit` and `git push` before that time, so that we can
promptly collect and grade your work.

There are many parts to this assignment, so you will probably want to `git push`
frequently. Please do not push object or executable files.