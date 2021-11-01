#include "lisp.h"

#include <fe.h>
#include <sds.h>

static char readsds(fe_Context* ctx, void* udata);
static fe_Object* fe_readsds(fe_Context* ctx, sds s);

static char readsds(fe_Context* ctx, void* udata) {
  sds s = (sds)udata;
  char chr = s[0];
  sdsrange(s, 1, -1);
  return chr;
}

static fe_Object* fe_readsds(fe_Context* ctx, sds s) {
  return fe_read(ctx, readsds, s);
}

void eval_sds(fe_Context* ctx, sds s) {
  int gc = fe_savegc(ctx);

  for (;;) {
    fe_Object* obj = fe_readsds(ctx, s);
    if (!obj) {
      break;
    }
    fe_eval(ctx, obj);
    fe_restoregc(ctx, gc);
  }
}
