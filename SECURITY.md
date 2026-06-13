# Security Policy

## Supported Versions

| Version | Supported |
|---------|:---------:|
| 1.x     | ✅        |
| 0.x     | ❌        |

## Reporting a Vulnerability

**Do not open a public issue.** Send details to `security@nexa.dev`.

Include:
- Affected version(s).
- Steps to reproduce.
- Impact assessment.

We will acknowledge within 48 hours and aim to release a fix within 7 days.

## Security Model

Nexa runs user-supplied JavaScript in isolated child processes:

- **Process isolation**: A native crash (SIGSEGV) or infinite loop in JS terminates only the child process, not the JVM.
- **Memory isolation**: Each child process has a hard memory cap (`perProcessMemoryMb`). OOM kills only that process.
- **Execution timeout**: Scripts are terminated after a configurable timeout.
- **Stack limit**: Configurable `stackSizeKb` prevents stack overflow from crashing the process.
- **Sandbox options**: `eval()` can be disabled, `require()` can be allowlisted, `fs` can be jailed, network modules can be disabled.

## Scope

Nexa itself (C++ core, JNI bindings, Java API) is in scope. User-registered `@NexaExport` modules are the responsibility of the user. npm packages loaded via `require()` are the responsibility of their respective authors.
