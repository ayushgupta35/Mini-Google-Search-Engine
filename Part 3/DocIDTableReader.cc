/*
* Copyright ©2025 Hal Perkins.  All rights reserved.  Permission is
* hereby granted to students registered for University of Washington
* CSE 333 for use solely during Winter Quarter 2025 for purposes of
* the course.  No other use, copying, distribution, or modification
* is permitted without prior written consent. Copyrights for
* third-party components of this work must be honored.  Instructors
* interested in reusing these course materials should contact the
* author.
*/

#include "./DocIDTableReader.h"

#include <list>      // for std::list

extern "C" {
#include "libhw1/CSE333.h"
}
#include "./LayoutStructs.h"

using std::list;

namespace hw3 {

  // The constructor for DocIDTableReader calls the constructor
  // of HashTableReader(), its base class. The base class takes
  // care of taking ownership of f and using it to extract and
  // cache the number of buckets within the table.
  DocIDTableReader::DocIDTableReader(FILE *f, IndexFileOffset_t offset)
  : HashTableReader(f, offset) { }

  bool DocIDTableReader::LookupDocID(
      const DocID_t &doc_id, list<DocPositionOffset_t> *const ret_val) const {
  // Use the base class's "LookupElementPositions" function to
  // walk through the docIDtable and get back a list of offsets
  // to elements in the bucket for this docID.
  auto elements = LookupElementPositions(doc_id);

  // If the list of elements is empty, we're done.
  if (elements.empty())
    return false;

  // Iterate through all of elements, looking for our docID.
  for (IndexFileOffset_t &curr : elements) {
    // STEP 1.
    // Slurp the next docid out of the current element.

    DocIDElementHeader curr_header;
    Verify333(fseek(file_, curr, SEEK_SET) == 0);
    Verify333(fread(&curr_header.doc_id,
        sizeof(curr_header.doc_id), 1, file_) == 1);
    curr_header.doc_id = ntohll(curr_header.doc_id);

    // Is it a match?
    if (curr_header.doc_id == doc_id) {
      // STEP 2.
      // Yes!  Extract the positions themselves, appending to
      // std::list<DocPositionOffset_t>.  Be sure to push in the right
      // order, adding to the end of the list as you extract
      // successive positions.

      Verify333(fread(&curr_header.num_positions,
        sizeof(curr_header.num_positions), 1, file_) == 1);
      curr_header.num_positions = ntohl(curr_header.num_positions);

      std::list<DocPositionOffset_t> l;
      DocPositionOffset_t next;
      for (DocPositionOffset_t i = 0; i < curr_header.num_positions; i++) {
        Verify333(fread(&next, sizeof(next), 1, file_) == 1);
        next = ntohl(next);
        l.push_back(next);
      }

      // STEP 3.
      // Return the positions list through the output parameter,
      // and return true.

      *ret_val = l;
      return true;
    }
  }

  // We failed to find a matching docID, so return false.
  return false;
  }

  list<DocIDElementHeader> DocIDTableReader::GetDocIDList() const {
  // This will be our returned list of docIDs within this table.
  list<DocIDElementHeader> doc_id_list;

  // Go through *all* of the buckets of this hashtable, extracting
  // out the docids in each element and the number of word positions
  // for the each docid.
  for (int i = 0; i < header_.num_buckets; i++) {
    // STEP 4.
    // Seek to the next BucketRecord.  The "offset_" member
    // variable stores the offset of this docid table within
    // the index file.

    IndexFileOffset_t base_offset = offset_ + 4;
    Verify333(fseek(file_, base_offset + (i * 8), SEEK_SET) == 0);

    // STEP 5.
    // Read in the chain length and bucket position fields from
    // the bucket_rec.
    BucketRecord bucket_rec;

    Verify333(fread(&bucket_rec.chain_num_elements,
      sizeof(bucket_rec.chain_num_elements), 1, file_) == 1);
    Verify333(fread(&bucket_rec.position,
      sizeof(bucket_rec.position), 1, file_) == 1);
    bucket_rec.position = ntohl(bucket_rec.position);
    bucket_rec.chain_num_elements =
      ntohl(bucket_rec.chain_num_elements);

    // Sweep through the next bucket, iterating through each
    // chain element in the bucket.
    for (int j = 0; j < bucket_rec.chain_num_elements; j++) {
      // Seek to chain element's position field in the bucket header.
      Verify333(fseek(file_, bucket_rec.position
        + j*sizeof(ElementPositionRecord), SEEK_SET) == 0);

      // STEP 6.
      // Read the next element position from the bucket header.
      // and seek to the element itself.

      ElementPositionRecord element_pos;
      Verify333(fread(&element_pos.position,
        sizeof(element_pos.position), 1, file_) == 1);

      element_pos.position = ntohl(element_pos.position);
      Verify333(fseek(file_, element_pos.position, SEEK_SET) == 0);

      // STEP 7.
      // Read in the docid and number of positions from the element.
      DocIDElementHeader element;
      Verify333(fread(&element.doc_id,
        sizeof(element.doc_id), 1, file_) == 1);
      Verify333(fread(&element.num_positions,
        sizeof(element.num_positions), 1, file_) == 1);
      element.doc_id = ntohll(element.doc_id);
      element.num_positions = ntohl(element.num_positions);

      // Append it to our result list.
      doc_id_list.push_back(element);
    }
  }

  // Done!  Return the result list.
  return doc_id_list;
  }

}  // namespace hw3
