let killGraceMs = 5000;

type outcome =
  | Exited(int)
  | Signaled(string)
  | TimedOut(int)
  | Aborted
  | Failed(string);

let describe = (argv, outcome) => {
  let command = ShellQuote.join(argv);
  switch (outcome) {
  | Exited(code) => Printf.sprintf("%s\n\nExit code: %d", command, code)
  | Signaled(signal) =>
    Printf.sprintf("%s\n\nTerminated by signal %s", command, signal)
  | TimedOut(seconds) =>
    Printf.sprintf("%s\n\nKilled after timeout (%ds)", command, seconds)
  | Aborted => Printf.sprintf("%s\n\nAborted by the user", command)
  | Failed(message) =>
    Printf.sprintf("%s\n\nFailed to start: %s", command, message)
  };
};

let isError = outcome =>
  switch (outcome) {
  | Exited(0) => false
  | _ => true
  };

let truncate = output => {
  let truncation =
    Pi.truncateTail(
      output,
      {
        maxLines: Pi.defaultMaxLines,
        maxBytes: Pi.defaultMaxBytes,
      },
    );
  if (truncation.truncated) {
    let file =
      Node.joinPath(
        Node.tmpdir(),
        "pi-test-run-" ++ Node.randomUUID() ++ ".log",
      );
    Node.writeFileSync(file, output);
    Printf.sprintf(
      "[Output truncated: last %d of %d lines. Full output saved to: %s]\n\n%s",
      truncation.outputLines,
      truncation.totalLines,
      file,
      truncation.content,
    );
  } else {
    truncation.content;
  };
};

let run = (~cwd, ~argv, ~timeout, ~signal): Js.Promise.t(Pi.result) =>
  Js.Promise.make((~resolve, ~reject as _) => {
    let chunks = ref([]);
    let finished = ref(false);
    let reason = ref(None);
    let timers = ref([]);
    let append = chunk => chunks := [chunk, ...chunks^];
    let finish = outcome =>
      if (! finished^) {
        finished := true;
        List.iter(Js.Global.clearTimeout, timers^);
        let output = String.concat("", List.rev(chunks^));
        let text = truncate(output) ++ "\n\n" ++ describe(argv, outcome);
        resolve(. {
          Pi.text,
          isError: isError(outcome),
        });
      };
    switch (argv) {
    | [] => finish(Failed("empty command"))
    | [program, ...args] =>
      let child =
        Node.spawn(
          program,
          Array.of_list(args),
          {
            cwd,
            detached: true,
            stdio: [|"ignore", "pipe", "pipe"|],
          },
        );
      let kill = signalName =>
        switch (Node.pid(child)) {
        | Some(pid) =>
          try(Node.killProcess(- pid, signalName)) {
          | _ => ()
          }
        | None => ()
        };
      let stop = why =>
        if (! finished^ && Option.is_none(reason^)) {
          reason := Some(why);
          kill("SIGTERM");
          timers :=
            [
              Js.Global.setTimeout(~f=() => kill("SIGKILL"), killGraceMs),
              ...timers^,
            ];
        };
      Node.setEncoding(Node.stdout(child), "utf8");
      Node.setEncoding(Node.stderr(child), "utf8");
      Node.onData(Node.stdout(child), append);
      Node.onData(Node.stderr(child), append);
      Node.onError(child, error =>
        finish(
          Failed(
            Option.value(Js.Exn.message(error), ~default="unknown error"),
          ),
        )
      );
      Node.onClose(child, (code, signalName) =>
        finish(
          switch (
            reason^,
            Js.Null.toOption(code),
            Js.Null.toOption(signalName),
          ) {
          | (Some(outcome), _, _) => outcome
          | (None, Some(code), _) => Exited(code)
          | (None, None, Some(signalName)) => Signaled(signalName)
          | (None, None, None) => Exited(-1)
          },
        )
      );
      timers :=
        [
          Js.Global.setTimeout(
            ~f=() => stop(TimedOut(timeout)),
            timeout * 1000,
          ),
          ...timers^,
        ];
      switch (signal) {
      | Some(signal) when Node.aborted(signal) => stop(Aborted)
      | Some(signal) => Node.onAbort(signal, () => stop(Aborted))
      | None => ()
      };
    };
  });
