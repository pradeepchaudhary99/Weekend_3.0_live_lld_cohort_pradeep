// Functional:
//     1. Submit a task for immediate execution
//     2. Schedule a tak to run on repeat every
//     3. support task priority so higher-priority
//     4. cancel a pedning task before it executes
//     5. Every Task will have multiple states : PENDING, RUNNING, COMPLETED, FAILED, CANCELLED...
//     6. Execution must be asynchronous, submission happens immediately, work happens on a worker pool.
//
// Non Functional Requirements:
//     1. No busy-polling
//     2. No double execution
//     3. Exetensibility: multiple new types of task support

// Core Entities:
// TaskAction
// ScheduledTask
// TaskStatus
//
// RecuurenceType ---> ONE_TIME, FIXED_DELAY

class TaskSchedularDemo {};
