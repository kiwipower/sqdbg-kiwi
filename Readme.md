SQDBG - fork
---
Resurrection of this quite old [debugger](http://wiki.squirrel-lang.org/mainsite/Wiki/default.aspx/SquirrelWiki/SQDBG.html) for squirrel [eclipse plugin](http://wiki.squirrel-lang.org/default.aspx/SquirrelWiki/SQDEV(2006-09-05-10-30-51.6840--218.164.50.26).html) ported to linux. It still works!

### Eclipse 
Requires [eclipse 3.2.0](https://archive.eclipse.org/eclipse/downloads/drops/R-3.2-200606291905/) 
Requires java < 11. I have tested with java 5 downloadable [here](https://www.oracle.com/uk/java/technologies/java-archive-javase5-downloads.html)
Change your PATH to include java 5 (setting JAVA_HOME, JDK_HOME, JRE_HOME does not seem to work)
```bash
export PATH=<path-to-install>/jdk1.5.0_22/bin/:$PATH
./eclipse &
```

### Eclipse plugin
The plugin binaries can be downloaded [here](http://wiki.squirrel-lang.org/files/sqdev_September_06_2006.zip)
unpack sqdev in <eclipse root>/plugins/

### Integrating with squirrel vm
Sorry - make has no install as yet. You can either add this as a git submodule then build as part of your project
```cmake
include_directories(
        ..
        sqdbg-kiwi
)

...

add_executable(proj sqdbg-kiwi/serialize_state.inl sqdbg-kiwi/sqrdbg.h sqdbg-kiwi/sqrdbg.cpp sqdbg-kiwi/sqdbgserver.h sqdbg-kiwi/sqdbgserver.cpp)

```
Or build libsqdbg.a and use the header files directly.

To integrate with your application - you need to do the following :
```c++
  sq_enabledebuginfo(vm, 1);
  HSQREMOTEDBG dbg = sq_rdbg_init(vm, 1234, false);
  sq_rdbg_waitforconnections(dbg);
```

The debugger will only trigger breakpoints if the path to the file in eclipse matches that in your applciation
