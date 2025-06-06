#include <napi.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstdio> // For popen/pclose or pipes directly

// For fork(), pipe(), close(), read(), write(), waitpid(), _exit()
#include <unistd.h>
#include <sys/wait.h>
#include <string.h> // For strerror

// A simple example function to be run in the child process
std::string performChildWork(int input) {
    long long result = (long long)input * input * input; // Simulate some work
    return "Child result for " + std::to_string(input) + ": " + std::to_string(result);
}

Napi::Value ForkAndRun(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Number expected").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    int input_value = info[0].As<Napi::Number>().Int32Value();

    // Create a pipe for communication between parent and child
    // pipefd[0] is for reading, pipefd[1] is for writing
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        Napi::Error::New(env, "Failed to create pipe: " + std::string(strerror(errno))).ThrowAsJavaScriptException();
        return env.Undefined();
    }

    pid_t pid = fork();

    if (pid == -1) {
        // Fork failed
        close(pipefd[0]);
        close(pipefd[1]);
        Napi::Error::New(env, "Failed to fork process: " + std::string(strerror(errno))).ThrowAsJavaScriptException();
        return env.Undefined();
    } else if (pid == 0) {
        // This is the CHILD process
        close(pipefd[0]); // Close read end in child

        std::string child_output = performChildWork(input_value);
        std::cout << "[Child " << getpid() << "] Performing work for: " << input_value << std::endl;
        std::cout << "[Child " << getpid() << "] Result: " << child_output << std::endl;

        // Write the result to the pipe
        if (write(pipefd[1], child_output.c_str(), child_output.length()) == -1) {
            std::cerr << "[Child " << getpid() << "] Error writing to pipe: " << strerror(errno) << std::endl;
        }
        close(pipefd[1]); // Close write end in child

        // Crucial: Use _exit() to terminate the child, avoiding V8/Node.js cleanup
        // which might be in an inconsistent state after fork.
        _exit(0); // Exit with code 0 for success
    } else {
        // This is the PARENT process
        close(pipefd[1]); // Close write end in parent

        // We use a Promise to handle the asynchronous nature of waiting for the child
        // and reading from the pipe.
        Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);

        // Create a thread-safe function to resolve/reject the promise from a separate thread
        // or a libuv async handle.
        // For simplicity, we'll block here to demonstrate, but in a real app,
        // you'd use a libuv worker or async handle to avoid blocking the Node.js event loop.

        // --- WARNING: THIS PART BLOCKS THE NODE.JS EVENT LOOP! ---
        // For a real application, you would use a libuv thread pool or async handle
        // to wait for the child and read from the pipe.
        // Example: Napi::AsyncWorker, Napi::AsyncContext, Napi::ThreadSafeFunction.
        // For this demo, we'll block briefly for simplicity and demonstration.
        std::cout << "[Parent " << getpid() << "] Forked child with PID: " << pid << std::endl;

        char buffer[256];
        ssize_t bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1);
        if (bytesRead == -1) {
            deferred.Reject(Napi::Error::New(env, "Error reading from pipe: " + std::string(strerror(errno))).Value());
        } else {
            buffer[bytesRead] = '\0'; // Null-terminate the string
            std::string childResult(buffer);

            int status;
            pid_t waited_pid = waitpid(pid, &status, 0); // Wait for the child to exit
            if (waited_pid == -1) {
                std::cerr << "[Parent " << getpid() << "] Error waiting for child: " << strerror(errno) << std::endl;
                deferred.Reject(Napi::Error::New(env, "Error waiting for child").Value());
            } else if (WIFEXITED(status)) {
                int exit_code = WEXITSTATUS(status);
                std::cout << "[Parent " << getpid() << "] Child " << waited_pid << " exited with status: " << exit_code << std::endl;
                deferred.Resolve(Napi::String::New(env, "Child (" + std::to_string(pid) + ") result: " + childResult + " (exit code: " + std::to_string(exit_code) + ")"));
            } else {
                std::cout << "[Parent " << getpid() << "] Child " << waited_pid << " exited abnormally." << std::endl;
                deferred.Reject(Napi::Error::New(env, "Child exited abnormally").Value());
            }
        }
        close(pipefd[0]); // Close read end in parent after reading

        return deferred.Promise();
    }
}

// Initialize the addon
Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set(Napi::String::New(env, "forkAndRun"),
                Napi::Function::New(env, ForkAndRun));
    return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)
