#include <string.h>

#define NOB_EXPERIMENTAL_DELETE_OLD
#define NOB_IMPLEMENTATION
#include "nob.h"

#define BUILD_FOLDER "build/"
#define SRC_FOLDER "./"

int main(int argc, char **argv) {
  NOB_GO_REBUILD_URSELF(argc, argv);
  shift_args(&argc, &argv);

  if (!nob_mkdir_if_not_exists(BUILD_FOLDER)) return 1;

  Nob_Cmd cmd = {0};

  nob_cmd_append(&cmd,
      "gcc", "-std=c23", "-O2",
      "-Wall", "-Wextra", "-Werror", "-Wswitch-enum", "-Wno-error=unused-variable", "-Wno-error=unused-but-set-variable",
      "-o", (BUILD_FOLDER "docgen"),
      (SRC_FOLDER "main.c"));

  if (!nob_cmd_run(&cmd)) return 1;

  if (argc > 0) {
    const char *program = shift_args(&argc, &argv);

    if (strcmp(program, "run") == 0) {
      nob_cmd_append(&cmd,
          BUILD_FOLDER "docgen");

      while (argc > 0) {
        const char *arg = shift_args(&argc, &argv);
        nob_cmd_append(&cmd, arg);
      }

      if (!nob_cmd_run(&cmd)) return 1;
    }
  }

  return 0;
}
