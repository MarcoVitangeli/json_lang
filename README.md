# JSON Lang

Simple DSL to operate in JSON, usind [simdjson](https://github.com/simdjson/simdjson) library to do on-demand parsing of the document.

## Definitions

- `$` works as the root of the JSON object
- `.[propertyName]` for accessing a single property
- `[expression]` bracket expressions to operate in arrays (TODO)

examples:
- `json_lang input.json $.user.properties.name`
- `json_lang input.json $.user.properties.locations[.name == 'Buenos Aires']` (TODO)

Currently is in active development.
