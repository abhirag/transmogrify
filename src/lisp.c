#include "lisp.h"

#include <fe.h>
#include <sds.h>

#include "transmogrify.h"

static char readsds(fe_Context* ctx, void* udata);
static fe_Object* fe_readsds(fe_Context* ctx, sds s);
static fe_Object* f_set_title(fe_Context* ctx, fe_Object* arg);
static fe_Object* f_set_author(fe_Context* ctx, fe_Object* arg);
static fe_Object* f_set_date(fe_Context* ctx, fe_Object* arg);

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

static fe_Object* f_set_title(fe_Context* ctx, fe_Object* arg) {
  char buf[64];
  fe_tostring(ctx, fe_nextarg(ctx, &arg), buf, sizeof(buf));
  set_title(buf);
}

static fe_Object* f_set_author(fe_Context* ctx, fe_Object* arg) {
  char buf[64];
  fe_tostring(ctx, fe_nextarg(ctx, &arg), buf, sizeof(buf));
  set_author(buf);
}

static fe_Object* f_set_date(fe_Context* ctx, fe_Object* arg) {
  char buf[64];
  fe_tostring(ctx, fe_nextarg(ctx, &arg), buf, sizeof(buf));
  set_date(buf);
}

void bind_fns(fe_Context* ctx) {
  fe_set(ctx, fe_symbol(ctx, "set-title"), fe_cfunc(ctx, f_set_title));
  fe_set(ctx, fe_symbol(ctx, "set-author"), fe_cfunc(ctx, f_set_author));
  fe_set(ctx, fe_symbol(ctx, "set-date"), fe_cfunc(ctx, f_set_date));
}
