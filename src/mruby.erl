-module(mruby).
-export([eval/1]).
-on_load(init/0).

eval(_) ->
    erlang:nif_error({nif_not_loaded, {module, ?MODULE}, {line, ?LINE}}).

init() ->
    PrivDir = case code:priv_dir(?MODULE) of
        {error, bad_name} ->
            case filelib:is_dir(filename:join(["..", "priv"])) of
              true ->
                filename:join(["..", "priv"]);
              _ ->
                "priv"
            end;
        Dir ->
            Dir
    end,
    erlang:load_nif(filename:join(PrivDir, "mruby"), 0).
