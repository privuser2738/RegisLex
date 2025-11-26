/**
 * @file main.c
 * @brief RegisLex Server Main Entry Point
 *
 * Main entry point for the RegisLex legal software server.
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/* Global context for signal handler */
static regislex_context_t* g_ctx = NULL;
static volatile int g_running = 1;

/**
 * @brief Signal handler for graceful shutdown
 */
static void signal_handler(int sig) {
    (void)sig;
    printf("\nShutdown signal received...\n");
    g_running = 0;
}

/**
 * @brief Print usage information
 */
static void print_usage(const char* program) {
    printf("RegisLex - Enterprise Legal Software Suite v%s\n\n", regislex_version());
    printf("Usage: %s [options]\n\n", program);
    printf("Options:\n");
    printf("  -c, --config <file>   Configuration file path\n");
    printf("  -d, --data-dir <dir>  Data directory path\n");
    printf("  -p, --port <port>     Server port (default: 8080)\n");
    printf("  -h, --help            Show this help message\n");
    printf("  -v, --version         Show version information\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s                          Start with default configuration\n", program);
    printf("  %s -c /etc/regislex.conf    Start with custom config file\n", program);
    printf("  %s -p 9000                  Start on port 9000\n", program);
    printf("\n");
}

/**
 * @brief Print version information
 */
static void print_version(void) {
    printf("RegisLex v%s\n", regislex_version());
    printf("Enterprise Legal Software Suite\n");
    printf("\n");
    printf("Features:\n");
    printf("  - Case Management\n");
    printf("  - Deadline Management\n");
    printf("  - Workflow Automation\n");
    printf("  - Document Management\n");
    printf("  - Reporting & Analytics\n");
    printf("  - Legislative Tracking\n");
    printf("  - Enterprise Legal Management (ELM)\n");
    printf("\n");
    printf("Platforms: Windows, Linux, macOS, Android\n");
    printf("\n");
}

/**
 * @brief Parse command line arguments
 */
static int parse_args(int argc, char** argv, regislex_config_t* config) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return -1;
        }

        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            print_version();
            return -1;
        }

        if ((strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--config") == 0) && i + 1 < argc) {
            regislex_error_t err = regislex_config_load(argv[++i], config);
            if (err != REGISLEX_OK) {
                fprintf(stderr, "Error: Failed to load configuration from %s\n", argv[i]);
                return 1;
            }
            continue;
        }

        if ((strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--data-dir") == 0) && i + 1 < argc) {
            strncpy(config->data_dir, argv[++i], sizeof(config->data_dir) - 1);
            continue;
        }

        if ((strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) && i + 1 < argc) {
            config->server.port = atoi(argv[++i]);
            if (config->server.port <= 0 || config->server.port > 65535) {
                fprintf(stderr, "Error: Invalid port number\n");
                return 1;
            }
            continue;
        }

        fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
        print_usage(argv[0]);
        return 1;
    }

    return 0;
}

/**
 * @brief Main entry point
 */
int main(int argc, char** argv) {
    regislex_config_t config;
    regislex_error_t err;
    int result = 0;

    printf("========================================\n");
    printf("   RegisLex v%s\n", regislex_version());
    printf("   Enterprise Legal Software Suite\n");
    printf("========================================\n\n");

    /* Initialize default configuration */
    err = regislex_config_default(&config);
    if (err != REGISLEX_OK) {
        fprintf(stderr, "Error: Failed to initialize configuration\n");
        return 1;
    }

    /* Parse command line arguments */
    int parse_result = parse_args(argc, argv, &config);
    if (parse_result != 0) {
        return (parse_result < 0) ? 0 : parse_result;
    }

    /* Set up signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
#ifndef REGISLEX_PLATFORM_WINDOWS
    signal(SIGHUP, signal_handler);
#endif

    printf("Initializing RegisLex...\n");
    printf("  Data directory: %s\n", config.data_dir);
    printf("  Database: %s (%s)\n", config.database.database, config.database.type);
    printf("  Server: %s:%d\n", config.server.host, config.server.port);
    printf("\n");

    /* Initialize RegisLex */
    err = regislex_init(&config, &g_ctx);
    if (err != REGISLEX_OK) {
        fprintf(stderr, "Error: Failed to initialize RegisLex: %s\n",
                g_ctx ? regislex_get_error(g_ctx) : "Unknown error");
        return 1;
    }

    printf("RegisLex initialized successfully.\n");
    printf("\n");

    /* Print server status */
    printf("Server Status:\n");
    printf("  Database: Connected\n");
    printf("  API Server: Starting on port %d...\n", config.server.port);
    printf("\n");

    /* TODO: Start HTTP server here */
    /* For now, just run a simple event loop */

    printf("RegisLex is now running.\n");
    printf("Press Ctrl+C to stop.\n");
    printf("\n");

    /* Main event loop */
    while (g_running) {
        /* Process events, handle requests, etc. */
        platform_sleep_ms(100);

        /* TODO: Implement actual server loop */
    }

    printf("Shutting down RegisLex...\n");

    /* Shutdown */
    regislex_shutdown(g_ctx);
    g_ctx = NULL;

    printf("RegisLex stopped.\n");

    return result;
}
