type api;
type events;
type toolContext = {cwd: string};

type truncateOptions = {
  maxLines: int,
  maxBytes: int,
};

type truncation = {
  content: string,
  truncated: bool,
  outputLines: int,
  totalLines: int,
};

[@mel.send] external getSettings: api => Js.Dict.t(Js.Json.t) = "getSettings";
[@mel.get] external events: api => events = "events";

type unsubscribe;

[@mel.send]
external on: (events, string, Js.Json.t => unit) => unsubscribe = "on";

[@mel.send]
external unsubscribe: (unsubscribe, [@mel.as {json|null|json}] _) => unit =
  "call";

[@mel.send] external emit: (events, string, Js.Json.t) => unit = "emit";

[@mel.send] external registerTool: (api, Js.t({..})) => unit = "registerTool";

[@mel.module "@earendil-works/pi-coding-agent"]
external truncateTail: (string, truncateOptions) => truncation =
  "truncateTail";

[@mel.module "@earendil-works/pi-coding-agent"]
external defaultMaxLines: int = "DEFAULT_MAX_LINES";

[@mel.module "@earendil-works/pi-coding-agent"]
external defaultMaxBytes: int = "DEFAULT_MAX_BYTES";

type result = {
  text: string,
  isError: bool,
};
