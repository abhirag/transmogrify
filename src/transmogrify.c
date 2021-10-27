#include "transmogrify.h"

#include <md4c.h>
#include <sds.h>
#include <stdio.h>

static int enter_block_callback(MD_BLOCKTYPE type, void* detail,
                                void* userdata);
static int leave_block_callback(MD_BLOCKTYPE type, void* detail,
                                void* userdata);
static int enter_span_callback(MD_SPANTYPE type, void* detail, void* userdata);
static int leave_span_callback(MD_SPANTYPE type, void* detail, void* userdata);
static int text_callback(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size,
                         void* userdata);
static void debug_log_callback(const char* msg, void* userdata);
static int render_verbatim(char const* text, md_latex_data* data);
static int render_verbatim_len(MD_CHAR* text, MD_SIZE size,
                               md_latex_data* data);

static int render_verbatim(char const* text, md_latex_data* data) {
  sds str = sdscat(data->output, text);
  if (!str) {
    fprintf(
        stderr,
        "transmogrify::render_verbatim failed in appending to SDS string\n");
    return -1;
  }
  data->output = str;
  return 0;
}

static int render_verbatim_len(MD_CHAR* text, MD_SIZE size,
                               md_latex_data* data) {
  sds str = sdscatlen(data->output, text, size);
  if (!str) {
    fprintf(stderr,
            "transmogrify::render_verbatim_len failed in appending to SDS "
            "string\n");
    return -1;
  }
  data->output = str;
  return 0;
}

static int enter_block_callback(MD_BLOCKTYPE type, void* detail,
                                void* userdata) {
  md_latex_data* d = (md_latex_data*)userdata;
  switch (type) {
    case MD_BLOCK_DOC:
      if (render_verbatim("\\begin{document}\n"
                          "\\maketitle\n",
                          d) == -1) {
        return -1;
      }
      break;
    case MD_BLOCK_P:
      if (render_verbatim("\n", d) == -1) {
        return -1;
      }
      break;
  }
  return 0;
}

static int leave_block_callback(MD_BLOCKTYPE type, void* detail,
                                void* userdata) {
  md_latex_data* d = (md_latex_data*)userdata;
  switch (type) {
    case MD_BLOCK_DOC:
      if (render_verbatim("\n\\end{document}", d) == -1) {
        return -1;
      }
      break;
    case MD_BLOCK_P:
      break;
  }
  return 0;
}

static int enter_span_callback(MD_SPANTYPE type, void* detail, void* userdata) {
  return 0;
}

static int leave_span_callback(MD_SPANTYPE type, void* detail, void* userdata) {
  return 0;
}

static int text_callback(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size,
                         void* userdata) {
  md_latex_data* d = (md_latex_data*)userdata;
  switch (type) {
    case MD_TEXT_NORMAL:
      if (render_verbatim_len(text, size, d) == -1) {
        return -1;
      }
      break;
  }
  return 0;
}

static void debug_log_callback(const char* msg, void* userdata) {}

int md_latex(const MD_CHAR* input, MD_SIZE input_size, md_latex_data* data) {
  MD_PARSER parser = {0,
                      data->flags,
                      enter_block_callback,
                      leave_block_callback,
                      enter_span_callback,
                      leave_span_callback,
                      text_callback,
                      debug_log_callback,
                      (void*)0};

  return md_parse(input, input_size, &parser, data);
}
