#include "transmogrify.h"

#include <string.h>

#include "utest.h"

UTEST(render, paragraph) {
  md_latex_data d = {
      .flags = (MD_FLAG_NOHTMLBLOCKS | MD_FLAG_NOHTMLSPANS |
                MD_FLAG_NOINDENTEDCODEBLOCKS | MD_FLAG_LATEXMATHSPANS),
      .code_block_detail = (void*)0,
      .output = sdsempty()};
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
  sdsfree(d.output);
}

UTEST(render, normal_code_block) {
  md_latex_data d = {
      .flags = (MD_FLAG_NOHTMLBLOCKS | MD_FLAG_NOHTMLSPANS |
                MD_FLAG_NOINDENTEDCODEBLOCKS | MD_FLAG_LATEXMATHSPANS),
      .code_block_detail = (void*)0,
      .output = sdsempty()};
  char const* input = "```\nthis is a normal code block\n```";
  md_latex(input, strlen(input), &d);
  sds expected_output = sdsempty();
  expected_output =
      sdscatprintf(expected_output,
                   "\\begin{minted}[frame=leftline, framesep=10pt, "
                   "fontsize=\\footnotesize]{text}\n%s"
                   "\\end{minted}",
                   input);
  ASSERT_EQ(0, sdscmp(expected_output, d.output));
  sdsfree(d.output);
}

UTEST_MAIN();
