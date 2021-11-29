#include "lisp.h"

#include <fe.h>
#include <sds.h>

#include "transmogrify.h"

static char readsds(fe_Context* ctx, void* udata);
static fe_Object* fe_readsds(fe_Context* ctx, sds s);
static fe_Object* f_set_title(fe_Context* ctx, fe_Object* arg);
static fe_Object* f_set_author(fe_Context* ctx, fe_Object* arg);
static fe_Object* f_set_date(fe_Context* ctx, fe_Object* arg);
static fe_Object* f_abstract(fe_Context* ctx, fe_Object* arg);
static fe_Object* f_set_pwidth(fe_Context* ctx, fe_Object* arg);
static fe_Object* f_set_pheight(fe_Context* ctx, fe_Object* arg);

static char readsds(fe_Context* ctx, void* udata) {
  sds s = (sds)udata;
  char chr = s[0];
  sdsrange(s, 1, -1);
  return chr;
}

static fe_Object* fe_readsds(fe_Context* ctx, sds s) {
  return fe_read(ctx, readsds, s);
}

sds eval_sds(fe_Context* ctx, sds s) {
  sds result = sdsempty();
  int gc = fe_savegc(ctx);

  for (;;) {
    fe_Object* obj = fe_readsds(ctx, s);
    if (!obj) {
      break;
    }
    fe_Object* o = fe_eval(ctx, obj);
    if (!fe_isnil(ctx, o)) {
      char buf[1500];
      fe_tostring(ctx, o, buf, sizeof(buf));
      result = sdscat(result, buf);
    }
    fe_restoregc(ctx, gc);
  }
  return result;
}

static fe_Object* f_set_title(fe_Context* ctx, fe_Object* arg) {
  char buf[64];
  fe_tostring(ctx, fe_nextarg(ctx, &arg), buf, sizeof(buf));
  set_title(buf);
  return fe_bool(ctx, 0);
}

static fe_Object* f_set_author(fe_Context* ctx, fe_Object* arg) {
  char buf[64];
  fe_tostring(ctx, fe_nextarg(ctx, &arg), buf, sizeof(buf));
  set_author(buf);
  return fe_bool(ctx, 0);
}

static fe_Object* f_set_date(fe_Context* ctx, fe_Object* arg) {
  char buf[64];
  fe_tostring(ctx, fe_nextarg(ctx, &arg), buf, sizeof(buf));
  set_date(buf);
  return fe_bool(ctx, 0);
}

static fe_Object* f_set_pwidth(fe_Context* ctx, fe_Object* arg) {
  float pwidth = fe_tonumber(ctx, fe_nextarg(ctx, &arg));
  set_pwidth((int)pwidth);
  return fe_bool(ctx, 0);
}

static fe_Object* f_set_pheight(fe_Context* ctx, fe_Object* arg) {
  float pheight = fe_tonumber(ctx, fe_nextarg(ctx, &arg));
  set_pheight((int)pheight);
  return fe_bool(ctx, 0);
}

static fe_Object* f_abstract(fe_Context* ctx, fe_Object* arg) {
  char buf[1000];
  fe_tostring(ctx, fe_nextarg(ctx, &arg), buf, sizeof(buf));
  char* abstract = render_abstract(buf);
  fe_Object* o = fe_string(ctx, abstract);
  free(abstract);
  return o;
}

void bind_fns(fe_Context* ctx) {
  fe_set(ctx, fe_symbol(ctx, "set-title"), fe_cfunc(ctx, f_set_title));
  fe_set(ctx, fe_symbol(ctx, "set-author"), fe_cfunc(ctx, f_set_author));
  fe_set(ctx, fe_symbol(ctx, "set-date"), fe_cfunc(ctx, f_set_date));
  fe_set(ctx, fe_symbol(ctx, "set-pwidth"), fe_cfunc(ctx, f_set_pwidth));
  fe_set(ctx, fe_symbol(ctx, "set-pheight"), fe_cfunc(ctx, f_set_pheight));
  fe_set(ctx, fe_symbol(ctx, "abstract"), fe_cfunc(ctx, f_abstract));
}
