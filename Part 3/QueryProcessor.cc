/*
* Copyright ©2024 Hal Perkins.  All rights reserved.  Permission is
* hereby granted to students registered for University of Washington
* CSE 333 for use solely during Winter Quarter 2024 for purposes of
* the course.  No other use, copying, distribution, or modification
* is permitted without prior written consent. Copyrights for
* third-party components of this work must be honored.  Instructors
* interested in reusing these course materials should contact the
* author.
*/

#include "./QueryProcessor.h"

#include <iostream>
#include <algorithm>
#include <list>
#include <string>
#include <vector>
#include <map>  // to test out using map

extern "C" {
  #include "./libhw1/CSE333.h"
}

using std::list;
using std::sort;
using std::string;
using std::vector;
using std::map;

namespace hw3 {

  QueryProcessor::QueryProcessor(
    const list<string>& index_list, bool validate) {
    // Stash away a copy of the index list.
    index_list_ = index_list;
    array_len_ = index_list_.size();
    Verify333(array_len_ > 0);

    // Create the arrays of DocTableReader*'s. and IndexTableReader*'s.
    dtr_array_ = new DocTableReader* [array_len_];
    itr_array_ = new IndexTableReader* [array_len_];

    // Populate the arrays with heap-allocated DocTableReader and
    // IndexTableReader object instances.
    list<string>::const_iterator idx_iterator = index_list_.begin();
    for (int i = 0; i < array_len_; i++) {
      FileIndexReader fir(*idx_iterator, validate);
      dtr_array_[i] = fir.NewDocTableReader();
      itr_array_[i] = fir.NewIndexTableReader();
      idx_iterator++;
    }
  }

  QueryProcessor::~QueryProcessor() {
    // Delete the heap-allocated DocTableReader and IndexTableReader
    // object instances.
    Verify333(dtr_array_ != nullptr);
    Verify333(itr_array_ != nullptr);
    for (int i = 0; i < array_len_; i++) {
      delete dtr_array_[i];
      delete itr_array_[i];
    }

    // Delete the arrays of DocTableReader*'s and IndexTableReader*'s.
    delete[] dtr_array_;
    delete[] itr_array_;
    dtr_array_ = nullptr;
    itr_array_ = nullptr;
  }

  // This structure is used to store a index-file-specific query result.
  typedef struct {
    DocID_t doc_id;  // The document ID within the index file.
    int     rank;    // The rank of the result so far.
  } IdxQueryResult;

  vector<QueryProcessor::QueryResult>
  QueryProcessor::ProcessQuery(const vector<string>& query) const {
    Verify333(query.size() > 0);
    // STEP 1.
    // (the only step in this file)
  map<string, int> scores;

  for (const auto& w : query) {
      map<string, int> curr_scores;

      for (int idx = 0; idx < array_len_; ++idx) {
          auto* res = itr_array_[idx] -> LookupWord(w);
          if (res == nullptr) {
              continue;
          }

          auto docs = res -> GetDocIDList();
          for (const auto& doc : docs) {
              string name;
              bool found = dtr_array_[idx] -> LookupDocID(doc.doc_id, &name);
              if (found) {
                  curr_scores[name] += doc.num_positions;
              }
          }

          delete res;
      }

      if (scores.empty()) {
          scores = std::move(curr_scores);
      } else {
          for (auto it = scores.begin(); it != scores.end();) {
              if (curr_scores.count(it -> first) == 0) {
                  it = scores.erase(it);
              } else {
                  it -> second += curr_scores[it -> first];
                  ++it;
              }
          }
      }
  }

  vector<QueryResult> final_result;

  for (const auto& pair : scores) {
      QueryResult output{pair.first, pair.second};
      final_result.emplace_back(output);
  }

    // Sort the final results.
    sort(final_result.begin(), final_result.end());
    return final_result;
  }

}  // namespace hw3
