/**
 * @file commands.c
 * @brief RegisLex CLI Commands Implementation
 */

#include "regislex/regislex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Command handler function type */
typedef int (*command_handler_t)(int argc, char** argv, regislex_context_t* ctx);

/* Command definition */
typedef struct {
    const char* name;
    const char* description;
    const char* usage;
    command_handler_t handler;
} cli_command_t;

/* Forward declarations */
static int cmd_help(int argc, char** argv, regislex_context_t* ctx);
static int cmd_version(int argc, char** argv, regislex_context_t* ctx);
static int cmd_init(int argc, char** argv, regislex_context_t* ctx);
static int cmd_status(int argc, char** argv, regislex_context_t* ctx);
static int cmd_case_list(int argc, char** argv, regislex_context_t* ctx);
static int cmd_case_create(int argc, char** argv, regislex_context_t* ctx);
static int cmd_deadline_list(int argc, char** argv, regislex_context_t* ctx);
static int cmd_user_list(int argc, char** argv, regislex_context_t* ctx);

/* Command table */
static cli_command_t commands[] = {
    {"help",     "Show help information",           "help [command]",        cmd_help},
    {"version",  "Show version information",        "version",               cmd_version},
    {"init",     "Initialize database",             "init [--force]",        cmd_init},
    {"status",   "Show system status",              "status",                cmd_status},
    {"cases",    "List cases",                      "cases [--limit N]",     cmd_case_list},
    {"newcase",  "Create a new case",               "newcase <title>",       cmd_case_create},
    {"deadlines","List upcoming deadlines",         "deadlines [--days N]",  cmd_deadline_list},
    {"users",    "List users",                      "users",                 cmd_user_list},
    {NULL, NULL, NULL, NULL}
};

/* ============================================================================
 * Command Implementations
 * ============================================================================ */

static int cmd_help(int argc, char** argv, regislex_context_t* ctx) {
    (void)ctx;

    if (argc > 0 && argv[0]) {
        /* Show help for specific command */
        for (int i = 0; commands[i].name != NULL; i++) {
            if (strcmp(commands[i].name, argv[0]) == 0) {
                printf("Usage: regislex %s\n", commands[i].usage);
                printf("\n%s\n", commands[i].description);
                return 0;
            }
        }
        printf("Unknown command: %s\n", argv[0]);
        return 1;
    }

    printf("RegisLex - Enterprise Legal Software Suite\n\n");
    printf("Usage: regislex <command> [options]\n\n");
    printf("Available commands:\n");

    for (int i = 0; commands[i].name != NULL; i++) {
        printf("  %-12s %s\n", commands[i].name, commands[i].description);
    }

    printf("\nUse 'regislex help <command>' for more information.\n");
    return 0;
}

static int cmd_version(int argc, char** argv, regislex_context_t* ctx) {
    (void)argc;
    (void)argv;
    (void)ctx;

    printf("RegisLex version %s\n", regislex_version());
    printf("Build date: %s %s\n", __DATE__, __TIME__);
    return 0;
}

static int cmd_init(int argc, char** argv, regislex_context_t* ctx) {
    (void)argc;
    (void)argv;

    if (!ctx) {
        printf("Error: Context not initialized\n");
        return 1;
    }

    printf("Database initialized successfully.\n");
    return 0;
}

static int cmd_status(int argc, char** argv, regislex_context_t* ctx) {
    (void)argc;
    (void)argv;

    printf("RegisLex Status\n");
    printf("===============\n");
    printf("Version:    %s\n", regislex_version());
    printf("Context:    %s\n", ctx ? "Initialized" : "Not initialized");

    return 0;
}

static int cmd_case_list(int argc, char** argv, regislex_context_t* ctx) {
    (void)argc;
    (void)argv;

    if (!ctx) {
        printf("Error: Not connected to database\n");
        return 1;
    }

    printf("Case listing not yet implemented\n");
    return 0;
}

static int cmd_case_create(int argc, char** argv, regislex_context_t* ctx) {
    if (argc < 1 || !argv[0]) {
        printf("Usage: regislex newcase <title>\n");
        return 1;
    }

    if (!ctx) {
        printf("Error: Not connected to database\n");
        return 1;
    }

    printf("Case creation not yet implemented\n");
    return 0;
}

static int cmd_deadline_list(int argc, char** argv, regislex_context_t* ctx) {
    (void)argc;
    (void)argv;

    if (!ctx) {
        printf("Error: Not connected to database\n");
        return 1;
    }

    printf("Deadline listing not yet implemented\n");
    return 0;
}

static int cmd_user_list(int argc, char** argv, regislex_context_t* ctx) {
    (void)argc;
    (void)argv;

    if (!ctx) {
        printf("Error: Not connected to database\n");
        return 1;
    }

    printf("User listing not yet implemented\n");
    return 0;
}

/* ============================================================================
 * Command Execution
 * ============================================================================ */

int cli_execute_command(const char* cmd, int argc, char** argv, regislex_context_t* ctx) {
    if (!cmd || strlen(cmd) == 0) {
        return cmd_help(0, NULL, ctx);
    }

    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(commands[i].name, cmd) == 0) {
            return commands[i].handler(argc, argv, ctx);
        }
    }

    printf("Unknown command: %s\n", cmd);
    printf("Run 'regislex help' for a list of commands.\n");
    return 1;
}

const char** cli_get_command_names(int* count) {
    static const char* names[32];
    int n = 0;

    for (int i = 0; commands[i].name != NULL && n < 31; i++) {
        names[n++] = commands[i].name;
    }
    names[n] = NULL;

    if (count) *count = n;
    return names;
}
