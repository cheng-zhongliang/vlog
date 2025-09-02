# vlog
A small, flexible and comprehensive logging library

## Features

- Multiple log levels (TRACE, DEBUG, INFO, WARN, ERROR, FATAL)
- Customizable output callbacks
- Thread-safe logging with optional locking
- File and console output support
- **Rate limiting** to prevent log flooding
- Configurable quiet mode and log level filtering

## Rate Limiting

vlog now includes rate limiting functionality to prevent excessive logging from the same code location:

```c
#include "vlog.h"

int main() {
    // Enable rate limiting - max one log per second from same location
    vlog_set_rate_limit(1.0);
    
    // Only the first message will appear, others are rate limited
    for(int i = 0; i < 5; i++) {
        vlog_info("This message is rate limited");
    }
    
    // Different locations are not affected by each other's rate limits
    vlog_warn("This warning will appear");
    
    // Disable rate limiting
    vlog_set_rate_limit(0.0);
    
    return 0;
}
```

### Rate Limiting API

- `vlog_set_rate_limit(double interval_seconds)` - Set rate limiting interval in seconds (0.0 to disable)

Rate limiting is applied per unique code location (file:line combination), so logs from different lines or functions won't interfere with each other.
