# docgen

A lightweight cross-language documentation generator that parses structured comments into machine-readable JSON.

## Rationale

See [RATIONALE.md](RATIONALE.md) for design decisions and implementation details.

## Building

```bash
cc nob.c -o nob
./nob # build main.c into build/docgen
```

# Example

## C

```c
// <@
// @name add
// @desc Adds two numbers together
// Works on only integers
// @param x The first number
// @param y The second number
// @return The sum of x and y
int add(int x, int y) {
// @>
  return x + y;
}
```

## Lua

```lua
-- <@
-- @name add
-- @desc Adds two numbers together
-- @param x The first number
-- @param y The second number
-- @return The sum of x and y
function add(x, y)
-- @>
  return x + y
end
```

# Generated Output

```json
[
  {
    "name": "add",
    "description": "Adds two numbers together\nWorks on only integers",
    "header": "int add(int x, int y) {",
    "return": "The sum of x and y",
    "location": {
      "file": "main.c",
      "line": 249
    },
    "params": [
      {
        "name": "x",
        "description": "The first number"
      },
      {
        "name": "y",
        "description": "The second number"
      }
    ]
  }
]
```

## Language-Agnostic by Design

The parser intentionally does not understand language semantics.

It only understands:

* structured metadata tags,
* comment delimiters,
* and raw source text.

That allows the same format to work across many languages without requiring compiler infrastructure or AST parsing.

# Syntax

Documentation blocks begin with:

```text
<@
```

and end with:

```text
@>
```

Inside the block:

* comment-prefixed lines become structured metadata
* non-comment lines become preserved header/signature content

Example:

```c
// <@
// @name add
// @desc Adds two numbers together
int add(int x, int y) {
// @>
```

# Supported Tags

| Tag           | Description              |
| ------------- | ------------------------ |
| `@name`       | Symbol name              |
| `@desc`       | Description              |
| `@param`      | Function parameter       |
| `@field`      | Struct/table field       |
| `@return`     | Return value description |
| `@module`     | Module name              |
| `@namespace`  | Namespace                |
| `@kind`       | Symbol kind              |
| `@note`       | Additional notes         |
| `@warning`    | Warning text             |
| `@deprecated` | Deprecation notice       |
| `@example`    | Example usage            |
| `@see_also`   | Related symbols          |

# Goals

* Keep the parser small
* Keep the format portable
* Avoid language lock-in
* Preserve original source information
* Enable downstream tooling instead of replacing it

# Non-Goals

`docgen` intentionally does not:

* validate types,
* understand language semantics,
* perform AST analysis,
* generate compiler metadata,
* or replace language servers.

It is designed to be a lightweight extraction layer.

# JSON usage

To generate HTML docs, it should be trivial to employ a clanker to walk the JSON and render.
