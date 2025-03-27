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

// Feature test macro for strtok_r (c.f., Linux Programming Interface p. 63)
#define _XOPEN_SOURCE 600

// Constant for our buffer size
#define BUFFER_SIZE 1024

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "libhw1/CSE333.h"
#include "./CrawlFileTree.h"
#include "./DocTable.h"
#include "./MemIndex.h"

//////////////////////////////////////////////////////////////////////////////
// Helper function declarations, constants, etc
static void Usage(void);
static void ProcessQueries(DocTable *dt, MemIndex *mi);
static int GetNextLine(FILE *f, char **ret_str);


//////////////////////////////////////////////////////////////////////////////
// Main
int main(int argc, char **argv) {
  if (argc != 2) {
    Usage();
  }

  // Implement searchshell!  We're giving you very few hints
  // on how to do it, so you'll need to figure out an appropriate
  // decomposition into functions as well as implementing the
  // functions.  There are several major tasks you need to build:
  //
  //  - Crawl from a directory provided by argv[1] to produce and index
  //  - Prompt the user for a query and read the query from stdin, in a loop
  //  - Split a query into words (check out strtok_r)
  //  - Process a query against the index and print out the results
  //
  // When searchshell detects end-of-file on stdin (cntrl-D from the
  // keyboard), searchshell should free all dynamically allocated
  // memory and any other allocated resources and then exit.
  //
  // Note that you should make sure the fomatting of your
  // searchshell output exactly matches our solution binaries
  // to get full points on this part.
  char *directory = argv[1];
  DocTable *doc_table = NULL;
  MemIndex *mem_index = NULL;

  printf("Indexing '%s'\n", directory);
  if (!CrawlFileTree(directory, &doc_table, &mem_index)) {
      fprintf(stderr, "Error: Unable to index directory.\n");
      return EXIT_FAILURE;
  }
  ProcessQueries(doc_table, mem_index);
  DocTable_Free(doc_table);
  MemIndex_Free(mem_index);

  return EXIT_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////
// Helper function definitions

static void Usage(void) {
  fprintf(stderr, "Usage: ./searchshell <docroot>\n");
  fprintf(stderr,
          "where <docroot> is an absolute or relative " \
          "path to a directory to build an index under.\n");
  exit(EXIT_FAILURE);
}

static void ProcessQueries(DocTable *dt, MemIndex *mi) {
  while (1==1) {
    char *query_buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));
    char **query_tokens = (char **)malloc(BUFFER_SIZE * sizeof(char *));
    if (!query_buffer || !query_tokens) {
        fprintf(stderr, "Memory allocation failed.\n");
        free(query_buffer);
        free(query_tokens);
        return;
    }

    if (GetNextLine(stdin, &query_buffer) != 0) {
        free(query_buffer);
        free(query_tokens);
        return;
    }

    size_t query_length = strlen(query_buffer);
    if (query_length > 0 && query_buffer[query_length - 1] == '\n') {
        query_buffer[query_length - 1] = '\0';
    }

    for (size_t i = 0; i < query_length; i++) {
        query_buffer[i] = tolower(query_buffer[i]);
    }

    int token_count = 0;
    char *saveptr;
    char *token = strtok_r(query_buffer, " ", &saveptr);
    while (token) {
        query_tokens[token_count++] = token;
        token = strtok_r(NULL, " ", &saveptr);
    }

    LinkedList *search_results = MemIndex_Search(mi, query_tokens, token_count);
    if (!search_results) {
        free(query_buffer);
        free(query_tokens);
        return;
    }

    LLIterator *iterator = LLIterator_Allocate(search_results);
    if (!iterator) {
        LinkedList_Free(search_results, (LLPayloadFreeFnPtr) free);
        free(query_buffer);
        free(query_tokens);
        return;
    }

    int result_count = LinkedList_NumElements(search_results);
    for (int i = 0; i < result_count; i++) {
        SearchResult *match;
        LLIterator_Get(iterator, (LLPayload_t) &match);
        printf("  %s (%d)\n",
          DocTable_GetDocName(dt, match -> doc_id), match -> rank);
        LLIterator_Next(iterator);
    }

    LLIterator_Free(iterator);
    LinkedList_Free(search_results, (LLPayloadFreeFnPtr) free);
    free(query_buffer);
    free(query_tokens);
  }
}

static int GetNextLine(FILE *f, char **ret_str) {
  printf("enter query:\n");
  fgets(*ret_str, BUFFER_SIZE, f);
  return feof(f);
}
