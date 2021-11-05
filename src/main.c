#include <fe.h>
#include <log.h>
#include <md4c.h>
#include <sds.h>
#include <stdio.h>
#include <string.h>

#include "lisp.h"
#include "transmogrify.h"

int main(void) {
  int size = 1024 * 1024;
  void* data = malloc(size);
  fe_Context* ctx = fe_open(data, size);
  bind_fns(ctx);
  sds s = sdsnew("(set-title \"testing123\") (set-author \"abhirag\")");
  eval_sds(ctx, s);
  md_latex_data d = {
      .flags = (MD_FLAG_NOHTMLBLOCKS | MD_FLAG_NOHTMLSPANS |
                MD_FLAG_NOINDENTEDCODEBLOCKS | MD_FLAG_LATEXMATHSPANS),
      .code_text = sdsempty(),
      .output = sdsempty()};
  md_latex("```\nthis is a normal code block\n```",
           strlen("```\nthis is a normal code block\n```"), &d);
  log_trace("%s\n", d.output);
  transmogrify_free(&d);
  fe_close(ctx);
  free(data);
  sdsfree(s);
  return 0;
}
