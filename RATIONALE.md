# Why I Built This

What I dislike in most documentation generators is that they tend to become deeply tied to:

* a specific compiler,
* a specific AST,
* a specific type system,
* or an entire ecosystem of tooling and plugins.

That works well when you only care about one language, but it also creates a huge amount of complexity and rigidity.

My goal was different.

I wanted a documentation system that:

* works across multiple languages,
* stays extremely lightweight,
* avoids compiler or AST dependencies,
* and outputs a clean intermediate representation that downstream tooling can fully control.

Instead of generating HTML directly, my tool generates structured JSON. That means the actual presentation layer is completely decoupled from the parser itself.

This way, downstream can then build:

* static documentation sites,
* IDE integrations,
* search indexes,
* code browsers,
* API explorers,
* bindings generators,
* or custom rendering pipelines

on top of the same output format.

# Core Design Philosophy

## Language-Agnostic by Design

Most documentation tooling assumes deep knowledge of the language being parsed.

I intentionally avoided that approach.

The parser only understands:

* structured metadata tags,
* comment delimiters,
* and raw source text.

That allows the same system to work across very different languages with minimal configuration as long as they have prefix-based comments.

For example:

### C

```c
// <@
// @name add
// @desc Adds two numbers together
int add(int x, int y) {
// @>
  return x + y;
}
```

### Lua

```lua
-- <@
-- @name add
-- @desc Adds two numbers together
function add(x, y)
-- @>
  return x + y
end
```

Both generate the same structured output format.

# One of the Main Ideas: Preserving Raw Signatures

One of the original ideas behind this project was preserving the declaration/signature exactly as written in source code.

Most documentation systems normalise signatures into language-specific type representations. Since my parser is intentionally language-agnostic, it cannot reliably understand every downstream type system.

Instead of trying to interpret types, I can preserve the raw declaration directly inside the generated JSON.

For example:

```json
{
  "name": "add",
  "header": "int add(int x, int y) {"
}
```

This keeps:

* original formatting,
* qualifiers,
* attributes,
* generics,
* macros,
* annotations,
* and language-specific syntax

fully intact.

That decision allows the parser to support complex declarations without needing compiler infrastructure or AST parsing.

# How It Works

The parser operates as a small state machine.

Inside a documentation block:

```c
// <@
```

we first pick up on the comment prefix (in this case `//`) and then parse structured metadata lines.

Comment-prefixed lines become structured metadata:

```c
// @param x The first number
```

while non-comment lines are captured as raw declaration/header content:

```c
int add(int x, int y) {
```

The block ends with:

```c
// @>
```

The extracted information is then emitted as structured JSON.

Example output:

```json
{
  "name": "add",
  "description": "Adds two numbers together",
  "header": "int add(int x, int y) {",
  "params": [
    {
      "name": "x",
      "description": "The first number"
    }
  ]
}
```

# Drawbacks and limitations

As this tool is language-agnostic and as such can't build an AST of the language, it cannot leverage the advantages that come from ASTs (for example linking an argument type to it's definiton in HTML documentation).

This is an unavoidable tradeoff that comes with going language-agnostic, but it also means the tool is much easier to extend to new languages and use in a wider variety of contexts.

# Technical Notes

The project is implemented in C with the aid of my `utils.h` library.
It originally started out as a real-world test of that library.

The parser intentionally avoids:

* regex-heavy parsing,
* AST generation,
* compiler frontends,
* or language-specific semantic analysis.

This keeps the implementation portable, fast, and easy to extend.
