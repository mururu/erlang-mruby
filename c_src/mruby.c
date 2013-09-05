#include <mruby.h>
#include <mruby/proc.h>
#include <mruby/compile.h>

#include "erl_nif.h"

static ERL_NIF_TERM eval(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]) {
  return enif_make_badarg(env);
}

static ErlNifFunc nif_funcs[] =
{
  {"eval", 1, eval}
};

ERL_NIF_INIT(mruby,nif_funcs,NULL,NULL,NULL,NULL);
