#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

using namespace std;

enum class LogLevel { INFO = 0, DEBUG, ERROR };

class Appender { // where to add/expose the log

public:
  virtual void append(const string &msg) = 0;

  virtual ~Appender() = default;
};

class ConsoleAppender : public Appender {
public:
  void append(const string &msg) override { cout << msg << '\n'; }
};

class FileAppender : public Appender {

  ofstream ofs;

public:
  FileAppender(const string &filename) { ofs.open(filename, ios::app); }

  void append(const string &msg) override { ofs << msg << '\n'; }

  ~FileAppender() { ofs.close(); }
};

// Open for future extension: KafkaAppender, DatabaseAppender, SocketAppender
// etc. class KafkaAppender : public Appender {}; class DatabaseAppender :
// public Appender {};

class Logger {

  vector<Appender *> appenders;
  LogLevel minLevel;

  mutex mtx;

  string enumToString(LogLevel level) {

    switch (level) {
    case LogLevel::INFO:
      return "INFO";
    case LogLevel::DEBUG:
      return "DEBUG";
    case LogLevel::ERROR:
      return "ERROR";
    }
    return "";
  }

  string currentTime() {

    auto now = chrono::system_clock::now();
    time_t t = chrono::system_clock::to_time_t(now);

    string ts = ctime(&t);
    ts.pop_back(); // VVIMP:remove '\n'

    return ts;
  }

public:
  Logger(LogLevel level = LogLevel::INFO) : minLevel(level) {}

  void addAppender(Appender *appender) { appenders.push_back(appender); }

  void log(LogLevel level, const string &msg) {

    if ((int)level < (int)minLevel)
      return;

    lock_guard<mutex> lock(mtx);

    string entry = "[" + currentTime() +
                   "] "
                   "[" +
                   enumToString(level) + "] " + msg;

    for (auto &appender : appenders)
      // polymorphism, Appender can be ConsoleAppender/ FileAppender/
      // KafkaAppender logger remains unchanged, using SuperClass Appender
      appender->append(entry);
  }
};

int main() {

  Logger logger(LogLevel::DEBUG);

  ConsoleAppender console;
  FileAppender file("append.log");

  logger.addAppender(&console);
  logger.addAppender(&file);

  logger.log(LogLevel::INFO,
             "Application Started"); // < provided minLevel, so filtered out
  logger.log(LogLevel::DEBUG, "Connecting DB");
  logger.log(LogLevel::ERROR, "Connection failed");

  return 0;
}

/*
 Problem Statement:----------- Design a logger that:
Accepts log messages with different levels: [INFO, DEBUG, ERROR]
Sends the log to one or more destinations: Console, File, (Future: Database,
Kafka, Network, etc.) Allows adding new destinations without modifying the
Logger itself. Any destination capable of receiving logs should implement
append()

                    Logger
         --------------------------------
         | minLevel                     |
         | vector<Appender*>            |
         | mutex                        |
         --------------------------------
                     |
      -----------------------------------------
      |                  |                   |
ConsoleAppender     FileAppender        KafkaAppender
      |                  |                   |
 stdout               app.log              Kafka

The code implements a publish-once, write-to-many logging system where Logger
generates messages and different Appenders decide where those messages are
stored or displayed.

Rule of thumb:
system_clock → timestamps, files, logs, calendars.
steady_clock → durations, latency measurement, timeout handling, benchmarking.

 Output:
[Wed Jun 24 17:05:44 2026] [DEBUG] Connecting DB
[Wed Jun 24 17:05:44 2026] [ERROR] Connection failed
*/
