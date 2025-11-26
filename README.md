# RegisLex

**Enterprise Legal Software Suite**

RegisLex is a robust, cross-platform legal software suite designed for government legal departments, attorneys, and agencies. It provides comprehensive case management, deadline tracking, workflow automation, document management, reporting, legislative tracking, and enterprise legal management capabilities.

## Features

### Case Management
- Full case lifecycle management (create, update, track, close)
- Party management (plaintiffs, defendants, witnesses, counsel)
- Court and docket information tracking
- Matter/sub-case support for complex litigation
- Custom metadata and tagging

### Deadline Management
- Court dates and filing deadline tracking
- Statute of limitations monitoring
- Automated reminder system (email, SMS, in-app)
- Business day calculations with holiday support
- Calendar synchronization (Google, Outlook, iCal)

### Workflow Automation
- Customizable workflow templates
- Trigger-based automation (events, schedules, conditions)
- Task assignment and approval workflows
- Document generation automation
- Email and notification actions

### Reporting & Analytics
- Caseload summary reports
- Attorney performance metrics
- Deadline compliance tracking
- Financial summaries
- Custom report builder
- Dashboard widgets
- Scheduled report delivery

### Document Management
- Secure document storage (filesystem, S3, Azure, GCS)
- Version control with full history
- Document templates with field population
- Full-text search with OCR support
- Digital signatures
- Check-in/check-out locking

### Legislative Tracking
- Federal, state, and local legislation tracking
- Regulation monitoring (Regulations.gov integration)
- Bill status alerts and notifications
- Stakeholder relationship management
- Position tracking and engagement logging

### Enterprise Legal Management (ELM)
- Outside counsel management
- eBilling with LEDES support
- Matter budgeting and variance analysis
- Contract lifecycle management
- Risk register and mitigation tracking
- Legal spend analytics

## Supported Platforms

- Windows (x64)
- Linux (x64, ARM64)
- macOS (x64, ARM64)
- Android (ARM64)

## Building

### Prerequisites

- CMake 3.16 or later
- C11 compatible compiler (GCC, Clang, MSVC)
- C++17 compatible compiler (optional, for some components)

### Dependencies

- SQLite 3.x (bundled or system)
- OpenSSL (optional, for encryption features)

### Build Steps

```bash
# Clone the repository
git clone https://github.com/your-org/regislex.git
cd regislex

# Download SQLite amalgamation (if not using system SQLite)
# Place sqlite3.c and sqlite3.h in third_party/sqlite/

# Create build directory
mkdir build && cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --config Release

# Install (optional)
cmake --install . --prefix /usr/local
```

### Build Options

```cmake
REGISLEX_BUILD_TESTS      # Build unit tests (default: ON)
REGISLEX_BUILD_SHARED     # Build shared library (default: ON)
REGISLEX_ENABLE_SSL       # Enable SSL/TLS support (default: ON)
REGISLEX_USE_SYSTEM_SQLITE # Use system SQLite (default: OFF)
```

## Quick Start

### Initialize

```bash
# Initialize RegisLex (creates database and directories)
regislex-cli init
```

### Create a Case

```bash
# Create a new case
regislex-cli case-create --number "2024-CV-001" --title "Smith v. Jones" --type civil
```

### List Cases

```bash
# List all cases
regislex-cli case-list

# Filter by status
regislex-cli case-list --status active
```

### Start Server

```bash
# Start the RegisLex server
regislex -p 8080
```

## API

RegisLex provides a C API for integration:

```c
#include <regislex/regislex.h>

int main() {
    regislex_context_t* ctx = NULL;
    regislex_config_t config;

    // Initialize with default configuration
    regislex_config_default(&config);
    regislex_init(&config, &ctx);

    // Create a case
    regislex_case_t case_data = {0};
    strcpy(case_data.case_number, "2024-CV-001");
    strcpy(case_data.title, "Smith v. Jones");
    case_data.type = REGISLEX_CASE_TYPE_CIVIL;
    case_data.status = REGISLEX_STATUS_ACTIVE;

    regislex_case_t* new_case = NULL;
    regislex_case_create(ctx, &case_data, &new_case);

    printf("Created case: %s\n", new_case->id.value);

    regislex_case_free(new_case);
    regislex_shutdown(ctx);
    return 0;
}
```

## Directory Structure

```
regislex/
├── include/
│   ├── regislex/           # Public API headers
│   │   ├── regislex.h      # Main header
│   │   └── modules/        # Module headers
│   ├── platform/           # Platform abstraction
│   └── database/           # Database abstraction
├── src/
│   ├── core/               # Core implementation
│   ├── platform/           # Platform-specific code
│   ├── database/           # Database drivers
│   ├── modules/            # Feature modules
│   ├── api/                # REST API
│   └── cli/                # CLI tool
├── third_party/
│   └── sqlite/             # SQLite amalgamation
├── tests/                  # Unit and integration tests
├── docs/                   # Documentation
└── resources/              # Configuration templates, etc.
```

## Configuration

RegisLex can be configured via configuration file:

```ini
[app]
name = RegisLex
data_dir = /var/lib/regislex
log_dir = /var/log/regislex
log_level = info

[database]
type = sqlite
database = /var/lib/regislex/regislex.db
pool_size = 5
timeout_seconds = 30

[server]
host = 0.0.0.0
port = 8080
use_ssl = false
max_connections = 100

[storage]
type = filesystem
base_path = /var/lib/regislex/documents
encryption_enabled = true
max_file_size = 104857600
```

## License

MIT License - see LICENSE file for details.

## Contributing

Contributions are welcome! Please read CONTRIBUTING.md for guidelines.

## Support

- Documentation: https://regislex.example.com/docs
- Issues: https://github.com/your-org/regislex/issues
- Email: support@regislex.example.com
