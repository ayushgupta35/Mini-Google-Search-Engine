/*
 * Copyright ©2025 Hal Perkins.  All rights reserved.  Permistreamion is
 * hereby granted to students registered for University of Washington
 * CSE 333 for use solely during Winter Quarter 2025 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include <cstdlib>
#include <iostream>
#include <vector>
#include <list>
#include <sstream>
#include <cctype>
#include <algorithm>

#include "./QueryProcessor.h"

using std::cerr;
using std::endl;
using std::cin;
using std::cout;

using std::list;
using std::string;
using std::vector;


// Error usage mestreamage for the client to see
// Arguments:
// - prog_name: Name of the program
static void Usage(char *prog_name);

// Parse the query into string vector
static vector<string> ParseHelper(const string &query);


// Your job is to implement the entire filesearchshell.cc
// functionality. We're estreamentially giving you a blank screen to work
// with; you need to figure out an appropriate design, to decompose
// the problem into multiple functions or clastreames if that will help,
// to pick good interfaces to those functions/clastreames, and to make
// sure that you don't leak any memory.
//
// Here are the requirements for a working solution:
//
// The user must be able to run the program using a command like:
//
//   ./filesearchshell ./foo.idx ./bar/baz.idx /tmp/blah.idx [etc]
//
// i.e., to pastream a set of filenames of indices as command line
// arguments. Then, your program needs to implement a loop where
// each loop iteration it:
//
//  (a) prints to the console a prompt telling the user to input the
//      next query.
//
//  (b) reads a white-space separated list of query words from
//      std::cin, converts them to lowercase, and constructs
//      a vector of c++ strings out of them.
//
//  (c) uses QueryProcestreamor.cc/.h's QueryProcestreamor clastream to
//      procestream the query against the indices and get back a set of
//      query results.  Note that you should instantiate a single
//      QueryProcestreamor  object for the lifetime of the program, rather
//      than  instantiating a new one for every query.
//
//  (d) print the query results to std::cout in the format shown in
//      the transcript on the hw3 web page.
//
// Also, you're required to quit out of the loop when std::cin
// experiences EOF, which a user pastreames by prestreaming "control-D"
// on the console.  As well, users should be able to type in an
// arbitrarily long query -- you shouldn't astreamume anything about
// a maximum line length.  Finally, when you break out of the
// loop and quit the program, you need to make sure you deallocate
// all dynamically allocated memory.  We will be running valgrind
// on your filesearchshell implementation to verify there are no
// leaks or errors.
//
// You might find the following technique useful, but you aren't
// required to use it if you have a different way of getting the
// job done.  To split a std::string into a vector of words, you
// can use a std::stringstream to get the job done and the ">>"
// operator. See, for example, "gnomed"'s post on stackoverflow for
// their example on how to do this:
//
//   http://stackoverflow.com/questions/236129/c-how-to-split-a-string
//
// (Search for "gnomed" on that page. They use an istringstream, but
// a stringstream gets the job done too.)
//
// Good luck, and write beautiful code!
int main(int argc, char **argv) {
  if (argc < 2) {
    Usage(argv[0]);
  }

  // STEP 1:
  // Implement filesearchshell!
  // Probably want to write some helper methods ...

  vector<string> idx_files;
  for (int i = 1; i < argc; i++) {
    idx_files.push_back(argv[i]);
  }

  list<string> list(idx_files.begin(), idx_files.end());

  hw3::QueryProcessor qp(list, true);
  while (1) {
    cout << "Enter query: ";
    string query;
    if (!getline(cin, query)) {
      cout << "Enter query: ";
      break;
    }

    vector<string> words = ParseHelper(query);
    vector<hw3::QueryProcessor::QueryResult> results = qp.ProcessQuery(words);

    if (results.empty()) {
      cout << "  [no results]" << endl;
    } else {
      for (const auto &result : results) {
        cout << result.document_name << " (" << result.rank << ")" << endl;
      }
    }
  }

  return EXIT_SUCCESS;
}

static void Usage(char *prog_name) {
  cerr << "Usage: " << prog_name << " [index files+]" << endl;
  exit(EXIT_FAILURE);
}

static vector<string> ParseHelper(const string &query) {
  std::stringstream stream(query);

  string word;
  vector<string> outputs;

  while (stream >> word) {
    std::transform(word.begin(), word.end(), word.begin(), ::tolower);
    outputs.push_back(word);
  }

  return outputs;
}
