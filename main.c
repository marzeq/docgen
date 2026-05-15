#include <assert.h>
#include <stdio.h>

#define USE_ALLOC_UTIL
#define USE_STR_BUILDER_UTIL
#define USE_STR_VIEW_UTIL
#define USE_FILE_UTIL
#define USE_DEFER_UTIL
#define USE_DYN_ARR_UTIL
#include "utils.h"

#define FIELD_PREFIX '@'
#define DOC_START    '<'
#define DOC_END      '>'

typedef dyn_arr(str_builder) str_builder_arr;

typedef struct {
  str_view file_name;
  usz line;
} doc_location;

typedef struct {
  str_view name;
  str_builder desc;
} doc_param_field;

typedef dyn_arr(doc_param_field) doc_param_field_arr;

typedef struct {
  str_view name;
  str_view module;
  str_view namespace;
  str_view kind;
  str_builder desc;
  str_builder_arr examples;
  doc_param_field_arr params;
  doc_param_field_arr fields;
  str_builder return_desc;
  str_builder header;
  doc_location location;
  str_builder_arr notes;
  str_builder_arr see_also;
  str_builder_arr warnings;
  str_builder deprecated;
} doc_comment;

typedef dyn_arr(doc_comment) doc_comment_arr;
typedef dyn_arr(doc_comment_arr) doc_comment_arr_arr;

static void json_escape(str_builder* out, str_view sv) {
  for (usz i = 0; i < sv.count; i++) {
    char c = sv.data[i];

    switch (c) {
      case '"':  str_builder_append_cstr(out, "\\\""); break;
      case '\\': str_builder_append_cstr(out, "\\\\"); break;
      case '\n': str_builder_append_cstr(out, "\\n"); break;
      case '\r': str_builder_append_cstr(out, "\\r"); break;
      case '\t': str_builder_append_cstr(out, "\\t"); break;
      default: {
        char buf[2] = {c, '\0'};
        str_builder_append(out, buf);
      } break;
    }
  }
}

static void json_write_str_view(str_builder* out, str_view sv) {
  str_builder_append(out, "\"");
  json_escape(out, sv);
  str_builder_append(out, "\"");
}

static void json_write_str_builder(str_builder* out, str_builder* sb) {
  json_write_str_view(out, str_builder_view(sb));
}

static bool str_builder_has_content(str_builder* sb) {
  return sb->count > 0;
}

static void json_write_comma(bool* first, str_builder* out) {
  if (!*first) {
    str_builder_append(out, ",");
  }

  *first = false;
}

static void json_write_string_field(
  str_builder* out,
  bool* first,
  const char* name,
  str_view value
) {
  if (value.count == 0) return;

  json_write_comma(first, out);

  json_write_str_view(out, str_view_from_cstr(name));
  str_builder_append(out, ":");
  json_write_str_view(out, value);
}

static void json_write_builder_field(
  str_builder* out,
  bool* first,
  const char* name,
  str_builder* value
) {
  if (!str_builder_has_content(value)) return;

  json_write_comma(first, out);

  json_write_str_view(out, str_view_from_cstr(name));
  str_builder_append(out, ":");
  json_write_str_builder(out, value);
}

static void json_write_string_array(
  str_builder* out,
  bool* first,
  const char* name,
  str_builder_arr arr
) {
  if (arr.count == 0) return;

  json_write_comma(first, out);

  json_write_str_view(out, str_view_from_cstr(name));
  str_builder_append(out, ":[");

  for (usz i = 0; i < arr.count; i++) {
    if (i > 0) {
      str_builder_append(out, ",");
    }

    json_write_str_builder(out, &arr.data[i]);
  }

  str_builder_append(out, "]");
}

static void json_write_param_field_array(
  str_builder* out,
  bool* first,
  const char* name,
  doc_param_field_arr arr
) {
  if (arr.count == 0) return;

  json_write_comma(first, out);

  json_write_str_view(out, str_view_from_cstr(name));
  str_builder_append(out, ":[");

  for (usz i = 0; i < arr.count; i++) {
    if (i > 0) {
      str_builder_append(out, ",");
    }

    doc_param_field* field = &arr.data[i];

    str_builder_append(out, "{");

    bool first_inner = true;

    if (field->name.count > 0) {
      json_write_comma(&first_inner, out);

      json_write_str_view(out, str_view_from_cstr("name"));
      str_builder_append(out, ":");
      json_write_str_view(out, field->name);
    }

    if (str_builder_has_content(&field->desc)) {
      json_write_comma(&first_inner, out);

      json_write_str_view(out, str_view_from_cstr("description"));
      str_builder_append(out, ":");
      json_write_str_builder(out, &field->desc);
    }

    str_builder_append(out, "}");
  }

  str_builder_append(out, "]");
}

void json_write_doc_comments(str_builder* out, doc_comment_arr_arr files) {
  str_builder_append(out, "[");

  bool first_comment = true;

  for (usz file_index = 0; file_index < files.count; file_index++) {
    doc_comment_arr* comments = &files.data[file_index];

    for (usz i = 0; i < comments->count; i++) {
      doc_comment* comment = &comments->data[i];

      if (!first_comment) {
        str_builder_append(out, ",");
      }

      first_comment = false;

      str_builder_append(out, "{");

      bool first = true;

      json_write_string_field(out, &first, "name", comment->name);
      json_write_string_field(out, &first, "module", comment->module);
      json_write_string_field(out, &first, "namespace", comment->namespace);
      json_write_string_field(out, &first, "kind", comment->kind);

      json_write_builder_field(out, &first, "description", &comment->desc);
      json_write_builder_field(out, &first, "header", &comment->header);
      json_write_builder_field(out, &first, "return", &comment->return_desc);
      json_write_builder_field(out, &first, "deprecated", &comment->deprecated);

      json_write_string_array(out, &first, "examples", comment->examples);
      json_write_string_array(out, &first, "notes", comment->notes);
      json_write_string_array(out, &first, "see_also", comment->see_also);
      json_write_string_array(out, &first, "warnings", comment->warnings);

      json_write_param_field_array(out, &first, "params", comment->params);
      json_write_param_field_array(out, &first, "fields", comment->fields);

      json_write_comma(&first, out);

      json_write_str_view(out, str_view_from_cstr("location"));
      str_builder_append(out, ":{");

      bool first_location = true;

      json_write_comma(&first_location, out);

      json_write_str_view(out, str_view_from_cstr("file"));
      str_builder_append(out, ":");
      json_write_str_view(out, comment->location.file_name);

      json_write_comma(&first_location, out);

      json_write_str_view(out, str_view_from_cstr("line"));
      str_builder_append(out, ":");

      char line_buf[32];
      snprintf(line_buf, sizeof(line_buf), "%lu", comment->location.line);

      str_builder_append(out, line_buf);

      str_builder_append(out, "}");

      str_builder_append(out, "}");
    }
  }

  str_builder_append(out, "]\n");
}

/*
Example valid doc string in C:

// <@
// @name add
// @kind function
// @desc
// Adds two numbers together
// Works on only integers (this is a continuation of the description)
// @param x The first number
// @param y The second number
// @return The sum of x and y
// @example
// add(2, 3) // returns 5
// @see_also x
// @deprecated This function is deprecated, use add_floats instead
int add(int x, int y) {
// @>
  return x + y;
}

// <@
// @name Person
// @kind struct
// @desc A struct representing a person
// @field first_name The person's first name
// @field last_name The person's last name
// @field age The person's age
// @>
typedef struct {
  str_view first_name;
  str_view last_name;
  usz age;
} Person;

// <@
// @name new_person
// @kind macro
// @desc Creates a new Person struct
// @param first_name The person's first name
// @param last_name The person's last name
// @param age The person's age
// @return A new Person struct
// @>
#define new_person(first_name, last_name, age) \
  ((Person){ .first_name = (first_name), .last_name = (last_name), .age = (age) })
*/

typedef enum {
  DOC_COMMENT_FIELD_NONE,
  DOC_COMMENT_FIELD_NAME,
  DOC_COMMENT_FIELD_KIND,
  DOC_COMMENT_FIELD_MODULE,
  DOC_COMMENT_FIELD_NAMESPACE,
  DOC_COMMENT_FIELD_DESC,
  DOC_COMMENT_FIELD_PARAM,
  DOC_COMMENT_FIELD_FIELD,
  DOC_COMMENT_FIELD_RETURN,
  DOC_COMMENT_FIELD_EXAMPLE,
  DOC_COMMENT_FIELD_NOTE,
  DOC_COMMENT_FIELD_SEE_ALSO,
  DOC_COMMENT_FIELD_WARNING,
  DOC_COMMENT_FIELD_DEPRECATED,
} doc_comment_field_type;

typedef enum {
  PARSER_STATE_IGNORING,
  PARSER_STATE_PARSING,
} parser_state_type;

int isnotspace(int c) {
  return !isspace(c);
}

allocator a;

doc_comment_arr parse_doc_comments(const char* file_name, bool* okay) {
  str_builder sb = { .alloc = a };

  if (!read_entire_file(file_name, &sb)) {
    return (doc_comment_arr){0};
  }

  str_view sv = str_builder_view(&sb);

  doc_comment_arr comments = {0};

  doc_comment_field_type current_field = DOC_COMMENT_FIELD_NONE;
  parser_state_type parser_state = PARSER_STATE_IGNORING;

  str_view expected_comment_prefix;
  usz line_number = 0;

#define DOC_COMMENT_INIT ((doc_comment){ \
    .desc = { .alloc = a },              \
    .header = { .alloc = a },            \
    .params = { .alloc = a },            \
    .fields = { .alloc = a },            \
    .return_desc = { .alloc = a },       \
    .notes = { .alloc = a },             \
    .see_also = { .alloc = a },          \
    .warnings = { .alloc = a },          \
    .deprecated = { .alloc = a },        \
  })

  doc_comment current_comment = DOC_COMMENT_INIT;

  while (sv.count > 0) {
    line_number += 1;
    str_view untrimmed_line = str_view_chop_by_delim(&sv, '\n');
    str_view line = str_view_trim(untrimmed_line);
    
    switch (parser_state) {
      case PARSER_STATE_IGNORING: {
        str_view comment_start = str_view_chop_by_delim(&line, DOC_START);
        if (line.count == 0 || line.data[0] != FIELD_PREFIX) {
          continue;
        }
        str_view trimmed_comment = str_view_trim(comment_start);

        if (line.count != 1 || line.data[0] != FIELD_PREFIX) {
          fprintf(stderr, "Error: Unexpected text after %c in file " svpfmt " line %lu\n", DOC_START, svpfarg(str_view_from_cstr(file_name)), line_number);
          *okay = false;
          return (doc_comment_arr){0};
        }

        expected_comment_prefix = trimmed_comment;
        parser_state = PARSER_STATE_PARSING;
        current_comment.location = (doc_location){
          .file_name = str_view_from_cstr(file_name),
          .line = line_number,
        };
      } break;
      case PARSER_STATE_PARSING: {
        if (!str_view_starts_with(line, expected_comment_prefix)) {
          if (current_comment.header.count != 0) {
            str_builder_append(&current_comment.header, "\n");
          }
          str_builder_append(&current_comment.header, untrimmed_line);
          continue;
        }

        str_view_chop_prefix(&line, expected_comment_prefix);
        line = str_view_trim(line);
        
        if (line.count == 0) continue;

        if (line.data[0] != FIELD_PREFIX) {
          switch (current_field) {
            case DOC_COMMENT_FIELD_NONE: {
              fprintf(stderr, "Error: Expected field starting with %c in file " svpfmt " line %lu\n", FIELD_PREFIX, svpfarg(str_view_from_cstr(file_name)), line_number);
              *okay = false;
              return (doc_comment_arr){0};
            } break;
            case DOC_COMMENT_FIELD_NAME: {
              if (current_comment.name.count > 0) {
                fprintf(stderr, "Multiline @name is not allowed. Error in file " svpfmt " line %lu\n", svpfarg(str_view_from_cstr(file_name)), line_number);
                *okay = false;
                return (doc_comment_arr){0};
              }
              current_comment.name = line;
            } break;
            case DOC_COMMENT_FIELD_KIND: {
              if (current_comment.kind.count > 0) {
                fprintf(stderr, "Multiline @kind is not allowed. Error in file " svpfmt " line %lu\n", svpfarg(str_view_from_cstr(file_name)), line_number);
                *okay = false;
                return (doc_comment_arr){0};
              }
              current_comment.kind = line;
            } break;
            case DOC_COMMENT_FIELD_MODULE: {
              if (current_comment.module.count > 0) {
                fprintf(stderr, "Multiline @module is not allowed. Error in file " svpfmt " line %lu\n", svpfarg(str_view_from_cstr(file_name)), line_number);
                *okay = false;
                return (doc_comment_arr){0};
              }
              current_comment.module = line;
            } break;
            case DOC_COMMENT_FIELD_NAMESPACE: {
              if (current_comment.namespace.count > 0) {
                fprintf(stderr, "Multiline @namespace is not allowed. Error in file " svpfmt " line %lu\n", svpfarg(str_view_from_cstr(file_name)), line_number);
                *okay = false;
                return (doc_comment_arr){0};
              }
              current_comment.namespace = line;
            } break;
            case DOC_COMMENT_FIELD_DESC: {
              if (current_comment.desc.count > 0) {
                str_builder_append(&current_comment.desc, "\n");
              }
              str_builder_append(&current_comment.desc, line);
            } break;
            case DOC_COMMENT_FIELD_PARAM: {
              assert(current_comment.params.count > 0 && "invalid state");

              doc_param_field* last_param = &current_comment.params.data[current_comment.params.count - 1];
              str_builder_append(&last_param->desc, "\n");
              str_builder_append(&last_param->desc, line);
            } break;
            case DOC_COMMENT_FIELD_FIELD: {
              assert(current_comment.fields.count > 0 && "invalid state");

              doc_param_field* last_field = &current_comment.fields.data[current_comment.fields.count - 1];
              str_builder_append(&last_field->desc, "\n");
              str_builder_append(&last_field->desc, line);
            } break;
            case DOC_COMMENT_FIELD_RETURN: {
              str_builder_append(&current_comment.return_desc, "\n");
              str_builder_append(&current_comment.return_desc, line);
            } break;
            case DOC_COMMENT_FIELD_EXAMPLE: {
              if (current_comment.examples.count > 0) {
                if (current_comment.examples.data[current_comment.examples.count - 1].count > 0) {
                  str_builder_append(&current_comment.examples.data[current_comment.examples.count - 1], "\n");
                }
                str_builder_append(&current_comment.examples.data[current_comment.examples.count - 1], line);
              } else {
                fprintf(stderr, "Error: Text after @example without an example name in file " svpfmt " line %lu\n", svpfarg(str_view_from_cstr(file_name)), line_number);
                *okay = false;
                return (doc_comment_arr){0};
              }
            } break;
            case DOC_COMMENT_FIELD_NOTE: {
              if (current_comment.notes.count > 0) {
                if (current_comment.notes.data[current_comment.notes.count - 1].count > 0) {
                  str_builder_append(&current_comment.notes.data[current_comment.notes.count - 1], "\n");
                }
                str_builder_append(&current_comment.notes.data[current_comment.notes.count - 1], line);
              } else {
                fprintf(stderr, "Error: Text after @note without a note name in file " svpfmt " line %lu\n", svpfarg(str_view_from_cstr(file_name)), line_number);
                *okay = false;
                return (doc_comment_arr){0};
              }
            } break;
            case DOC_COMMENT_FIELD_SEE_ALSO: {
              if (current_comment.see_also.count > 0) {
                if (current_comment.see_also.data[current_comment.see_also.count - 1].count > 0) {
                  str_builder_append(&current_comment.see_also.data[current_comment.see_also.count - 1], "\n");
                }
                str_builder_append(&current_comment.see_also.data[current_comment.see_also.count - 1], line);
              } else {
                fprintf(stderr, "Error: Text after @see_also without a see_also name in file " svpfmt " line %lu\n", svpfarg(str_view_from_cstr(file_name)), line_number);
                *okay = false;
                return (doc_comment_arr){0};
              }
            } break;
            case DOC_COMMENT_FIELD_WARNING: {
              if (current_comment.warnings.count > 0) {
                str_builder_append(&current_comment.warnings.data[current_comment.warnings.count - 1], "\n");
                str_builder_append(&current_comment.warnings.data[current_comment.warnings.count - 1], line);
              } else {
                fprintf(stderr, "Error: Text after @warning without a warning name in file " svpfmt " line %lu\n", svpfarg(str_view_from_cstr(file_name)), line_number);
                *okay = false;
                return (doc_comment_arr){0};
              }
            } break;
            case DOC_COMMENT_FIELD_DEPRECATED: {
              if (current_comment.deprecated.count > 0) {
                if (current_comment.deprecated.count > 0) {
                  str_builder_append(&current_comment.deprecated, "\n");
                }
                str_builder_append(&current_comment.deprecated, line);
              } else {
                fprintf(stderr, "Error: Text after @deprecated without a @deprecated field in file " svpfmt " line %lu\n", svpfarg(str_view_from_cstr(file_name)), line_number);
                *okay = false;
                return (doc_comment_arr){0};
              }
            } break;
            default: assert(false && "Unreachable");
          }
          continue;
        }

        str_view_chop_left(&line, 1);

        if (line.data[0] == DOC_END) {
          if (line.count != 1) {
            fprintf(stderr, "Error: Unexpected text after %c in file " svpfmt " line %lu\n", DOC_END, svpfarg(str_view_from_cstr(file_name)), line_number);
            *okay = false;
            return (doc_comment_arr){0};
          }
          if (current_comment.name.count == 0) {
            fprintf(stderr, "Error: Doc comment missing @name field in file " svpfmt " line %lu\n", svpfarg(str_view_from_cstr(file_name)), line_number);
            *okay = false;
            return (doc_comment_arr){0};
          }
          da_push(&comments, current_comment);
          current_comment = DOC_COMMENT_INIT;
          parser_state = PARSER_STATE_IGNORING;
          continue;
        }

        str_view field_name = str_view_chop_while(&line, isnotspace);
        if (field_name.count == 0) {
          fprintf(stderr, "Error: Expected field name after '%c' in file " svpfmt " line %lu\n", FIELD_PREFIX, svpfarg(str_view_from_cstr(file_name)), line_number);
          *okay = false;
          return (doc_comment_arr){0};
        }

        line = str_view_trim(line);

        if (str_view_eq_cstr(field_name, "name")) {
          current_field = DOC_COMMENT_FIELD_NAME;
          if (current_comment.name.count > 0) {
            fprintf(stderr, "Error: Multiple @name fields in file " svpfmt " line %lu\n", svpfarg(str_view_from_cstr(file_name)), line_number);
            *okay = false;
            return (doc_comment_arr){0};
          }
          current_comment.name = line;
        } else if (str_view_eq_cstr(field_name, "kind")) {
          current_field = DOC_COMMENT_FIELD_KIND;
          if (current_comment.kind.count > 0) {
            fprintf(stderr, "Error: Multiple @kind fields in file " svpfmt " line %lu\n", svpfarg(str_view_from_cstr(file_name)), line_number);
            *okay = false;
            return (doc_comment_arr){0};
          }
          current_comment.kind = line;
        } else if (str_view_eq_cstr(field_name, "module")) {
          current_field = DOC_COMMENT_FIELD_MODULE;
          if (current_comment.module.count > 0) {
            fprintf(stderr, "Error: Multiple @module fields in file " svpfmt " line %lu\n", svpfarg(str_view_from_cstr(file_name)), line_number);
            *okay = false;
            return (doc_comment_arr){0};
          }
          current_comment.module = line;
        } else if (str_view_eq_cstr(field_name, "namespace")) {
          current_field = DOC_COMMENT_FIELD_NAMESPACE;
          if (current_comment.namespace.count > 0) {
            fprintf(stderr, "Error: Multiple @namespace fields in file " svpfmt " line %lu\n", svpfarg(str_view_from_cstr(file_name)), line_number);
            *okay = false;
            return (doc_comment_arr){0};
          }
          current_comment.namespace = line;
        } else if (str_view_eq_cstr(field_name, "desc")) {
          current_field = DOC_COMMENT_FIELD_DESC;
          str_builder_append(&current_comment.desc, line);
        } else if (str_view_eq_cstr(field_name, "param")) {
          current_field = DOC_COMMENT_FIELD_PARAM;
          str_view param_name = str_view_chop_while(&line, isgraph);
          line = str_view_trim(line);
          if (param_name.count == 0) {
            fprintf(stderr, "Error: Expected parameter name after '@param' in file " svpfmt " line %lu\n", svpfarg(str_view_from_cstr(file_name)), line_number);
            *okay = false;
            return (doc_comment_arr){0};
          }
          doc_param_field new_param = {
            .name = param_name,
            .desc = { .alloc = a },
          };
          str_builder_append(&new_param.desc, line);
          da_push(&current_comment.params, new_param);
        } else if (str_view_eq_cstr(field_name, "field")) {
          current_field = DOC_COMMENT_FIELD_FIELD;
          str_view field_name = str_view_chop_while(&line, isgraph);
          line = str_view_trim(line);
          if (field_name.count == 0) {
            fprintf(stderr, "Error: Expected field name after '@field' in file " svpfmt " line %lu\n", svpfarg(str_view_from_cstr(file_name)), line_number);
            *okay = false;
            return (doc_comment_arr){0};
          }
          doc_param_field new_field = {
            .name = field_name,
            .desc = { .alloc = a },
          };
          str_builder_append(&new_field.desc, line);
          da_push(&current_comment.fields, new_field);
        } else if (str_view_eq_cstr(field_name, "return")) {
          current_field = DOC_COMMENT_FIELD_RETURN;
          if (current_comment.return_desc.count > 0) {
            fprintf(stderr, "Error: Multiple @return fields in file " svpfmt " line %lu\n", svpfarg(str_view_from_cstr(file_name)), line_number);
            *okay = false;
            return (doc_comment_arr){0};
          }
          str_builder_append(&current_comment.return_desc, line);
        } else if (str_view_eq_cstr(field_name, "example")) {
          current_field = DOC_COMMENT_FIELD_EXAMPLE;
          str_view_chop_while(&line, isgraph);
          line = str_view_trim(line);
          str_builder new_example = { .alloc = a };
          str_builder_append(&new_example, line);
          da_push(&current_comment.examples, new_example);
        } else if (str_view_eq_cstr(field_name, "note")) {
          current_field = DOC_COMMENT_FIELD_NOTE;
          str_builder new_note = { .alloc = a };
          str_builder_append(&new_note, line);
          da_push(&current_comment.notes, new_note);
        } else if (str_view_eq_cstr(field_name, "see_also")) {
          current_field = DOC_COMMENT_FIELD_SEE_ALSO;
          str_builder new_see_also = { .alloc = a };
          str_builder_append(&new_see_also, line);
          da_push(&current_comment.see_also, new_see_also);
        } else if (str_view_eq_cstr(field_name, "warning")) {
          current_field = DOC_COMMENT_FIELD_WARNING;
          str_builder new_warning = { .alloc = a };
          str_builder_append(&new_warning, line);
          da_push(&current_comment.warnings, new_warning);
        } else if (str_view_eq_cstr(field_name, "deprecated")) {
          current_field = DOC_COMMENT_FIELD_DEPRECATED;
          str_builder_append(&current_comment.deprecated, line);
        } else {
          fprintf(stderr, "Error: Unknown field '" svpfmt "' in file " svpfmt " line %lu\n", svpfarg(field_name), svpfarg(str_view_from_cstr(file_name)), line_number);
          *okay = false;
          return (doc_comment_arr){0};
        }
      } break;
      default: assert(false && "Unreachable");
    }
  }

  *okay = true;
  return comments;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s [input_files...]\n", argv[0]);
    return 1;
  }

  alloc_tracker at = {0};
  defer(alloc_tracker_free_all(&at););
  a = tracked_allocator(&at);
  
  doc_comment_arr_arr all_comments = { .alloc = a };

  for (int i = 1; i < argc; i++) {
    bool okay;
    doc_comment_arr comments = parse_doc_comments(argv[i], &okay);
    if (!okay) {
      fprintf(stderr, "Failed to parse doc comments in file %s\n", argv[i]);
      return 1;
    }

    da_push(&all_comments, comments);
  }

  str_builder output = { .alloc = a };
  json_write_doc_comments(&output, all_comments);

  if (!write_entire_file("/dev/stdout", output)) {
    fprintf(stderr, "Failed to write output file\n");
    return 1;
  }

  return 0;
}
