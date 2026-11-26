open Jest;
open Expect;

let existing = ["tests", "tests/test_a.py"];
let pathExists = path => List.mem(path, existing);
let validate = Targets.validate(~pathExists);

let targets = (~paths=[], ~selectors=[], ~names=[], ()): Targets.t => {
  paths,
  selectors,
  names,
};

describe("Targets.validate", () => {
  test("accepts empty targets", () =>
    expect(validate(Targets.empty)) |> toEqual(Ok(Targets.empty))
  );

  test("accepts existing paths, selectors and names", () => {
    let input =
      targets(
        ~paths=["tests", "tests/test_a.py"],
        ~selectors=["tests/test_a.py::TestA::test_b"],
        ~names=["login and not slow"],
        (),
      );
    expect(validate(input)) |> toEqual(Ok(input));
  });

  testAll(
    "rejects values starting with a dash",
    [
      targets(~paths=["-p"], ()),
      targets(~selectors=["--rootdir=/"], ()),
      targets(~names=["-x"], ()),
    ],
    input =>
    expect(Result.is_error(validate(input))) |> toBe(true)
  );

  test("rejects empty values", () =>
    expect(Result.is_error(validate(targets(~names=[""], ()))))
    |> toBe(true)
  );

  test("rejects missing paths", () =>
    expect(validate(targets(~paths=["tests/missing.py"], ())))
    |> toEqual(
         Error("Path \"tests/missing.py\" does not exist in the project"),
       )
  );

  testAll(
    "rejects paths outside the project",
    ["/etc/passwd", "../other", "tests/../../x"],
    path =>
    expect(Result.is_error(validate(targets(~paths=[path], ()))))
    |> toBe(true)
  );
});
