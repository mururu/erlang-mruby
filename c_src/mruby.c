#include <stdio.h>
#include <string.h>
#include <mruby.h>
#include <mruby/proc.h>
#include <mruby/compile.h>

#include "erl_nif.h"

static int _mrb_fixnum(mrb_value o) { return (int) mrb_fixnum(o); }
static float _mrb_float(mrb_value o) { return (float) mrb_float(o); }

static ERL_NIF_TERM mruby2erl(ErlNifEnv* env, mrb_value value) {
  switch(value.tt) {
    case MRB_TT_TRUE:
      return enif_make_atom(env, "true");
    case MRB_TT_FALSE:
      return enif_make_atom(env, "false");
    case MRB_TT_FIXNUM:
      return enif_make_int(env, _mrb_fixnum(value));
    case MRB_TT_FLOAT:
      return enif_make_double(env, _mrb_float(value));
    default :
      return enif_make_string(env, "undefined return type", ERL_NIF_LATIN1);
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

  mrbc_context_free(mrb, cxt);
  mrb_close(mrb);

  enif_release_binary(&script_binary);

  ERL_NIF_TERM erl_result = mruby2erl(env, result);

  return erl_result;
}

static ErlNifFunc nif_funcs[] =
{
  {"eval", 1, eval}
};

ERL_NIF_INIT(mruby,nif_funcs,NULL,NULL,NULL,NULL);
