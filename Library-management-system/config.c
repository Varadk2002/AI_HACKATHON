#include "config.h"
#include <string.h>

// Define the global configuration variable
Config cfg;

// Function to initialize the configuration
void config_init() {
    strncpy(cfg.appname, "Simple Library Management System", sizeof(cfg.appname));
    cfg.max_borrow_days = 14;
    cfg.daily_fine = 0.50;
}