all: compile

deps/mruby/.git:
	git submodule init
	git submodule update

deps/mruby/build/host/lib/libmruby.a:
	cd deps/mruby && make

compile: deps/mruby/build/host/lib/libmruby.a
	./rebar compile

clean:
	cd deps/mruby && make clean
	./rebar clean
