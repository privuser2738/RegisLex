/**
 * @file cli_main.c
 * @brief RegisLex CLI Tool
 *
 * Command-line interface for managing RegisLex legal software.
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Command function type */
typedef int (*command_func_t)(regislex_context_t* ctx, int argc, char** argv);

/* Command structure */
typedef struct {
    const char* name;
    const char* description;
    const char* usage;
    command_func_t func;
} cli_command_t;

/* Forward declarations */
static int cmd_help(regislex_context_t* ctx, int argc, char** argv);
static int cmd_version(regislex_context_t* ctx, int argc, char** argv);
static int cmd_init(regislex_context_t* ctx, int argc, char** argv);
static int cmd_case_list(regislex_context_t* ctx, int argc, char** argv);
static int cmd_case_create(regislex_context_t* ctx, int argc, char** argv);
static int cmd_case_show(regislex_context_t* ctx, int argc, char** argv);
static int cmd_deadline_list(regislex_context_t* ctx, int argc, char** argv);
static int cmd_deadline_upcoming(regislex_context_t* ctx, int argc, char** argv);
static int cmd_document_list(regislex_context_t* ctx, int argc, char** argv);
static int cmd_report_generate(regislex_context_t* ctx, int argc, char** argv);
static int cmd_status(regislex_context_t* ctx, int argc, char** argv);

/* Available commands */
static const cli_command_t commands[] = {
    {"help", "Show help information", "help [command]", cmd_help},
    {"version", "Show version information", "version", cmd_version},
    {"init", "Initialize database and configuration", "init [--force]", cmd_init},
    {"status", "Show system status", "status", cmd_status},
    {"case-list", "List cases", "case-list [--status <status>] [--limit <n>]", cmd_case_list},
    {"case-create", "Create a new case", "case-create --number <num> --title <title> --type <type>", cmd_case_create},
    {"case-show", "Show case details", "case-show <case-id>", cmd_case_show},
    {"deadline-list", "List deadlines", "deadline-list [--case <case-id>]", cmd_deadline_list},
    {"deadline-upcoming", "Show upcoming deadlines", "deadline-upcoming [--days <n>]", cmd_deadline_upcoming},
    {"document-list", "List documents", "document-list [--case <case-id>]", cmd_document_list},
    {"report", "Generate a report", "report <report-type> [--format <format>]", cmd_report_generate},
    {NULL, NULL, NULL, NULL}
};

/**
 * @brief Print CLI banner
 */
static void print_banner(void) {
    printf("\n");
    printf("  ____            _     _           \n");
    printf(" |  _ \\ ___  __ _(_)___| |    _____  __\n");
    printf(" | |_) / _ \\/ _` | / __| |   / _ \\ \\/ /\n");
    printf(" |  _ <  __/ (_| | \\__ \\ |__|  __/>  < \n");
    printf(" |_| \\_\\___|\\__, |_|___/_____\\___/_/\\_\\\n");
    printf("            |___/                     \n");
    printf("\n");
    printf(" Enterprise Legal Software Suite v%s\n", regislex_version());
    printf("\n");
}

/**
 * @brief Find command by name
 */
static const cli_command_t* find_command(const char* name) {
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(commands[i].name, name) == 0) {
            return &commands[i];
        }
    }
    return NULL;
}

/**
 * @brief Help command
 */
static int cmd_help(regislex_context_t* ctx, int argc, char** argv) {
    (void)ctx;

    if (argc > 0) {
        /* Show help for specific command */
        const cli_command_t* cmd = find_command(argv[0]);
        if (cmd) {
            printf("Command: %s\n", cmd->name);
            printf("  %s\n\n", cmd->description);
            printf("Usage: regislex-cli %s\n\n", cmd->usage);
            return 0;
        }
        printf("Unknown command: %s\n\n", argv[0]);
    }

    print_banner();
    printf("Usage: regislex-cli <command> [options]\n\n");
    printf("Available Commands:\n\n");

    for (int i = 0; commands[i].name != NULL; i++) {
        printf("  %-20s %s\n", commands[i].name, commands[i].description);
    }

    printf("\nUse 'regislex-cli help <command>' for more information about a command.\n\n");
    return 0;
}

/**
 * @brief Version command
 */
static int cmd_version(regislex_context_t* ctx, int argc, char** argv) {
    (void)ctx;
    (void)argc;
    (void)argv;

    printf("RegisLex CLI v%s\n", regislex_version());
    printf("\n");
    printf("Components:\n");
    printf("  - Core Library: %s\n", regislex_version());
    printf("  - Database: SQLite (embedded)\n");
    printf("  - Platform: ");
#ifdef REGISLEX_PLATFORM_WINDOWS
    printf("Windows\n");
#elif defined(REGISLEX_PLATFORM_LINUX)
    printf("Linux\n");
#elif defined(REGISLEX_PLATFORM_MACOS)
    printf("macOS\n");
#elif defined(REGISLEX_PLATFORM_ANDROID)
    printf("Android\n");
#else
    printf("Unknown\n");
#endif
    printf("\n");

    return 0;
}

/**
 * @brief Init command
 */
static int cmd_init(regislex_context_t* ctx, int argc, char** argv) {
    (void)ctx;

    bool force = false;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--force") == 0 || strcmp(argv[i], "-f") == 0) {
            force = true;
        }
    }

    printf("Initializing RegisLex...\n");

    regislex_config_t config;
    regislex_error_t err = regislex_config_default(&config);
    if (err != REGISLEX_OK) {
        fprintf(stderr, "Error: Failed to create default configuration\n");
        return 1;
    }

    printf("  Data directory: %s\n", config.data_dir);

    /* Check if already initialized */
    if (!force && platform_file_exists(config.database.database)) {
        printf("\nRegisLex is already initialized.\n");
        printf("Use --force to reinitialize (this will NOT delete existing data).\n");
        return 0;
    }

    /* Create directories */
    if (!platform_file_exists(config.data_dir)) {
        printf("  Creating data directory...\n");
        platform_mkdir(config.data_dir, true);
    }

    if (!platform_file_exists(config.log_dir)) {
        printf("  Creating log directory...\n");
        platform_mkdir(config.log_dir, true);
    }

    if (!platform_file_exists(config.storage.base_path)) {
        printf("  Creating document storage directory...\n");
        platform_mkdir(config.storage.base_path, true);
    }

    /* Initialize database */
    printf("  Initializing database...\n");
    regislex_context_t* init_ctx = NULL;
    err = regislex_init(&config, &init_ctx);
    if (err != REGISLEX_OK) {
        fprintf(stderr, "Error: Failed to initialize database\n");
        return 1;
    }

    regislex_shutdown(init_ctx);

    printf("\nRegisLex initialized successfully!\n");
    printf("\nYou can now:\n");
    printf("  - Start the server: regislex\n");
    printf("  - Create a case: regislex-cli case-create --number \"2024-001\" --title \"New Case\"\n");
    printf("  - List cases: regislex-cli case-list\n");
    printf("\n");

    return 0;
}

/**
 * @brief Status command
 */
static int cmd_status(regislex_context_t* ctx, int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("RegisLex System Status\n");
    printf("======================\n\n");

    regislex_config_t config;
    regislex_config_default(&config);

    /* Check directories */
    printf("Directories:\n");
    printf("  Data: %s %s\n", config.data_dir,
           platform_file_exists(config.data_dir) ? "(exists)" : "(missing)");
    printf("  Logs: %s %s\n", config.log_dir,
           platform_file_exists(config.log_dir) ? "(exists)" : "(missing)");
    printf("  Documents: %s %s\n", config.storage.base_path,
           platform_file_exists(config.storage.base_path) ? "(exists)" : "(missing)");
    printf("\n");

    /* Check database */
    printf("Database:\n");
    printf("  Type: %s\n", config.database.type);
    printf("  Path: %s\n", config.database.database);
    printf("  Status: %s\n",
           platform_file_exists(config.database.database) ? "Initialized" : "Not initialized");
    printf("\n");

    if (ctx) {
        /* Show counts */
        printf("Statistics:\n");
        printf("  Cases: (query database for count)\n");
        printf("  Documents: (query database for count)\n");
        printf("  Deadlines: (query database for count)\n");
        printf("\n");
    }

    return 0;
}

/**
 * @brief Case list command
 */
static int cmd_case_list(regislex_context_t* ctx, int argc, char** argv) {
    int limit = 20;
    const char* status_filter = NULL;

    for (int i = 0; i < argc; i++) {
        if ((strcmp(argv[i], "--limit") == 0 || strcmp(argv[i], "-n") == 0) && i + 1 < argc) {
            limit = atoi(argv[++i]);
        }
        if ((strcmp(argv[i], "--status") == 0 || strcmp(argv[i], "-s") == 0) && i + 1 < argc) {
            status_filter = argv[++i];
        }
    }

    (void)status_filter;

    printf("Cases (showing up to %d)\n", limit);
    printf("%-36s  %-15s  %-30s  %-10s  %-10s\n",
           "ID", "Case Number", "Title", "Status", "Priority");
    printf("%-36s  %-15s  %-30s  %-10s  %-10s\n",
           "------------------------------------",
           "---------------",
           "------------------------------",
           "----------",
           "----------");

    if (!ctx) {
        printf("\n(No database connection - initialize first with 'regislex-cli init')\n");
        return 0;
    }

    /* Query database for cases */
    regislex_case_filter_t filter = {0};
    filter.limit = limit;

    regislex_case_list_t* list = NULL;
    regislex_error_t err = regislex_case_list(ctx, &filter, &list);

    if (err != REGISLEX_OK) {
        printf("\n(Error retrieving cases)\n");
        return 1;
    }

    if (list && list->count > 0) {
        const char* status_names[] = {
            "Draft", "Active", "Pending", "On Hold",
            "Completed", "Closed", "Archived", "Cancelled"
        };
        const char* priority_names[] = {
            "Low", "Normal", "High", "Urgent", "Critical"
        };

        for (int i = 0; i < list->count; i++) {
            regislex_case_t* c = list->cases[i];
            char short_title[28];
            strncpy(short_title, c->title, 27);
            short_title[27] = '\0';
            if (strlen(c->title) > 27) {
                strcat(short_title, "...");
            }

            printf("%-36s  %-15s  %-30s  %-10s  %-10s\n",
                   c->id.value,
                   c->case_number,
                   short_title,
                   status_names[c->status],
                   priority_names[c->priority]);
        }
        printf("\nTotal: %d cases\n", list->count);
        regislex_case_list_free(list);
    } else {
        printf("\n(No cases found)\n");
    }

    return 0;
}

/**
 * @brief Case create command
 */
static int cmd_case_create(regislex_context_t* ctx, int argc, char** argv) {
    const char* case_number = NULL;
    const char* title = NULL;
    const char* type_str = "civil";
    const char* description = NULL;

    for (int i = 0; i < argc; i++) {
        if ((strcmp(argv[i], "--number") == 0 || strcmp(argv[i], "-n") == 0) && i + 1 < argc) {
            case_number = argv[++i];
        }
        if ((strcmp(argv[i], "--title") == 0 || strcmp(argv[i], "-t") == 0) && i + 1 < argc) {
            title = argv[++i];
        }
        if ((strcmp(argv[i], "--type") == 0) && i + 1 < argc) {
            type_str = argv[++i];
        }
        if ((strcmp(argv[i], "--description") == 0 || strcmp(argv[i], "-d") == 0) && i + 1 < argc) {
            description = argv[++i];
        }
    }

    if (!case_number || !title) {
        printf("Usage: regislex-cli case-create --number <num> --title <title> [--type <type>]\n\n");
        printf("Required:\n");
        printf("  --number, -n    Case number (e.g., \"2024-CV-001\")\n");
        printf("  --title, -t     Case title\n");
        printf("\nOptional:\n");
        printf("  --type          Case type: civil, criminal, administrative, etc.\n");
        printf("  --description   Case description\n");
        return 1;
    }

    if (!ctx) {
        fprintf(stderr, "Error: Not connected to database. Run 'regislex-cli init' first.\n");
        return 1;
    }

    /* Parse case type */
    regislex_case_type_t type = REGISLEX_CASE_TYPE_CIVIL;
    if (strcmp(type_str, "criminal") == 0) type = REGISLEX_CASE_TYPE_CRIMINAL;
    else if (strcmp(type_str, "administrative") == 0) type = REGISLEX_CASE_TYPE_ADMINISTRATIVE;
    else if (strcmp(type_str, "regulatory") == 0) type = REGISLEX_CASE_TYPE_REGULATORY;
    else if (strcmp(type_str, "appellate") == 0) type = REGISLEX_CASE_TYPE_APPELLATE;
    else if (strcmp(type_str, "bankruptcy") == 0) type = REGISLEX_CASE_TYPE_BANKRUPTCY;
    else if (strcmp(type_str, "family") == 0) type = REGISLEX_CASE_TYPE_FAMILY;
    else if (strcmp(type_str, "contract") == 0) type = REGISLEX_CASE_TYPE_CONTRACT;
    else if (strcmp(type_str, "tort") == 0) type = REGISLEX_CASE_TYPE_TORT;

    /* Create case */
    regislex_case_t case_data = {0};
    strncpy(case_data.case_number, case_number, sizeof(case_data.case_number) - 1);
    strncpy(case_data.title, title, sizeof(case_data.title) - 1);
    case_data.type = type;
    case_data.status = REGISLEX_STATUS_ACTIVE;
    case_data.priority = REGISLEX_PRIORITY_NORMAL;

    if (description) {
        strncpy(case_data.description, description, sizeof(case_data.description) - 1);
    }

    regislex_case_t* new_case = NULL;
    regislex_error_t err = regislex_case_create(ctx, &case_data, &new_case);

    if (err != REGISLEX_OK) {
        fprintf(stderr, "Error: Failed to create case\n");
        return 1;
    }

    printf("Case created successfully!\n\n");
    printf("  ID: %s\n", new_case->id.value);
    printf("  Number: %s\n", new_case->case_number);
    printf("  Title: %s\n", new_case->title);
    printf("  Type: %s\n", type_str);
    printf("\n");

    regislex_case_free(new_case);
    return 0;
}

/**
 * @brief Case show command
 */
static int cmd_case_show(regislex_context_t* ctx, int argc, char** argv) {
    if (argc < 1) {
        printf("Usage: regislex-cli case-show <case-id>\n");
        return 1;
    }

    if (!ctx) {
        fprintf(stderr, "Error: Not connected to database. Run 'regislex-cli init' first.\n");
        return 1;
    }

    regislex_uuid_t id;
    strncpy(id.value, argv[0], sizeof(id.value) - 1);

    regislex_case_t* c = NULL;
    regislex_error_t err = regislex_case_get(ctx, &id, &c);

    if (err == REGISLEX_ERROR_NOT_FOUND) {
        printf("Case not found: %s\n", argv[0]);
        return 1;
    } else if (err != REGISLEX_OK) {
        fprintf(stderr, "Error retrieving case\n");
        return 1;
    }

    printf("Case Details\n");
    printf("============\n\n");
    printf("ID:              %s\n", c->id.value);
    printf("Case Number:     %s\n", c->case_number);
    printf("Title:           %s\n", c->title);
    if (c->short_title[0]) printf("Short Title:     %s\n", c->short_title);
    printf("Type:            %d\n", c->type);
    printf("Status:          %d\n", c->status);
    printf("Priority:        %d\n", c->priority);
    if (c->description[0]) printf("\nDescription:\n%s\n", c->description);
    printf("\n");

    regislex_case_free(c);
    return 0;
}

/**
 * @brief Deadline list command
 */
static int cmd_deadline_list(regislex_context_t* ctx, int argc, char** argv) {
    (void)ctx;
    (void)argc;
    (void)argv;

    printf("Deadlines\n");
    printf("=========\n\n");
    printf("%-36s  %-30s  %-20s  %-10s\n", "ID", "Title", "Due Date", "Status");
    printf("%-36s  %-30s  %-20s  %-10s\n",
           "------------------------------------",
           "------------------------------",
           "--------------------",
           "----------");

    /* TODO: Implement deadline listing */
    printf("\n(Deadline listing not yet implemented)\n");

    return 0;
}

/**
 * @brief Deadline upcoming command
 */
static int cmd_deadline_upcoming(regislex_context_t* ctx, int argc, char** argv) {
    int days = 7;

    for (int i = 0; i < argc; i++) {
        if ((strcmp(argv[i], "--days") == 0 || strcmp(argv[i], "-d") == 0) && i + 1 < argc) {
            days = atoi(argv[++i]);
        }
    }

    (void)ctx;

    printf("Upcoming Deadlines (next %d days)\n", days);
    printf("=================================\n\n");

    /* TODO: Implement upcoming deadlines */
    printf("(Not yet implemented)\n");

    return 0;
}

/**
 * @brief Document list command
 */
static int cmd_document_list(regislex_context_t* ctx, int argc, char** argv) {
    (void)ctx;
    (void)argc;
    (void)argv;

    printf("Documents\n");
    printf("=========\n\n");

    /* TODO: Implement document listing */
    printf("(Not yet implemented)\n");

    return 0;
}

/**
 * @brief Report generate command
 */
static int cmd_report_generate(regislex_context_t* ctx, int argc, char** argv) {
    if (argc < 1) {
        printf("Usage: regislex-cli report <type> [--format <format>]\n\n");
        printf("Report Types:\n");
        printf("  caseload       Caseload summary report\n");
        printf("  performance    Attorney performance report\n");
        printf("  deadline       Deadline compliance report\n");
        printf("  aging          Case aging report\n");
        printf("  financial      Financial summary report\n");
        printf("\nFormats: pdf, html, csv, json (default: pdf)\n");
        return 1;
    }

    (void)ctx;

    const char* report_type = argv[0];
    const char* format = "pdf";

    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "--format") == 0 || strcmp(argv[i], "-f") == 0) && i + 1 < argc) {
            format = argv[++i];
        }
    }

    printf("Generating %s report in %s format...\n", report_type, format);

    /* TODO: Implement report generation */
    printf("\n(Report generation not yet implemented)\n");

    return 0;
}

/**
 * @brief Main entry point for CLI
 */
int main(int argc, char** argv) {
    if (argc < 2) {
        cmd_help(NULL, 0, NULL);
        return 0;
    }

    const char* cmd_name = argv[1];
    const cli_command_t* cmd = find_command(cmd_name);

    if (!cmd) {
        fprintf(stderr, "Unknown command: %s\n\n", cmd_name);
        fprintf(stderr, "Run 'regislex-cli help' for a list of available commands.\n");
        return 1;
    }

    /* Initialize context for commands that need database access */
    regislex_context_t* ctx = NULL;

    if (strcmp(cmd_name, "help") != 0 &&
        strcmp(cmd_name, "version") != 0 &&
        strcmp(cmd_name, "init") != 0)
    {
        regislex_config_t config;
        regislex_config_default(&config);

        if (platform_file_exists(config.database.database)) {
            regislex_error_t err = regislex_init(&config, &ctx);
            if (err != REGISLEX_OK) {
                /* Proceed without context - command will handle it */
                ctx = NULL;
            }
        }
    }

    /* Execute command */
    int result = cmd->func(ctx, argc - 2, &argv[2]);

    /* Cleanup */
    if (ctx) {
        regislex_shutdown(ctx);
    }

    return result;
}
