#include <log.h>
#include <sds.h>
#include <string.h>

#include "transmogrify.h"

int main(void) {
  md_latex_data d = {.code_text = sdsempty(), .output = sdsempty()};
  md_latex(
      "```config\n(set-title \"testing123\") (set-author \"abhirag\") "
      "(set-date "
      "\"7 June "
      "2021\")\n```\n test\n```fe\n(abstract \"this is abstract\")\n```",
      strlen(
          "```config\n(set-title \"testing123\") (set-author \"abhirag\") "
          "(set-date \"7 June "
          "2021\")\n```\n test\n```fe\n(abstract \"this is abstract\")\n```"),
      &d);
  prepend_preamble(&d);
  log_trace("%s\n", d.output);
  transmogrify_free(&d);
  return 0;
}
