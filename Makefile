all: compile

deps/mruby/.git:
	git submodule init
	git submodule update

deps/mruby/build/host/lib/libmruby.a: deps/mruby/.git
	cd deps/mruby && make

compile: deps/mruby/build/host/lib/libmruby.a
	./rebar compile

clean:
	cd deps/mruby && make clean
	rm -f c_src/mruby.o
	rm -f priv/mruby.so
	./rebar clean
