
debug:
	$(MAKE) BUILDTYPE=Debug -C src all
	$(MAKE) BUILDTYPE=Debug -C ccore all
	$(MAKE) BUILDTYPE=Debug -C test/unittests all

release:
	$(MAKE) BUILDTYPE=Release -C src all
	$(MAKE) BUILDTYPE=Release -C ccore all
	$(MAKE) BUILDTYPE=Release -C test/unittests all

profile:
	$(MAKE) BUILDTYPE=Profile -C src all
	$(MAKE) BUILDTYPE=Profile -C ccore all
	$(MAKE) BUILDTYPE=Profile -C test/unittests all

depend format clean:
	$(MAKE) -C src $@
	$(MAKE) -C ccore $@
	$(MAKE) -C test/unittests $@

distclean: clean
	rm -f make.conf

testpopcnt: all
	build/ccore --debuglog --logfile=/tmp/ccore.log -n 2000000000 testpopcnt

