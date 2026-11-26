type childProcess;
type stream;
type abortSignal;

type spawnOptions = {
  cwd: string,
  detached: bool,
  stdio: array(string),
};

[@mel.module "node:child_process"]
external spawn: (string, array(string), spawnOptions) => childProcess =
  "spawn";

[@mel.get] external pid: childProcess => option(int) = "pid";
[@mel.get] external stdout: childProcess => stream = "stdout";
[@mel.get] external stderr: childProcess => stream = "stderr";

[@mel.send]
external onData: (stream, [@mel.as "data"] _, Js.String.t => unit) => unit =
  "on";

[@mel.send]
external onClose:
  (
    childProcess,
    [@mel.as "close"] _,
    (Js.Null.t(int), Js.Null.t(string)) => unit
  ) =>
  unit =
  "on";

[@mel.send]
external onError: (childProcess, [@mel.as "error"] _, Js.Exn.t => unit) => unit =
  "on";

[@mel.send] external setEncoding: (stream, string) => unit = "setEncoding";

[@mel.scope "process"] external killProcess: (int, string) => unit = "kill";

[@mel.get] external aborted: abortSignal => bool = "aborted";

[@mel.send]
external onAbort: (abortSignal, [@mel.as "abort"] _, unit => unit) => unit =
  "addEventListener";

[@mel.module "node:fs"] external existsSync: string => bool = "existsSync";

[@mel.module "node:fs"]
external writeFileSync: (string, string) => unit = "writeFileSync";

[@mel.module "node:path"]
external resolvePath: (string, string) => string = "resolve";

[@mel.module "node:path"]
external joinPath: (string, string) => string = "join";

[@mel.module "node:os"] external tmpdir: unit => string = "tmpdir";

[@mel.module "node:crypto"] external randomUUID: unit => string = "randomUUID";
