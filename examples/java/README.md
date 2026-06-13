# Nexa Java Examples

Requires: JDK 11+, `libnexa_jni.dylib` built from project root.

## Build

```bash
# From project root
cmake -B build -DNEXA_USE_STUB=OFF -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build

# Place JNI lib on java.library.path
export JAVA_LIBRARY_PATH=$(pwd)/build
```

## Run

```bash
cd examples/java

# One-shot, Standalone, Pool
java -Djava.library.path=../../build -cp ../../bindings/jni/java:. \
    io.nexa.examples.BasicExamples

# Module injection
java -Djava.library.path=../../build -cp ../../bindings/jni/java:. \
    io.nexa.examples.ModuleInjectionExample

# Load + Call pattern
java -Djava.library.path=../../build -cp ../../bindings/jni/java:. \
    io.nexa.examples.LoadAndCallExample
```

## Examples

| File | Demonstrates |
|------|-------------|
| `BasicExamples.java` | One-shot, Standalone, Pool, Presets |
| `ModuleInjectionExample.java` | @NexaExport, module registration, sync/async |
| `LoadAndCallExample.java` | load+call pattern, multi-function, hot reload |
