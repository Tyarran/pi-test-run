open Jest;
open Expect;

let config =
    (
      ~command,
      ~paths=Config.emptyFragment,
      ~selectors=Config.emptyFragment,
      ~names=Config.emptyFragment,
      (),
    )
    : Config.t => {
  mode: Sync,
  command,
  paths,
  selectors,
  names,
};

let pytest =
  config(
    ~command=["pytest", "-q", "{paths}", "{selectors}", "{names}"],
    ~names={
      flag: Some("-k"),
      join: Some(" or "),
    },
    (),
  );

let targets = (~paths=[], ~selectors=[], ~names=[], ()): Targets.t => {
  paths,
  selectors,
  names,
};

describe("Template.render", () => {
  test("removes placeholders without values", () =>
    expect(Template.render(pytest, Targets.empty))
    |> toEqual(["pytest", "-q"])
  );

  test("passes one argument per path and selector", () =>
    expect(
      Template.render(
        pytest,
        targets(
          ~paths=["tests/a.py", "tests/b"],
          ~selectors=["tests/c.py::test_d"],
          (),
        ),
      ),
    )
    |> toEqual([
         "pytest",
         "-q",
         "tests/a.py",
         "tests/b",
         "tests/c.py::test_d",
       ])
  );

  test("joins names behind a single flag", () =>
    expect(
      Template.render(pytest, targets(~names=["login", "logout"], ())),
    )
    |> toEqual(["pytest", "-q", "-k", "login or logout"])
  );

  test("repeats the flag for every value without join", () =>
    expect(
      Template.render(
        config(
          ~command=["vitest", "run", "{names}"],
          ~names={
            flag: Some("-t"),
            join: None,
          },
          (),
        ),
        targets(~names=["a", "b"], ()),
      ),
    )
    |> toEqual(["vitest", "run", "-t", "a", "-t", "b"])
  );

  test("joins values without a flag", () =>
    expect(
      Template.render(
        config(
          ~command=["runner", "{names}"],
          ~names={
            flag: None,
            join: Some("|"),
          },
          (),
        ),
        targets(~names=["a", "b"], ()),
      ),
    )
    |> toEqual(["runner", "a|b"])
  );

  test("keeps a value with spaces as a single argument", () =>
    expect(
      Template.render(pytest, targets(~names=["a or b; rm -rf ~"], ())),
    )
    |> toEqual(["pytest", "-q", "-k", "a or b; rm -rf ~"])
  );
});
