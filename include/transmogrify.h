#include <md4c.h>
#include <sds.h>

typedef struct md_latex_data md_latex_data;
struct md_latex_data {
  sds output;
  MD_BLOCK_CODE_DETAIL* code_block_detail;
  unsigned flags;
};

int md_latex(const MD_CHAR* input, MD_SIZE input_size, md_latex_data* data);