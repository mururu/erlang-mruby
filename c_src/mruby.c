#include <stdio.h>
#include <string.h>

#include <mruby.h>
#include <mruby/proc.h>
#include <mruby/compile.h>
#include <mruby/string.h>
#include <mruby/array.h>
#include <mruby/hash.h>

#include "erl_nif.h"

static ERL_NIF_TERM mruby2erl(ErlNifEnv* env, mrb_state* mrb, mrb_value value);

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

static ERL_NIF_TERM make_array(ErlNifEnv* env, mrb_state* mrb, mrb_value o) {
  size_t len = (int) RARRAY(o)->len;
  ERL_NIF_TERM list = enif_make_list_from_array(env, NULL, 0);

  for(int i = len; i>0; --i) {
    ERL_NIF_TERM term = mruby2erl(env, mrb, mrb_ary_ref(mrb, o, (mrb_int)i - 1));
    list = enif_make_list_cell(env, term, list);
  }

  return list;
}

static ERL_NIF_TERM make_hash(ErlNifEnv* env, mrb_state* mrb, mrb_value o) {
  mrb_value keys = mrb_hash_keys(mrb, o);
  size_t len = (int) RARRAY(keys)->len;
  ERL_NIF_TERM list = enif_make_list_from_array(env, NULL, 0);

  for(int i = len; i>0; --i) {
    mrb_value k = mrb_ary_ref(mrb, keys, (mrb_int)i - 1);
    ERL_NIF_TERM key = mruby2erl(env, mrb, k);
    ERL_NIF_TERM value = mruby2erl(env, mrb, mrb_hash_get(mrb, o, k));
    list = enif_make_list_cell(env, enif_make_tuple2(env, key, value), list);
  }

  return enif_make_tuple1(env, list);
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
        return make_array(env, mrb, value);

      case MRB_TT_HASH:
        return make_hash(env, mrb, value);

      default :
        return enif_make_string(env, "undefined return type", ERL_NIF_LATIN1);
    }
  }
}

static mrb_value erl2mruby(ErlNifEnv* env, mrb_state* mrb, ERL_NIF_TERM term) {
  if (enif_is_atom(env, term)) {
    unsigned len;
    enif_get_atom_length(env, term, &len, ERL_NIF_LATIN1);
    char * atom_str = (char *)malloc(sizeof(char)*(len+1));
    int r = enif_get_atom(env, term, atom_str, len+1, ERL_NIF_LATIN1);
    mrb_value value;
    if(strncmp(atom_str, "nil", r) == 0){
      value = mrb_nil_value();
    }else if(strncmp(atom_str, "true", r) == 0){
      value = mrb_true_value();
    }else if(strncmp(atom_str, "false", r) == 0){
      value = mrb_false_value();
    }else{
      value = mrb_symbol_value(mrb_intern_cstr(mrb, atom_str));
    }
    free(atom_str);
    return value;
  } else if (enif_is_binary(env, term)) {
    ErlNifBinary bin;
    enif_inspect_binary(env, term, &bin);
    return mrb_str_new(mrb, (const char *)bin.data, bin.size);
  } else if (enif_is_number(env, term)) {
    double d;
    if (enif_get_double(env, term, &d)) {
      return mrb_float_value(mrb, (mrb_float)d);
    } else {
      ErlNifSInt64 i;
      enif_get_int64(env, term, &i);
      return mrb_fixnum_value((mrb_int)i);
    }
  } else if (enif_is_empty_list(env, term)) {
    return mrb_ary_new(mrb);
  } else if (enif_is_list(env, term)) {
    unsigned len;
    enif_get_list_length(env, term, &len);
    mrb_value ary = mrb_ary_new(mrb);
    ERL_NIF_TERM cur;
    for (cur = term; !enif_is_empty_list(env, cur); ) {
      ERL_NIF_TERM head, tail;
      enif_get_list_cell(env, cur, &head, &tail);

      mrb_ary_push(mrb, ary, erl2mruby(env, mrb, head));
      cur = tail;
    }
    return ary;
  } else if (enif_is_tuple(env, term)) {
    int arity;
    const ERL_NIF_TERM * array;
    enif_get_tuple(env, term, &arity, &array);

    unsigned len = 0;
    enif_get_list_length(env, array[0], &len);
    mrb_value hash = mrb_hash_new(mrb);

    ERL_NIF_TERM cur;
    for(cur = array[0]; !enif_is_empty_list(env, cur); ){
      ERL_NIF_TERM head, tail;
      enif_get_list_cell(env, cur, &head, &tail);
      const ERL_NIF_TERM * array0;
      int arity0;
      enif_get_tuple(env, head, &arity0, &array0);

      mrb_hash_set(mrb, hash, erl2mruby(env, mrb, array0[0]), erl2mruby(env, mrb, array0[1]));

      cur = tail;
    }
    return hash;
  } else {
    return mrb_nil_value();
  }
}

static ERL_NIF_TERM eval1(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]) {
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

  char *script = malloc(script_binary.size+1);
  script[script_binary.size] = '\0';

  strncpy(script, (const char *)script_binary.data, (int)script_binary.size);

  mrb_value result = mrb_load_string_cxt(mrb, (const char *)script, cxt);

  free(script);

  ERL_NIF_TERM erl_result = mruby2erl(env, mrb, result);

  mrbc_context_free(mrb, cxt);
  mrb_close(mrb);

  enif_release_binary(&script_binary);


  return erl_result;
}

static ERL_NIF_TERM eval2(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]) {
  ErlNifBinary script_binary;

  if (!enif_inspect_binary(env, argv[0], &script_binary)){
    return enif_make_badarg(env);
  }

  if (!enif_is_list(env, argv[1])) {
    enif_release_binary(&script_binary);
    return enif_make_badarg(env);
  }


  mrb_state *mrb;
  mrbc_context *cxt;

  mrb = mrb_open();

  if (mrb == NULL) {
    return enif_make_atom(env, "error");
  }


  unsigned int mrb_argv_len;
  enif_get_list_length(env, argv[1], &mrb_argv_len);

  mrb_value mrb_argv = mrb_ary_new(mrb);
  ERL_NIF_TERM cur;
  for(cur = argv[1]; !enif_is_empty_list(env, cur); ) {
    ERL_NIF_TERM head, tail;
    enif_get_list_cell(env, cur, &head, &tail);
    mrb_ary_push(mrb, mrb_argv, erl2mruby(env, mrb, head));
    cur = tail;
  }

  mrb_define_global_const(mrb, "ARGV", mrb_argv);

  cxt = mrbc_context_new(mrb);

  char *script = malloc(script_binary.size+1);

  strncpy(script, (const char *)script_binary.data, (int)script_binary.size);
  script[script_binary.size] = '\0';

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
  {"eval", 1, eval1},
  {"eval", 2, eval2}
};

ERL_NIF_INIT(mruby,nif_funcs,NULL,NULL,NULL,NULL);
