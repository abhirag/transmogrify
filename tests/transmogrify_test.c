#include "transmogrify.h"

#include <string.h>

#include "utest.h"

static unsigned flags = (MD_FLAG_NOHTMLBLOCKS | MD_FLAG_NOHTMLSPANS |
                         MD_FLAG_NOINDENTEDCODEBLOCKS | MD_FLAG_LATEXMATHSPANS);

UTEST(render, paragraph) {
  md_latex_data d = {
      .flags = flags, .code_text = sdsempty(), .output = sdsempty()};
  char const* input = "this is a paragraph";
  md_latex(input, strlen(input), &d);
  sds expected_output = sdsempty();
  expected_output = sdscatprintf(expected_output,
                                 "\\begin{document}\n"
                                 "\\maketitle\n"
                                 "\n%s"
                                 "\n\\end{document}",
                                 input);
  ASSERT_EQ(sdslen(expected_output), sdslen(d.output));
  ASSERT_EQ(0, sdscmp(expected_output, d.output));
  transmogrify_free(&d);
  sdsfree(expected_output);
}

UTEST(render, normal_code_block) {
  md_latex_data d = {
      .flags = flags, .code_text = sdsempty(), .output = sdsempty()};
  char const* input = "```\nthis is a normal code block\n```";
  md_latex(input, strlen(input), &d);
  sds expected_output = sdsempty();
  expected_output =
      sdscatprintf(expected_output,
                   "\\begin{document}\n"
                   "\\maketitle\n"
                   "\\begin{minted}[frame=leftline, framesep=10pt, "
                   "fontsize=\\footnotesize]{text}\n"
                   "this is a normal code block\n"
                   "\n\\end{minted}"
                   "\n\\end{document}",
                   input);
  ASSERT_EQ(0, sdscmp(expected_output, d.output));
  transmogrify_free(&d);
  sdsfree(expected_output);
}

UTEST_MAIN();
