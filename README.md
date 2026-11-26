# pi-test-run

A [pi](https://pi.dev) extension that provides a dedicated `test_run` tool to run unit tests.

It is designed for sessions where the agent has **no `bash` and no `bg_run`** (for example with a `bash-toggle` extension). `test_run` then becomes the only way to run tests: the agent passes typed targets, and the command comes from the project configuration.

## How it works

- The agent calls `test_run` with typed targets (`paths`, `selectors`, `names`) and a required `timeout`.
- Targets are validated: no value may start with `-`, and every path must exist inside the project.
- The configured command template is rendered into an argument list.
- Depending on the configured `mode`, the command runs:
  - `sync`: spawned directly by the extension, **without a shell**, one argument per value;
  - `background`: through the [`pi-background-tasks`](https://github.com/ismailsaleekh/pi-background-tasks) EventBus API, with every argument shell-quoted. A notification arrives when the run completes.

## Installation

Requirements: [mise](https://mise.jdx.dev) (or opam 2 and Node.js ≥ 22.19 installed manually).

```sh
git clone https://github.com/Tyarran/pi-test-run.git ~/Projects/pi-test-run
cd ~/Projects/pi-test-run
mise install
opam init -a --bare            # first opam use only
opam switch create . 5.5.0 --deps-only --with-test -y
eval $(opam env)
npm ci
dune build
```

Register the extension in `~/.pi/agent/settings.json`:

```json
{
  "extensions": ["~/Projects/pi-test-run"]
}
```

Then run `/reload` in pi. Rebuild with `dune build` and `/reload` after each change.

The `background` mode also requires the `pi-background-tasks` extension:

```sh
pi install npm:pi-background-tasks
```

## Configuration

`test_run` reads the `pi-test-run` key from pi's settings (`pi.getSettings()`):

- `~/.pi/agent/settings.json`: shared defaults (`mode`, fragments);
- `<project>/.pi/settings.json`: the project `command` and overrides (loaded once the project is trusted).

Project settings are deep-merged over global settings.

| Key | Type | Default | Description |
|---|---|---|---|
| `mode` | `"sync"` \| `"background"` | `"sync"` | How the command runs. The agent cannot change it. |
| `command` | string | — (required) | Command template, split on whitespace. |
| `paths` | fragment | `{}` | How `{paths}` is rendered. |
| `selectors` | fragment | `{}` | How `{selectors}` is rendered. |
| `names` | fragment | `{}` | How `{names}` is rendered. |

The `command` may contain the placeholders `{paths}`, `{selectors}` and `{names}`. Each placeholder must be a standalone token (`--files={paths}` is rejected).

A fragment is `{ "flag"?: string, "join"?: string }`:

- no value for a target type: its placeholder is removed;
- `join` set: values are joined into a single argument, preceded by `flag` if set;
- no `join`: one argument per value, each preceded by `flag` if set.

### pytest

```json
{
  "pi-test-run": {
    "mode": "background",
    "command": "pytest {paths} {selectors} {names}",
    "names": { "flag": "-k", "join": " or " }
  }
}
```

`names: ["login", "logout"]` renders `pytest -k "login or logout"`.

### vitest

```json
{
  "pi-test-run": {
    "command": "npx vitest run {paths} {names}",
    "names": { "flag": "-t", "join": "|" }
  }
}
```

## Tool reference

`test_run` parameters:

| Name | Type | Required | Description |
|---|---|---|---|
| `paths` | `string[]` | no | Files or directories relative to the project |
| `selectors` | `string[]` | no | Test selectors, e.g. `tests/test_a.py::TestA::test_b` |
| `names` | `string[]` | no | Name filters |
| `timeout` | integer (seconds, ≥ 1) | yes | Maximum run time |

Results:

- `sync`: raw stdout and stderr, followed by the command and its exit code. Output is truncated to its last 2000 lines / 50 KB (pi's `truncateTail`); the full output is then saved to a temporary file whose path is given. On timeout or abort (`Esc`), the process group receives `SIGTERM`, then `SIGKILL` 5 seconds later. A non-zero exit code, a timeout or an abort is reported as a tool error.
- `background`: the background task id. `pi-background-tasks` delivers the completion notification.

Explicit errors are returned when no `command` is configured, when the configuration is invalid, when a target is invalid, or when `pi-background-tasks` does not answer in `background` mode.

## Security

`test_run` is a usage guardrail, not a security boundary. An agent able to edit `.pi/settings.json` can change the command. This risk is accepted by design.

## Development

The extension is written in [ReasonML](https://reasonml.github.io) and compiled to ES modules by [Melange](https://melange.re). `index.js` is the only JavaScript source: it re-exports the compiled `activate` function.

```
src/core/        pure logic: Config, Targets, Template, ShellQuote
src/extension/   bindings (Pi, Node, TypeBox) and runners
test/            melange-jest unit tests of the pure logic
```

```sh
dune build                    # build
dune build @fmt --auto-promote # format
npm test                      # build and run the tests
```

## License

[MIT](LICENSE)
