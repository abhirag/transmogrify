#include <log.h>
#include <md4c.h>
#include <sds.h>
#include <stdio.h>
#include <string.h>

#include "transmogrify.h"

int main(void) {
  md_latex_data d = {
      .flags = (MD_FLAG_NOHTMLBLOCKS | MD_FLAG_NOHTMLSPANS |
                MD_FLAG_NOINDENTEDCODEBLOCKS | MD_FLAG_LATEXMATHSPANS),
      .code_text = sdsempty(),
      .output = sdsempty()};
  md_latex("```\nthis is a normal code block\n```",
           strlen("```\nthis is a normal code block\n```"), &d);
  log_trace("%s\n", d.output);
  sdsfree(d.output);
  sdsfree(d.code_text);
  return 0;
}
