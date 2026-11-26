open Jest;
open Expect;

let decode = text => Config.decode(Some(Js.Json.parseExn(text)));

let message = result =>
  switch (result) {
  | Ok(_) => "ok"
  | Error(message) => message
  };

describe("Config.decode", () => {
  test("decodes a full configuration", () =>
    expect(
      decode(
        {|{"mode": "background", "command": "pytest {paths} {selectors} {names}", "names": {"flag": "-k", "join": " or "}}|},
      ),
    )
    |> toEqual(
         Ok(
           {
             mode: Background,
             command: ["pytest", "{paths}", "{selectors}", "{names}"],
             paths: Config.emptyFragment,
             selectors: Config.emptyFragment,
             names: {
               flag: Some("-k"),
               join: Some(" or "),
             },
           }: Config.t,
         ),
       )
  );

  test("defaults to sync mode", () =>
    expect(
      Result.map(
        (config: Config.t) => config.mode,
        decode({|{"command": "vitest run"}|}),
      ),
    )
    |> toEqual(Ok(Config.Sync))
  );

  test("splits the command on spaces and tabs", () =>
    expect(
      Result.map(
        (config: Config.t) => config.command,
        decode({|{"command": "  npx\tvitest   run {paths} "}|}),
      ),
    )
    |> toEqual(Ok(["npx", "vitest", "run", "{paths}"]))
  );

  test("reports a missing configuration", () =>
    expect(message(Config.decode(None)))
    |> toEqual("No pi-test-run.command configured for this project")
  );

  test("reports a missing command", () =>
    expect(message(decode({|{"mode": "sync"}|})))
    |> toEqual("No pi-test-run.command configured for this project")
  );

  testAll(
    "rejects invalid configurations",
    [
      {|"pytest"|},
      {|{"command": ""}|},
      {|{"command": 42}|},
      {|{"command": "pytest", "mode": "async"}|},
      {|{"command": "pytest", "names": "-k"}|},
      {|{"command": "pytest", "names": {"flag": 1}}|},
      {|{"command": "pytest --files={paths}"}|},
    ],
    text =>
    expect(Result.is_error(decode(text))) |> toBe(true)
  );
});
