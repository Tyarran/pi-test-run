type params = {
  paths: option(array(string)),
  selectors: option(array(string)),
  names: option(array(string)),
  timeout: int,
};

let description = "Run the project's unit tests with the command configured under the pi-test-run settings key. Targets are typed: paths (files or directories inside the project), selectors (e.g. tests/test_a.py::TestA::test_b) and names (name filters). The timeout in seconds is required.";

let parameters =
  TypeBox.object_(
    Js.Dict.fromList([
      (
        "paths",
        TypeBox.optional(
          TypeBox.array(
            TypeBox.string_({
              "description": "File or directory path relative to the project",
            }),
            { "description": "Files or directories to test" },
          ),
        ),
      ),
      (
        "selectors",
        TypeBox.optional(
          TypeBox.array(
            TypeBox.string_({
              "description": "Test selector, e.g. tests/test_a.py::TestA::test_b",
            }),
            { "description": "Individual tests to run" },
          ),
        ),
      ),
      (
        "names",
        TypeBox.optional(
          TypeBox.array(
            TypeBox.string_({ "description": "Test name filter" }),
            {
              "description": "Name filters combined by the project configuration",
            },
          ),
        ),
      ),
      (
        "timeout",
        TypeBox.integer({
          "description": "Timeout in seconds",
          "minimum": 1,
        }),
      ),
    ]),
    { "additionalProperties": false },
  );

let targetsOf = (params): Targets.t => {
  let list = values => Option.fold(~none=[], ~some=Array.to_list, values);
  {
    paths: list(params.paths),
    selectors: list(params.selectors),
    names: list(params.names),
  };
};

let toolResult = (result: Pi.result) => {
  "content": [|
    {
      "type": "text",
      "text": result.text,
    },
  |],
  "details": Js.Undefined.empty,
  "isError": result.isError,
};

let failure = text =>
  Js.Promise.resolve({
    Pi.text,
    isError: true,
  });

let execute = (pi, params, signal, ctx: Pi.toolContext) => {
  let pathExists = path => Node.existsSync(Node.resolvePath(ctx.cwd, path));
  switch (
    Config.decode(Js.Dict.get(Pi.getSettings(pi), "pi-test-run")),
    Targets.validate(~pathExists, targetsOf(params)),
  ) {
  | (Error(message), _)
  | (_, Error(message)) => failure(message)
  | (Ok(config), Ok(targets)) =>
    let argv = Template.render(config, targets);
    switch (config.mode) {
    | Sync =>
      SyncRunner.run(~cwd=ctx.cwd, ~argv, ~timeout=params.timeout, ~signal)
    | Background =>
      BackgroundRunner.run(
        ~events=Pi.events(pi),
        ~argv,
        ~timeout=params.timeout,
      )
    };
  };
};

let activate = pi =>
  Pi.registerTool(
    pi,
    {
      "name": "test_run",
      "label": "Test Run",
      "description": description,
      "promptSnippet": "Run the project's unit tests (paths, selectors, names, timeout)",
      "promptGuidelines": [|
        "Use test_run to run unit tests; pass typed targets (paths, selectors, names) and a timeout in seconds.",
      |],
      "parameters": parameters,
      "execute": (_toolCallId, params, signal, _onUpdate, ctx) =>
        execute(pi, params, Js.Nullable.toOption(signal), ctx)
        |> Js.Promise.then_(result => Js.Promise.resolve(toolResult(result))),
    },
  );
