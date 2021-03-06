#include <md4c.h>
#include <sds.h>

typedef struct md_latex_data md_latex_data;
struct md_latex_data {
  sds output;
  sds code_text;
  unsigned int label;
};

int md_latex(const MD_CHAR* input, MD_SIZE input_size, md_latex_data* data);
int prepend_preamble(md_latex_data* data);
void set_title(char const* title);
void set_author(char const* author);
void set_date(char const* date);
void set_pwidth(int pwidth);
void set_pheight(int pheight);
char* render_abstract(char* abstract);
void transmogrify_free(md_latex_data* data);