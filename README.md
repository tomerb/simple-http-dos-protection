A simple HTTP server that tries to provide a basic DoS protection.


**General architecture**

The server is using a safe (blocking) message queue to handle incoming traffic. Each client request is put on the queue, and the context is released to handle the next incoming connection.

The server maintains a thread pool to asynchronously handle requests that are taken out of the message queue. The number of threads is determined by the number of cores that exist on the system the program is running on.

A DoS clients runtime is also provided, mostly for sake of testing/experimenting.
