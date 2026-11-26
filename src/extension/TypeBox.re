type schema;

[@mel.module "typebox"] [@mel.scope "Type"]
external string_: {. "description": string } => schema = "String";

[@mel.module "typebox"] [@mel.scope "Type"]
external integer:
  {
    .
    "description": string,
    "minimum": int,
  } =>
  schema =
  "Integer";

[@mel.module "typebox"] [@mel.scope "Type"]
external array: (schema, {. "description": string }) => schema = "Array";

[@mel.module "typebox"] [@mel.scope "Type"]
external optional: schema => schema = "Optional";

[@mel.module "typebox"] [@mel.scope "Type"]
external object_:
  (Js.Dict.t(schema), {. "additionalProperties": bool }) => schema =
  "Object";
