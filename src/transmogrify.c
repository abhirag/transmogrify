#include "transmogrify.h"

#include <assert.h>
#include <fe.h>
#include <log.h>
#include <md4c.h>
#include <pikchr.h>
#include <sds.h>
#include <stdio.h>
#include <string.h>
#include <xxhash.h>

#include "lisp.h"

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
static sds code_block_lang(MD_BLOCK_CODE_DETAIL* detail);
static int process_config_code_block(void* detail, md_latex_data* data);
static int process_fe_code_block(void* detail, md_latex_data* data);
static int process_pikchr_code_block(void* detail, md_latex_data* data);

typedef struct config config;
struct config {
  sds title;
  sds author;
  sds date;
  int pwidth;
  int pheight;
  unsigned flags;
};

static config conf = {
    .title = (void*)0,
    .author = (void*)0,
    .date = (void*)0,
    .pwidth = 0,
    .pheight = 0,
    .flags = (MD_FLAG_NOHTMLBLOCKS | MD_FLAG_NOHTMLSPANS |
              MD_FLAG_NOINDENTEDCODEBLOCKS | MD_FLAG_LATEXMATHSPANS),
};

void set_pwidth(int pwidth) { conf.pwidth = pwidth; }

void set_pheight(int pheight) { conf.pheight = pheight; }

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

void set_date(char const* date) {
  if (conf.date == (void*)0) {
    conf.date = sdsnew(date);
  } else {
    sdsclear(conf.date);
    conf.date = sdscat(conf.date, date);
  }

  if (conf.date == (void*)0) {
    log_fatal("transmogrify::set_date failed\n");
  }
}

char* render_abstract(char* abstract) {
  char const* begin_abstract = "\\begin{abstract}\n";
  char const* end_abstract = "\n\\end{abstract}";
  size_t output_size =
      strlen(begin_abstract) + strlen(end_abstract) + strlen(abstract) + 1;
  char* output = malloc(output_size);
  memset(output, 0, output_size);
  strcat(output, begin_abstract);
  strcat(output, abstract);
  strcat(output, end_abstract);
  return output;
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

int prepend_preamble(md_latex_data* data) {
  assert(conf.author != (void*)0);
  assert(conf.title != (void*)0);
  sds preamble = sdscatprintf(
      sdsempty(),
      "\\documentclass{tufte-handout}\n"
      "\\usepackage{amsmath}\n"
      "\\usepackage{svg}\n"
      "\\usepackage{graphicx}\n"
      "\\setkeys{Gin}{width=\\linewidth,totalheight=\\textheight,"
      "keepaspectratio}\n"
      "\\graphicspath{{graphics/}}\n"
      "\\usepackage{booktabs}\n"
      "\\usepackage{units}\n"
      "\\usepackage{fancyvrb}\n"
      "\\fvset{fontsize=\\normalsize}\n"
      "\\usepackage{multicol}\n"
      "\\usepackage{minted}\n"
      "\\usemintedstyle{bw}\n"
      "\\usepackage{FiraMono}\n"
      "\\usepackage[T1]{fontenc}\n"
      "\\usepackage{enumitem}\n"
      "\\definecolor{carnelian}{rgb}{0.7, 0.11, 0.11}\n"
      "\\usepackage{hyperref}\n"
      "\\hypersetup{colorlinks=true, linkcolor=carnelian, filecolor=carnelian, "
      "urlcolor=carnelian,}\n"
      "\\title{%s}\n"
      "\\author{%s}\n"
      "\\date{%s}\n",
      conf.title, conf.author, conf.date);
  sds str = sdscatsds(preamble, data->output);
  if (!str) {
    log_fatal(
        "transmogrify::prepend_preamble failed in appending to SDS string\n");
    return -1;
  }
  sdsfree(data->output);
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

static sds code_block_lang(MD_BLOCK_CODE_DETAIL* detail) {
  size_t initlen = (size_t)(detail->lang.size);
  sds lang = sdsnewlen(detail->lang.text, initlen);
  return lang;
}

static int process_config_code_block(void* detail, md_latex_data* data) {
  sds open_doc_block = sdsnew(
      "\\begin{document}\n"
      "\\maketitle\n");
  assert(sdscmp(data->output, open_doc_block) == 0);
  int size = 1024 * 1024;
  void* fe_data = malloc(size);
  fe_Context* ctx = fe_open(fe_data, size);
  bind_fns(ctx);
  sds s = eval_sds(ctx, data->code_text);
  fe_close(ctx);
  free(fe_data);
  sdsfree(open_doc_block);
  sdsfree(s);
}

static int process_fe_code_block(void* detail, md_latex_data* data) {
  int return_val = 0;
  int size = 1024 * 1024;
  void* fe_data = malloc(size);
  fe_Context* ctx = fe_open(fe_data, size);
  bind_fns(ctx);
  sds s = eval_sds(ctx, data->code_text);
  if (render_verbatim_sds(s, data) == -1) {
    return_val = -1;
  }
  fe_close(ctx);
  free(fe_data);
  sdsfree(s);
}

static int process_pikchr_code_block(void* detail, md_latex_data* data) {
  assert(conf.pwidth != 0);
  assert(conf.pheight != 0);
  int return_value = 0;
  XXH64_state_t* const state = XXH64_createState();
  if (state == (void*)0) {
    log_fatal(
        "transmogrify::process_pikchr_code_block failed in creating a hash "
        "state\n");
    return_value = -1;
    goto xxhash_cleanup;
  }
  XXH64_hash_t const seed = 0;
  if (XXH64_reset(state, seed) == XXH_ERROR) {
    log_fatal(
        "transmogrify::process_pikchr_code_block failed in initializing the "
        "hash state\n");
    return_value = -1;
    goto xxhash_cleanup;
  }

  unsigned mFlags = PIKCHR_PLAINTEXT_ERRORS;
  char* svg =
      pikchr(data->code_text, (void*)0, mFlags, &conf.pwidth, &conf.pheight);
  if (conf.pwidth < 0) {
    log_fatal(
        "transmogrify::process_pikchr_code_block failed in generating svg text "
        "due to: %s\n",
        svg);
    return_value = -1;
    goto pikchr_xxhash_cleanup;
  }
  if (XXH64_update(state, svg, sizeof(svg)) == XXH_ERROR) {
    log_fatal(
        "transmogrify::process_pikchr_code_block failed in feeding the state "
        "with input data\n");
    return_value = -1;
    goto pikchr_xxhash_cleanup;
  }
  XXH64_hash_t const hash = XXH64_digest(state);
  sds fname = sdscatprintf(sdsempty(), "%lu.svg", hash);
  FILE* fd = fopen(fname, "w");
  if (fd == (void*)0) {
    log_fatal(
        "transmogrify::process_pikchr_code_block failed in opening file: %s\n",
        fname);
    return_value = -1;
    goto sds_pikchr_xxhash_cleanup;
  }
  if (fputs(svg, fd) == EOF) {
    log_fatal(
        "transmogrify::process_pikchr_code_block failed in writing to file: "
        "%s\n",
        fname);
    return_value = -1;
  }
  sds s = sdscatprintf(sdsempty(),
                       "\\begin{figure}\n"
                       "\\fontfamily{lmss}\\fontsize{8pt}{10pt}\\selectfont"
                       "  \\includesvg[width=100mm,height=200mm,keepaspectratio]{%lu.svg}\n"
                       "\\end{figure}",
                       hash);
  if (render_verbatim_sds(s, data) == -1) {
    return_value = -1;
  }
sds_pikchr_xxhash_cleanup:
  sdsfree(fname);
  sdsfree(s);
  fclose(fd);
pikchr_xxhash_cleanup:
  free(svg);
xxhash_cleanup:
  XXH64_freeState(state);
  return return_value;
}

static int process_code_block(void* detail, md_latex_data* data) {
  MD_BLOCK_CODE_DETAIL* det = (MD_BLOCK_CODE_DETAIL*)detail;
  int return_value = 0;
  sds lang = code_block_lang(det);
  sds config_lang = sdsnew("config");
  sds fe_lang = sdsnew("fe");
  sds pikchr_lang = sdsnew("pikchr");
  if (sdscmp(config_lang, lang) == 0) {
    return_value = process_config_code_block(detail, data);
    goto end;
  }
  if (sdscmp(fe_lang, lang) == 0) {
    return_value = process_fe_code_block(detail, data);
    goto end;
  }
  if (sdscmp(pikchr_lang, lang) == 0) {
    return_value = process_pikchr_code_block(detail, data);
    goto end;
  }
  if (render_verbatim("\\begin{minted}[frame=leftline, framesep=10pt, "
                      "fontsize=\\footnotesize]{text}\n",
                      data) == -1) {
    return_value = -1;
    goto end;
  }
  if (render_verbatim_sds(data->code_text, data) == -1) {
    return_value = -1;
    goto end;
  }
  if (render_verbatim("\n\\end{minted}\n", data) == -1) {
    return_value = -1;
    goto end;
  }

end:
  sdsfree(lang);
  sdsfree(config_lang);
  sdsfree(fe_lang);
  sdsfree(pikchr_lang);
  return return_value;
}

static int process_code_span(void* detail, md_latex_data* data) {
  int rc = 0;
  sds to_render = sdsnew("\\texttt{");
  sds empty_str = sdsnew("");
  if (sdscmp(data->code_text, empty_str) == 0) {
    rc = 0;
    goto end;
  }
  if (data->code_text[0] == 'f' && data->code_text[1] == 'e') {
    sdsrange(data->code_text, 2, -1);
    rc = process_fe_code_block(detail, data);
    goto end;
  }
  to_render = sdscatfmt(to_render, "%S}", data->code_text);
  rc = render_verbatim_sds(to_render, data);
end:
  sdsfree(to_render);
  sdsfree(empty_str);
  return rc;
}

static int render_open_h_block(MD_BLOCK_H_DETAIL* detail, md_latex_data* data) {
  int rc = 0;
  if (detail->level == 1) {
    rc = render_verbatim("\\section{", data);
  } else {
    rc = render_verbatim("\\subsection{", data);
  }
  return rc;
}

static int render_closed_h_block(MD_BLOCK_H_DETAIL* detail,
                                 md_latex_data* data) {
  int rc = 0;
  sds to_render = sdsempty();
  if (detail->level == 1) {
    to_render = sdscatfmt(to_render, "}\\label{sec:%u}", data->label);
    rc = render_verbatim_sds(to_render, data);
    if (rc == 0) {
      data->label += 1;
    }
    sdsfree(to_render);
  } else {
    rc = render_verbatim("}", data);
  }
  return rc;
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
    case MD_BLOCK_CODE:
      sdsclear(d->code_text);
      break;
    case MD_BLOCK_H:
      if (render_open_h_block((MD_BLOCK_H_DETAIL*)detail, d) == -1) {
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
    case MD_BLOCK_CODE:
      if (process_code_block(detail, d) == -1) {
        return -1;
      }
      break;
    case MD_BLOCK_H:
      if (render_closed_h_block((MD_BLOCK_H_DETAIL*)detail, d) == -1) {
        return -1;
      }
      break;
  }
  return 0;
}

static int enter_span_callback(MD_SPANTYPE type, void* detail, void* userdata) {
  md_latex_data* d = (md_latex_data*)userdata;
  switch (type) {
    case MD_SPAN_EM:
      if (render_verbatim("\\textit{", d) == -1) {
        return -1;
      }
      break;
    case MD_SPAN_STRONG:
      if (render_verbatim("\\textbf{", d) == -1) {
        return -1;
      }
      break;
    case MD_SPAN_CODE:
      sdsclear(d->code_text);
      break;
  }
  return 0;
}

static int leave_span_callback(MD_SPANTYPE type, void* detail, void* userdata) {
  md_latex_data* d = (md_latex_data*)userdata;
  switch (type) {
    case MD_SPAN_EM:
      if (render_verbatim("}", d) == -1) {
        return -1;
      }
      break;
    case MD_SPAN_STRONG:
      if (render_verbatim("}", d) == -1) {
        return -1;
      }
      break;
    case MD_SPAN_CODE:
      if (process_code_span(detail, d) == -1) {
        return -1;
      }
      break;
  }
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
                      conf.flags,
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
  sdsfree(conf.date);
  sdsfree(data->code_text);
  sdsfree(data->output);
}
