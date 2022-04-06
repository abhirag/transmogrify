#include <assert.h>
#include <log.h>
#include <sds.h>
#include <stdlib.h>
#include <string.h>

#include "transmogrify.h"

static char* read_file_to_str(const char* path, unsigned int* file_len_out);

static char* read_file_to_str(const char* path, unsigned int* file_len_out) {
  FILE* file;
  int e;

  file = fopen(path, "rb");
  if (!file) {
    log_error("indexer::read_file_to_str unable to open file %s", path);
    return (void*)0;
  }

  e = fseek(file, 0, SEEK_END);
  if (-1 == e) {
    log_error("indexer::read_file_to_str unable to seek file %s", path);
    fclose(file);
    return (void*)0;
  }

  long file_len = ftell(file);
  if (-1 == file_len) {
    log_error("indexer::read_file_to_str unable to ftell() file %s", path);
    fclose(file);
    return (void*)0;
  }

  e = fseek(file, 0, SEEK_SET);
  if (-1 == e) {
    log_error("indexer::read_file_to_str unable to seek file %s", path);
    fclose(file);
    return (void*)0;
  }

  char* contents = malloc(file_len + 1);
  if (!contents) {
    log_error("indexer::read_file_to_str memory error!");
    fclose(file);
    return (void*)0;
  }

  unsigned long bytes_read = fread(contents, file_len, 1, file);
  if (bytes_read == 0 && ferror(file)) {
    log_error("indexer::read_file_to_str read error");
    free(contents);
    fclose(file);
    return (void*)0;
  }
  fclose(file);

  contents[file_len] = '\0';

  if (file_len_out) *file_len_out = file_len + 1;

  return contents;
}

int main(int argc, char** argv) {
  md_latex_data d = {
      .code_text = sdsempty(),
      .output = sdsempty(),
      .label = 1,
  };
  assert(argc == 3);
  int file_len = 0;
  char* contents = read_file_to_str(argv[1], &file_len);
  md_latex(contents, file_len, &d);
  prepend_preamble(&d);
  FILE* fp = fopen(argv[2], "w");
  fprintf(fp, d.output);
  fclose(fp);
  transmogrify_free(&d);
  free(contents);
  return 0;
}
