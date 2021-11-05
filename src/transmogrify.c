#include "transmogrify.h"

#include <log.h>
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
static int render_verbatim(MD_CHAR* text, md_latex_data* data);
static int render_verbatim_sds(sds text, md_latex_data* data);
static int render_verbatim_len(MD_CHAR* text, MD_SIZE size,
                               md_latex_data* data);
static int accumulate_code_text(MD_CHAR* text, MD_SIZE size,
                                md_latex_data* data);
static int process_code_block(void* detail, md_latex_data* data);

typedef struct config config;
struct config {
  sds title;
  sds author
};

static config conf = {.title = (void*)0, .author = (void*)0};

void set_title(char const* title) {
  if (conf.title == (void*)0) {
    conf.title = sdsnew(title);
  } else {
    sdsclear(conf.title);
    conf.title = sdscat(conf.title, title);
  }

  if (conf.title == (void*)0) {
    log_fatal("transmogrify::set_title failed\n");
  }
}

void set_author(char const* author) {
  if (conf.author == (void*)0) {
    conf.author = sdsnew(author);
  } else {
    sdsclear(conf.author);
    conf.author = sdscat(conf.author, author);
  }

  if (conf.author == (void*)0) {
    log_fatal("transmogrify::set_author failed\n");
  }
}

static int render_verbatim(MD_CHAR* text, md_latex_data* data) {
  sds str = sdscat(data->output, text);
  if (!str) {
    log_fatal(
        "transmogrify::render_verbatim failed in appending to SDS string\n");
    return -1;
  }
  data->output = str;
  return 0;
}

static int render_verbatim_sds(sds text, md_latex_data* data) {
  sds str = sdscatsds(data->output, text);
  if (!str) {
    log_fatal(
        "transmogrify::render_verbatim_sds failed in appending to SDS "
        "string\n");
    return -1;
  }
  data->output = str;
  return 0;
}

static int render_verbatim_len(MD_CHAR* text, MD_SIZE size,
                               md_latex_data* data) {
  sds str = sdscatlen(data->output, text, size);
  if (!str) {
    log_fatal(
        "transmogrify::render_verbatim_len failed in appending to SDS "
        "string\n");
    return -1;
  }
  data->output = str;
  return 0;
}

static int accumulate_code_text(MD_CHAR* text, MD_SIZE size,
                                md_latex_data* data) {
  sds str = sdscatlen(data->code_text, text, size);
  if (!str) {
    log_fatal(
        "transmogrify::accumulate_code_text failed in appending to SDS "
        "string\n");
    return -1;
  }
  data->code_text = str;
  return 0;
}

static int process_code_block(void* detail, md_latex_data* data) {
  MD_BLOCK_CODE_DETAIL* det = (MD_BLOCK_CODE_DETAIL*)detail;
  if (render_verbatim("\\begin{minted}[frame=leftline, framesep=10pt, "
                      "fontsize=\\footnotesize]{text}\n",
                      data) == -1) {
    return -1;
  }
  if (render_verbatim_sds(data->code_text, data) == -1) {
    return -1;
  }
  if (render_verbatim("\n\\end{minted}", data) == -1) {
    return -1;
  }

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
    case MD_BLOCK_CODE:  // noop
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
    case MD_BLOCK_CODE:
      if (process_code_block(detail, d) == -1) {
        return -1;
      }
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
    case MD_TEXT_CODE:
      if (accumulate_code_text(text, size, d) == -1) {
        return -1;
      }
      break;
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

void transmogrify_free(md_latex_data* data) {
  sdsfree(conf.title);
  sdsfree(conf.author);
  sdsfree(data->code_text);
  sdsfree(data->output);
}
