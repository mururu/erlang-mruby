# erlang-mruby

WIP

## BUILD

```
make
```

## Examples

```erlang
1> mruby:eval(<<"[1,2,3].map{|i| i+1}">>).
[2,3,4]
2> mruby:eval(<<"ARGV[0]+ARGV[1]">>, [<<"a">>,<<"b">>]).
<<"ab">>
```


* 1 <=> 1
* 1.0 <=> 1.0
* abc <=> :abc
* true <=> true
* false <=> false
* nil <=> nil
* <<"abc">> <=> "abc"
* [1,2,3] <=> [1,2,3]
* {[{a, 1}, {b, 2}]} <=> {a: 1, b: 2}

## LICENSE

MIT
