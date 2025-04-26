#include "../include/log.h"
#include <iomanip>
#include <sstream>

Logger::Logger(const std::string baseFilename, size_t maxFileSize)
    : baseFilename(baseFilename)
    , maxFileSize(maxFileSize)
    , currentFileSize(0)
    , fileCounter(0)
    , running(false)
    , startTime(std::chrono::system_clock::now())
{
}

Logger::~Logger() {
    stop();
}

void Logger::start() {
    running = true;
    logFile.open(generateFilename(), std::ios::out | std::ios::app);
    processingThread = std::thread(&Logger::processLogQueue, this);
}

void Logger::start(const std::string& baseFilenameOverride) {
    baseFilename = baseFilenameOverride;
	start();
}

void Logger::stop() {
    running = false;
    if (processingThread.joinable()) {
		queueCondition.notify_one();
        processingThread.join();
    }
    if (logFile.is_open()) {
        logFile.close();
    }
}

void Logger::logSentMessage(const std::string& messageType, unsigned int messageId) {
    LogEntry entry{
        std::chrono::system_clock::now(),
        messageType,
        "SENT",
        messageId
    };
    
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        logQueue.push(entry);
    }
    queueCondition.notify_one();
}

void Logger::logReceivedMessage(const std::string& messageType, unsigned int messageId) {
    LogEntry entry{
        std::chrono::system_clock::now(),
        messageType,
        "RECEIVED",
        messageId
    };
    
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        logQueue.push(entry);
    }
    queueCondition.notify_one();
}

void Logger::processLogQueue() {
    while (running) {
        LogEntry entry;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCondition.wait(lock, [this]() { 
                return !logQueue.empty() || !running; 
            });
            
            if (!running && logQueue.empty()) {
                return;
            }
            
            entry = logQueue.front();
            logQueue.pop();
        }
        
        writeEntryToFile(entry);
    }
}

void Logger::checkAndRotateFile() {
    if (currentFileSize >= maxFileSize) {
        if (logFile.is_open()) {
            logFile.close();
        }
        fileCounter++;
        logFile.open(generateFilename(), std::ios::out | std::ios::app);
        currentFileSize = 0;
    }
}

std::string Logger::generateFilename() const {
    std::stringstream ss;
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    ss << baseFilename << "_" 
       << std::put_time(std::localtime(&now_c), "%Y%m%d_%H%M%S")
       << "_" << fileCounter << ".log";
    return ss.str();
}

void Logger::writeEntryToFile(const LogEntry& entry) {
    std::lock_guard<std::mutex> lock(fileMutex);
    
    if (!logFile.is_open()) {
        logFile.open(generateFilename(), std::ios::out | std::ios::app);
    }

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        entry.timestamp - startTime);
    
    std::stringstream ss;
    auto timestamp_c = std::chrono::system_clock::to_time_t(entry.timestamp);
    
    ss << std::put_time(std::localtime(&timestamp_c), "%Y-%m-%d %H:%M:%S")
       << "." << std::setfill('0') << std::setw(3) 
       << duration.count() % 1000 << " [" 
       << entry.direction << "] " 
       << entry.messageType << " (ID: " 
       << entry.messageId << ")\n";
    
    logFile << ss.str();
    logFile.flush();
    
    currentFileSize += ss.str().length();
    checkAndRotateFile();
}