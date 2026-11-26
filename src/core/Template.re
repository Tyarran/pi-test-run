let withFlag = (flag, args) =>
  switch (flag) {
  | None => args
  | Some(flag) => [flag, ...args]
  };

let renderFragment = (fragment: Config.fragment, values) =>
  switch (values, fragment.join) {
  | ([], _) => []
  | (values, Some(separator)) =>
    withFlag(fragment.flag, [String.concat(separator, values)])
  | (values, None) =>
    List.concat_map(value => withFlag(fragment.flag, [value]), values)
  };

let render = (config: Config.t, targets: Targets.t) =>
  List.concat_map(
    token =>
      switch (token) {
      | "{paths}" => renderFragment(config.paths, targets.paths)
      | "{selectors}" => renderFragment(config.selectors, targets.selectors)
      | "{names}" => renderFragment(config.names, targets.names)
      | token => [token]
      },
    config.command,
  );
