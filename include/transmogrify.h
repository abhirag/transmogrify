#include <md4c.h>
#include <sds.h>

typedef struct md_latex_data md_latex_data;
struct md_latex_data {
  sds output;
  sds code_text;
  unsigned flags;
};

int md_latex(const MD_CHAR* input, MD_SIZE input_size, md_latex_data* data);
void set_title(char const* title);