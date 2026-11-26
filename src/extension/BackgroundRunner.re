let requestChannel = "pi-background-tasks:request:v1";
let responseChannel = "pi-background-tasks:response:v1";
let responseTimeoutMs = 5000;

let field = (json, key) =>
  switch (Js.Json.classify(json)) {
  | JSONObject(dict) => Js.Dict.get(dict, key)
  | _ => None
  };

let stringField = (json, key) =>
  Option.bind(field(json, key), Js.Json.decodeString);

let request = (events, operation, payload) =>
  Js.Promise.make((~resolve, ~reject as _) => {
    let requestId = Node.randomUUID();
    let subscription = ref(None);
    let timer = ref(None);
    let settle = result => {
      Option.iter(Pi.unsubscribe, subscription^);
      Option.iter(Js.Global.clearTimeout, timer^);
      resolve(. result);
    };
    subscription :=
      Some(
        Pi.on(events, responseChannel, frame =>
          if (stringField(frame, "request_id") == Some(requestId)) {
            switch (Option.bind(field(frame, "ok"), Js.Json.decodeBoolean)) {
            | Some(true) =>
              settle(
                Ok(
                  Option.value(
                    field(frame, "result"),
                    ~default=Js.Json.null,
                  ),
                ),
              )
            | _ =>
              settle(
                Error(
                  Option.value(
                    stringField(frame, "error"),
                    ~default="pi-background-tasks rejected the request",
                  ),
                ),
              )
            };
          }
        ),
      );
    timer :=
      Some(
        Js.Global.setTimeout(
          ~f=
            () =>
              settle(
                Error(
                  "pi-background-tasks did not answer: is the extension installed and loaded?",
                ),
              ),
          responseTimeoutMs,
        ),
      );
    Pi.emit(
      events,
      requestChannel,
      Js.Json.object_(
        Js.Dict.fromList([
          (
            "schema_version",
            Js.Json.string("pi-background-tasks.extension-request.v1"),
          ),
          ("request_id", Js.Json.string(requestId)),
          ("operation", Js.Json.string(operation)),
          ("payload", payload),
        ]),
      ),
    );
  });

let runPayload = (~command, ~timeout) =>
  Js.Json.object_(
    Js.Dict.fromList([
      ("name", Js.Json.string("test_run")),
      ("command", Js.Json.string(command)),
      ("isAgent", Js.Json.boolean(false)),
      ("notifyOnCompletion", Js.Json.boolean(true)),
      ("triggerOnCompletion", Js.Json.boolean(true)),
      ("timeoutSeconds", Js.Json.number(float_of_int(timeout))),
    ]),
  );

let failure = text => {
  Pi.text,
  isError: true,
};

let run = (~events, ~argv, ~timeout): Js.Promise.t(Pi.result) => {
  let command = ShellQuote.join(argv);
  request(events, "capabilities", Js.Json.object_(Js.Dict.empty()))
  |> Js.Promise.then_(capabilities =>
       switch (capabilities) {
       | Error(message) => Js.Promise.resolve(Error(message))
       | Ok(capabilities) =>
         switch (
           Option.bind(field(capabilities, "run"), Js.Json.decodeBoolean)
         ) {
         | Some(true) =>
           request(events, "run", runPayload(~command, ~timeout))
         | _ =>
           Js.Promise.resolve(
             Error("pi-background-tasks does not support the run operation"),
           )
         }
       }
     )
  |> Js.Promise.then_(response =>
       Js.Promise.resolve(
         switch (response) {
         | Error(message) =>
           failure("Background test run failed: " ++ message)
         | Ok(task) => {
             Pi.text:
               Printf.sprintf(
                 "Started background test run %s (timeout %ds):\n%s\n\nA notification will arrive when it completes.",
                 Option.value(stringField(task, "id"), ~default="?"),
                 timeout,
                 command,
               ),
             isError: false,
           }
         },
       )
     );
};
