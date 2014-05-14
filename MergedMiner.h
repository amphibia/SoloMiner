#pragma once

#include <atomic>
#include <mutex>
#include <queue>
#include <string>

class MergedMiner {
public:
  MergedMiner();
  uint32_t getBlockCount() const;
  std::string getMessage();
  bool mine(std::string address1, std::string wallet1, std::string address2, std::string wallet2, size_t threads);
  void start();
  void stop();

private:
  std::atomic<uint32_t> m_blockCount;
  std::atomic<bool> m_stopped;
  std::queue<std::string> m_messages;
  std::mutex m_mutex;
};
