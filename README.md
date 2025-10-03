## Debian 10 compilation


### How to edit and test

Go to ns2/queue, create symlink to the files

```C++
ln -s ~/workspace/gearbox/multi-level-gearbox/gearbox-one-level.cc .
ln -s ~/workspace/gearbox/multi-level-gearbox/gearbox-one-level.h .
```

Make and run test

```bash
# under ns-allinone-2.34/ns-2.34
make clean && make -j8
```

```bash
# under the project root directory
ns tcp-HRCC.tcl 100 0.5 Topology-8hosts-NSDI21.tcl
```

## Improvements

* Code style
    * All class variables are named with lowercase + _
    * All files are named with lowercase + _

* Performance
    * Use int map for FlowMap

* Remove excess log
    * Some log are extra, check "puts" in Hyline.patch and remove them from ns-link.tcl