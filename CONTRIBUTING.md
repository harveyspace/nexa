# Contributing to Nexa

Thanks for your interest in contributing!

## Getting Started

```bash
git clone https://github.com/nexa/nexa.git && cd nexa
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
```

Requires: CMake 3.20+, GCC 11+ or Clang 15+, JDK 11+.

## Development Workflow

1. Fork the repository.
2. Create a feature branch.
3. Make your changes. Add tests.
4. Run the test suite:

```bash
cd build && ctest --output-on-failure
```

5. Submit a pull request.

## Code Style

- C++: Google C++ Style Guide. `clang-format` config provided.
- Java: Google Java Format.
- Commit messages: conventional commits (`feat:`, `fix:`, `docs:`, `chore:`).

## Testing

- Unit tests: Google Test (C++), JUnit 5 (Java).
- Integration tests: spawn real V8 child processes, verify IPC.
- Benchmarks: JMH for Java API, Google Benchmark for C API.

## Pull Request Checklist

- [ ] Tests pass locally.
- [ ] New code has corresponding tests.
- [ ] Public API changes are documented.
- [ ] No breaking changes to stable C API without prior discussion.

## Reporting Bugs

Open an issue with:
- Nexa version, OS, JDK version.
- Minimal reproduction script.
- Stack trace or crash dump if applicable.

## Security Issues

See [SECURITY.md](SECURITY.md). Do not open public issues for vulnerabilities.
