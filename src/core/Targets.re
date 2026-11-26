type t = {
  paths: list(string),
  selectors: list(string),
  names: list(string),
};

let empty = {
  paths: [],
  selectors: [],
  names: [],
};

let startsWithDash = value => String.length(value) > 0 && value.[0] == '-';

let escapesProject = path =>
  String.length(path) > 0
  && path.[0] == '/'
  || String.split_on_char('/', path)
  |> List.exists(segment => segment == "..");

let checkValue = (kind, value) =>
  if (value == "") {
    Error(Printf.sprintf("Empty value in %s", kind));
  } else if (startsWithDash(value)) {
    Error(
      Printf.sprintf("Value %S in %s must not start with '-'", value, kind),
    );
  } else {
    Ok();
  };

let checkPath = (~pathExists, path) =>
  if (escapesProject(path)) {
    Error(Printf.sprintf("Path %S must be relative to the project", path));
  } else if (!pathExists(path)) {
    Error(Printf.sprintf("Path %S does not exist in the project", path));
  } else {
    Ok();
  };

let rec firstError = checks =>
  switch (checks) {
  | [] => Ok()
  | [check, ...rest] =>
    switch (check()) {
    | Ok () => firstError(rest)
    | Error(_) as error => error
    }
  };

let validate = (~pathExists, targets) => {
  let valueChecks = (kind, values) =>
    List.map((value, ()) => checkValue(kind, value), values);
  let pathChecks =
    List.map((path, ()) => checkPath(~pathExists, path), targets.paths);
  switch (
    firstError(
      List.concat([
        valueChecks("paths", targets.paths),
        valueChecks("selectors", targets.selectors),
        valueChecks("names", targets.names),
        pathChecks,
      ]),
    )
  ) {
  | Ok () => Ok(targets)
  | Error(_) as error => error
  };
};
