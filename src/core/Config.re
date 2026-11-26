type mode =
  | Sync
  | Background;

type fragment = {
  flag: option(string),
  join: option(string),
};

type t = {
  mode,
  command: list(string),
  paths: fragment,
  selectors: fragment,
  names: fragment,
};

let placeholders = ["{paths}", "{selectors}", "{names}"];

let emptyFragment = {
  flag: None,
  join: None,
};

let contains = (~sub, text) => {
  let subLength = String.length(sub);
  let rec loop = index =>
    index
    + subLength <= String.length(text)
    && (String.sub(text, index, subLength) == sub || loop(index + 1));
  loop(0);
};

let tokenize = command =>
  String.split_on_char(' ', command)
  |> List.concat_map(String.split_on_char('\t'))
  |> List.filter(token => token != "");

let misplacedPlaceholder = token =>
  !List.mem(token, placeholders)
  && List.exists(
       placeholder => contains(~sub=placeholder, token),
       placeholders,
     );

let field = (dict, key) => Js.Dict.get(dict, key);

let decodeString = (name, json) =>
  switch (Js.Json.classify(json)) {
  | JSONString(value) => Ok(value)
  | _ => Error(Printf.sprintf("pi-test-run.%s must be a string", name))
  };

let decodeOptionalString = (name, dict, key) =>
  switch (field(dict, key)) {
  | None => Ok(None)
  | Some(json) =>
    switch (decodeString(name ++ "." ++ key, json)) {
    | Ok(value) => Ok(Some(value))
    | Error(_) as error => error
    }
  };

let decodeFragment = (dict, key) =>
  switch (field(dict, key)) {
  | None => Ok(emptyFragment)
  | Some(json) =>
    switch (Js.Json.classify(json)) {
    | JSONObject(fragment) =>
      switch (
        decodeOptionalString(key, fragment, "flag"),
        decodeOptionalString(key, fragment, "join"),
      ) {
      | (Ok(flag), Ok(join)) =>
        Ok({
          flag,
          join,
        })
      | (Error(message), _)
      | (_, Error(message)) => Error(message)
      }
    | _ => Error(Printf.sprintf("pi-test-run.%s must be an object", key))
    }
  };

let decodeMode = dict =>
  switch (field(dict, "mode")) {
  | None => Ok(Sync)
  | Some(json) =>
    switch (Js.Json.classify(json)) {
    | JSONString("sync") => Ok(Sync)
    | JSONString("background") => Ok(Background)
    | _ => Error("pi-test-run.mode must be \"sync\" or \"background\"")
    }
  };

let decodeCommand = dict =>
  switch (field(dict, "command")) {
  | None => Error("No pi-test-run.command configured for this project")
  | Some(json) =>
    switch (decodeString("command", json)) {
    | Error(_) as error => error
    | Ok(command) =>
      switch (tokenize(command)) {
      | [] => Error("pi-test-run.command is empty")
      | tokens when List.exists(misplacedPlaceholder, tokens) =>
        Error(
          "pi-test-run.command placeholders must be standalone tokens: {paths}, {selectors}, {names}",
        )
      | tokens => Ok(tokens)
      }
    }
  };

let decode = (json: option(Js.Json.t)) =>
  switch (Option.map(Js.Json.classify, json)) {
  | None => Error("No pi-test-run.command configured for this project")
  | Some(JSONObject(dict)) =>
    switch (
      decodeMode(dict),
      decodeCommand(dict),
      decodeFragment(dict, "paths"),
      decodeFragment(dict, "selectors"),
      decodeFragment(dict, "names"),
    ) {
    | (Ok(mode), Ok(command), Ok(paths), Ok(selectors), Ok(names)) =>
      Ok({
        mode,
        command,
        paths,
        selectors,
        names,
      })
    | (Error(message), _, _, _, _)
    | (_, Error(message), _, _, _)
    | (_, _, Error(message), _, _)
    | (_, _, _, Error(message), _)
    | (_, _, _, _, Error(message)) => Error(message)
    }
  | Some(_) => Error("pi-test-run must be an object")
  };
