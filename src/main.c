#include <md4c.h>
#include <sds.h>
#include <stdio.h>

#include "transmogrify.h"

int main(void) {
  md_latex_data d = {
      .flags = (MD_FLAG_NOHTMLBLOCKS | MD_FLAG_NOHTMLSPANS |
                MD_FLAG_NOINDENTEDCODEBLOCKS | MD_FLAG_LATEXMATHSPANS),
      .code_block_detail = (void*)0,
      .output = sdsempty()};
  md_latex("this is a paragraph", sizeof("this is a paragraph"), &d);
  printf("%s\n", d.output);
  sdsfree(d.output);
  return 0;
}
