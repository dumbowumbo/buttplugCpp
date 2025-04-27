#include <fstream>
#include <queue>
#include <mutex>
#include <string>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <filesystem>

// Structure to store information about a log entry
struct LogEntry {
    std::chrono::system_clock::time_point timestamp;
    std::string messageType;
    std::string direction; 
    unsigned int messageId;
};

// Logger class that handles message logging in a thread-safe way
// Uses a separate thread to write log entries to avoid blocking the main thread
class Logger {
public:
    // Constructor that sets up the logger with filename and max file size
    Logger(const std::string baseFilename = "log", size_t maxFileSize = 102400);
    ~Logger();

    // Start the logging system with default or custom filename
    void start();
    void start(const std::string& baseFilenameOverride);
    
    // Stop the logging system and clean up resources
    void stop();
    
    // Thread-safe methods to log sent and received messages
    void logSentMessage(const std::string& messageType, unsigned int messageId);
    void logReceivedMessage(const std::string& messageType, unsigned int messageId);
    void logOkMessage(const std::string& messageType, unsigned int messageId);
    void logErrorMessage(const std::string& messageType, unsigned int messageId, const std::string& errorMessage);

private:
    // Background thread processing function that writes queued log entries to file
    void processLogQueue();
    
    // Check if log file needs rotation based on size
    void checkAndRotateFile();
    
    // Generate a unique log filename based on date, time and counter
    std::string generateFilename() const;
    
    // Write a single log entry to the file
    void writeEntryToFile(const LogEntry& entry);

    // Queue for storing log entries awaiting processing
    std::queue<LogEntry> logQueue;
    
    // Synchronization primitives
    std::mutex queueMutex;
    std::mutex fileMutex;
    std::condition_variable queueCondition;
    
    // Log file and properties
    std::ofstream logFile;
    std::string baseFilename;
    size_t maxFileSize;
    size_t currentFileSize;
    unsigned int fileCounter;
    
    // Flag to control background thread operation
    std::atomic<bool> running;
    std::thread processingThread;

    // Timestamp when logging started
    std::chrono::system_clock::time_point startTime;
};