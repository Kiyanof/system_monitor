#ifndef SYSTEM_MONITOR_H
#define SYSTEM_MONITOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/signal.h>

#include "cpu_monitor.h"
#include "memory_monitor.h"
#include "disk_monitor.h"
#include "process_monitor.h"
#include "monitor_config.h"

// Function declarations
void signal_handler(int signum);

#endif // SYSTEM_MONITOR_H 