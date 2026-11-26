open Jest;
open Expect;

describe("ShellQuote.quote", () => {
  test("wraps a plain value in single quotes", () =>
    expect(ShellQuote.quote("tests/test_a.py"))
    |> toEqual("'tests/test_a.py'")
  );

  test("neutralizes shell metacharacters", () =>
    expect(ShellQuote.quote("a; rm -rf ~ $(id) `id` | x"))
    |> toEqual("'a; rm -rf ~ $(id) `id` | x'")
  );

  test("escapes embedded single quotes", () =>
    expect(ShellQuote.quote("it's")) |> toEqual("'it'\\''s'")
  );

  test("quotes the empty string", () =>
    expect(ShellQuote.quote("")) |> toEqual("''")
  );
});

describe("ShellQuote.join", () =>
  test("quotes and joins every argument", () =>
    expect(ShellQuote.join(["pytest", "-k", "a or b"]))
    |> toEqual("'pytest' '-k' 'a or b'")
  )
);
