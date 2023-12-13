To create a web server in C using the provided threadpool API, we need to design and implement several components. Here's a high-level overview of how you can structure your web server:

### 1. Server Initialization
- Parse the command line arguments to get the port number.
- Initialize network components (like socket, bind, listen).
- Create three threadpools for handling different tasks: `read_msg_threadpool`, `read_file_threadpool`, and `send_msg_threadpool`.

### 2. Threadpool Usage
- `read_msg_threadpool`: Handles incoming HTTP requests.
- `read_file_threadpool`: Handles file reading operations for requests.
- `send_msg_threadpool`: Handles sending responses back to the client.

### 3. Main Server Loop
- The server enters a loop where it waits for client connections.
- Upon accepting a connection, it dispatches the request to the `read_msg_threadpool`.

### 4. Handling Requests
- A thread from `read_msg_threadpool` processes the request, parsing the HTTP headers and determining the required action (e.g., serving a file).
- If a file needs to be read, the task is sent to `read_file_threadpool`.
- Once the file is read, or if the request is for a simple message, the response preparation is handed over to the `send_msg_threadpool`.

### 5. Sending Responses
- The `send_msg_threadpool` prepares the HTTP response (including headers and content) and sends it back to the client.

### 6. Server Shutdown
- Implement a graceful shutdown procedure to close sockets, and destroy threadpools.

### Sample Code Structure

```c
#include "thpool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
// ... other necessary includes

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [port_number]\n", argv[0]);
        return 1;
    }
    int port = atoi(argv[1]);

    // Initialize threadpools
    threadpool read_msg_pool = thpool_init(4);
    threadpool read_file_pool = thpool_init(4);
    threadpool send_msg_pool = thpool_init(4);

    // Initialize server socket
    // ...

    while (1) {
        // Accept connection
        // ...

        // Dispatch to read_msg_pool
        thpool_add_work(read_msg_pool, handle_request, /* args */);
    }

    // Cleanup
    thpool_destroy(read_msg_pool);
    thpool_destroy(read_file_pool);
    thpool_destroy(send_msg_pool);
    // Close socket, free resources
}

void handle_request(void *args) {
    // Process request, read file if needed, send response
    // Use read_file_pool and send_msg_pool as needed
}
```

### Additional Notes
- Ensure proper synchronization (mutexes, condition variables) when accessing shared resources.
- Handle errors and edge cases, such as client disconnections and malformed requests.
- Test thoroughly under various conditions to ensure stability and performance.

This is a simplified outline. Building a fully functional web server involves many more details, especially concerning HTTP protocol handling, error management, and resource allocation.