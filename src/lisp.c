#include "lisp.h"

#include <fe.h>
#include <sds.h>
#include <string.h>

#include "transmogrify.h"

static char readsds(fe_Context* ctx, void* udata);
static fe_Object* fe_readsds(fe_Context* ctx, sds s);
static fe_Object* f_set_title(fe_Context* ctx, fe_Object* arg);
static fe_Object* f_set_author(fe_Context* ctx, fe_Object* arg);
static fe_Object* f_set_date(fe_Context* ctx, fe_Object* arg);
static fe_Object* f_abstract(fe_Context* ctx, fe_Object* arg);
static fe_Object* f_set_pwidth(fe_Context* ctx, fe_Object* arg);
static fe_Object* f_set_pheight(fe_Context* ctx, fe_Object* arg);
static fe_Object* f_toc(fe_Context* ctx, fe_Object* arg);
static fe_Object* f_marginnote(fe_Context* ctx, fe_Object* arg);
static fe_Object* f_sidenote(fe_Context* ctx, fe_Object* arg);
static fe_Object* f_italic(fe_Context* ctx, fe_Object* arg);
static fe_Object* f_bold(fe_Context* ctx, fe_Object* arg);
static fe_Object* f_concat(fe_Context* ctx, fe_Object* arg);

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

static fe_Object* f_toc(fe_Context* ctx, fe_Object* arg) {
  unsigned int label = 1;
  sds to_render = sdsnew(
      "\\marginnote{\n"
      "  \\textsc{table of contents}\n"
      "  \\begin{enumerate}[nosep]\n");
  char buf[1000];
  while (!fe_isnil(ctx, arg)) {
    memset(buf, 0, 1000);
    fe_tostring(ctx, fe_nextarg(ctx, &arg), buf, sizeof(buf));
    to_render =
        sdscatfmt(to_render, "    \\item \\hyperref[sec:%u]{%s}\n", label, buf);
    label += 1;
  }
  to_render = sdscat(to_render, "\\end{enumerate}\n}");
  fe_Object* o = fe_string(ctx, to_render);
  sdsfree(to_render);
  return o;
}

static fe_Object* f_marginnote(fe_Context* ctx, fe_Object* arg) {
  sds to_render = sdsnew("\\marginnote[");
  char text[1000];
  char offset[1000];
  memset(text, 0, 1000);
  memset(offset, 0, 1000);
  fe_tostring(ctx, fe_nextarg(ctx, &arg), text, sizeof(text));
  fe_tostring(ctx, fe_nextarg(ctx, &arg), offset, sizeof(offset));
  to_render = sdscatfmt(to_render, "%smm]{%s}", offset, text);
  fe_Object* o = fe_string(ctx, to_render);
  sdsfree(to_render);
  return o;
}

static fe_Object* f_sidenote(fe_Context* ctx, fe_Object* arg) {
  sds to_render = sdsnew("\\sidenote[][");
  char text[1000];
  char offset[1000];
  memset(text, 0, 1000);
  memset(offset, 0, 1000);
  fe_tostring(ctx, fe_nextarg(ctx, &arg), text, sizeof(text));
  fe_tostring(ctx, fe_nextarg(ctx, &arg), offset, sizeof(offset));
  to_render = sdscatfmt(to_render, "%smm]{%s}", offset, text);
  fe_Object* o = fe_string(ctx, to_render);
  sdsfree(to_render);
  return o;
}

static fe_Object* f_italic(fe_Context* ctx, fe_Object* arg) {
  sds to_render = sdsnew("\\textit{");
  char buf[1000];
  memset(buf, 0, 1000);
  fe_tostring(ctx, fe_nextarg(ctx, &arg), buf, sizeof(buf));
  to_render = sdscatfmt(to_render, "%s}", buf);
  fe_Object* o = fe_string(ctx, to_render);
  sdsfree(to_render);
  return o;
}

static fe_Object* f_bold(fe_Context* ctx, fe_Object* arg) {
  sds to_render = sdsnew("\\textbf{");
  char buf[1000];
  memset(buf, 0, 1000);
  fe_tostring(ctx, fe_nextarg(ctx, &arg), buf, sizeof(buf));
  to_render = sdscatfmt(to_render, "%s}", buf);
  fe_Object* o = fe_string(ctx, to_render);
  sdsfree(to_render);
  return o;
}

static fe_Object* f_concat(fe_Context* ctx, fe_Object* arg) {
  sds to_render = sdsempty();
  char buf[1000];
  while (!fe_isnil(ctx, arg)) {
    memset(buf, 0, 1000);
    fe_tostring(ctx, fe_nextarg(ctx, &arg), buf, sizeof(buf));
    to_render = sdscatfmt(to_render, " %s", buf);
  }
  fe_Object* o = fe_string(ctx, to_render);
  sdsfree(to_render);
  return o;
}

void bind_fns(fe_Context* ctx) {
  fe_set(ctx, fe_symbol(ctx, "set-title"), fe_cfunc(ctx, f_set_title));
  fe_set(ctx, fe_symbol(ctx, "set-author"), fe_cfunc(ctx, f_set_author));
  fe_set(ctx, fe_symbol(ctx, "set-date"), fe_cfunc(ctx, f_set_date));
  fe_set(ctx, fe_symbol(ctx, "set-pwidth"), fe_cfunc(ctx, f_set_pwidth));
  fe_set(ctx, fe_symbol(ctx, "set-pheight"), fe_cfunc(ctx, f_set_pheight));
  fe_set(ctx, fe_symbol(ctx, "abstract"), fe_cfunc(ctx, f_abstract));
  fe_set(ctx, fe_symbol(ctx, "toc"), fe_cfunc(ctx, f_toc));
  fe_set(ctx, fe_symbol(ctx, "marginnote"), fe_cfunc(ctx, f_marginnote));
  fe_set(ctx, fe_symbol(ctx, "sidenote"), fe_cfunc(ctx, f_sidenote));
  fe_set(ctx, fe_symbol(ctx, "italic"), fe_cfunc(ctx, f_italic));
  fe_set(ctx, fe_symbol(ctx, "bold"), fe_cfunc(ctx, f_bold));
  fe_set(ctx, fe_symbol(ctx, "concat"), fe_cfunc(ctx, f_concat));
}
