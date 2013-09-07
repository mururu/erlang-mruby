#include <stdio.h>
#include <string.h>
#include <mruby.h>
#include <mruby/proc.h>
#include <mruby/compile.h>
#include <mruby/string.h>

#include "erl_nif.h"

static int _mrb_fixnum(mrb_value o) { return (int) mrb_fixnum(o); }
static float _mrb_float(mrb_value o) { return (float) mrb_float(o); }
static const char *_mrb_symbol(mrb_state* mrb, mrb_value o) {
  mrb_sym id = mrb_symbol(o);
  size_t len;

  return mrb_sym2name_len(mrb, id, &len);
}
static const char *_mrb_string(mrb_state* mrb, mrb_value o) { return mrb_string_value_ptr(mrb, o); }

static ERL_NIF_TERM make_binary(ErlNifEnv* env, const char* bin) {
  ErlNifBinary new_bin;
  size_t len = strlen(bin);
  enif_alloc_binary(len, &new_bin);
  memcpy(new_bin.data, bin, len);
  return enif_make_binary(env, &new_bin);
}

static ERL_NIF_TERM make_array() {
  return enif_make_atom(env, "todo_array");
}

static ERL_NIF_TERM make_hash() {
 return enif_make_atom(env, "todo_hash");
}

static ERL_NIF_TERM mruby2erl(ErlNifEnv* env, mrb_state* mrb, mrb_value value) {
  if (mrb_nil_p(value)) {
    return enif_make_atom(env, "nil");
  } else {
    switch(value.tt) {
      case MRB_TT_TRUE:
        return enif_make_atom(env, "true");
      case MRB_TT_FALSE:
        return enif_make_atom(env, "false");
      case MRB_TT_SYMBOL:
        return enif_make_atom(env, _mrb_symbol(mrb, value));
      case MRB_TT_FIXNUM:
        return enif_make_int(env, _mrb_fixnum(value));
      case MRB_TT_FLOAT:
        return enif_make_double(env, _mrb_float(value));
      case MRB_TT_STRING:
        return make_binary(env, _mrb_string(mrb, value));
      case MRB_TT_ARRAY:
        return make_array();
      case MRB_TT_HASH:
        return make_hash();
      default :
        return enif_make_string(env, "undefined return type", ERL_NIF_LATIN1);
    }
  }
}

static ERL_NIF_TERM eval(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]) {
  ErlNifBinary script_binary;

  if (!enif_inspect_binary(env, argv[0], &script_binary)){
    return enif_make_badarg(env);
  }

  mrb_state *mrb;
  mrbc_context *cxt;

  mrb = mrb_open();

  if (mrb == NULL) {
    return enif_make_atom(env, "error");
  }

  cxt = mrbc_context_new(mrb);

  char *script = malloc(script_binary.size);

  strncpy(script, (const char *)script_binary.data, (int)script_binary.size);

  mrb_value result = mrb_load_string_cxt(mrb, (const char *)script, cxt);

  free(script);

  ERL_NIF_TERM erl_result = mruby2erl(env, mrb, result);

  mrbc_context_free(mrb, cxt);
  mrb_close(mrb);

  enif_release_binary(&script_binary);


  return erl_result;
}

static ErlNifFunc nif_funcs[] =
{
  {"eval", 1, eval}
};

ERL_NIF_INIT(mruby,nif_funcs,NULL,NULL,NULL,NULL);
