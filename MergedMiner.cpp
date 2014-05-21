#include "MergedMiner.h"
#include <future>
#include <include_base_utils.h>
#include "net/http_client.h"
#include "storages/http_abstract_invoke.h"
#include "cryptonote_core/cryptonote_format_utils.h"
#include "rpc/core_rpc_server_commands_defs.h"

class Miner {
public:
  Miner() : m_stopped(false) {
  }

  void start() {
    m_stopped = false;
  }

  void stop() {
    m_stopped = true;
  }

  bool findNonce(cryptonote::block* block, uint64_t height, cryptonote::difficulty_type difficulty, size_t threads, crypto::hash* hash) {
    block->nonce = 0;
    m_block = block;
    m_difficulty = difficulty;
    m_hashes = 0;
    m_height = height;
    m_nonce = 0;
    m_nonceFound = false;
    m_startedThreads = 1;
    m_threads = threads;
    uint8_t* long_state = new uint8_t[(1 << 21) * threads];

    std::vector<std::future<crypto::hash>> futures;
    for (size_t i = 1; i < threads; ++i) {
      futures.emplace_back(std::async(std::launch::async, &Miner::threadProcedure, this, long_state + (1 << 21) * i));
    }

    *hash = findBlockNonce(*block, long_state);
    for (std::future<crypto::hash>& future : futures) {
      crypto::hash threadHash = future.get();
      if (cryptonote::null_hash != threadHash) {
        *hash = threadHash;
      }
    }

    delete[] long_state;
    block->nonce = m_nonce;
    return m_nonceFound;
  }

  uint64_t getHashCount() const {
    return m_hashes;
  }

private:
  cryptonote::block* m_block;
  cryptonote::difficulty_type m_difficulty;
  std::atomic<uint64_t> m_hashes;
  uint64_t m_height;
  std::atomic<uint32_t> m_nonce;
  std::atomic<bool> m_nonceFound;
  std::atomic<uint32_t> m_startedThreads;
  std::atomic<bool> m_stopped;
  size_t m_threads;

  crypto::hash threadProcedure(uint8_t* long_state) {
    cryptonote::block block = *m_block;
    block.nonce = m_startedThreads++;
    return findBlockNonce(block, long_state);
  }

  crypto::hash findBlockNonce(cryptonote::block& block, uint8_t* long_state) {
    crypto::hash blockHash = cryptonote::null_hash;
    while (!m_nonceFound && !m_stopped) {
      crypto::hash hash;
      bool result = get_block_longhash(block, hash, m_height, long_state);
      ++m_hashes;
      if (!result) {
        break;
      }

      if (cryptonote::check_hash(hash, m_difficulty)) {
        m_nonce = block.nonce;
        m_nonceFound = true;
        blockHash = hash;
        break;
      }

      if (std::numeric_limits<uint32_t>::max() - block.nonce < m_threads) {
        break;
      }

      block.nonce += static_cast<uint32_t>(m_threads);
    }

    return blockHash;
  }
};

const size_t TX_EXTRA_FIELD_TAG_BYTES = 1;
const size_t TX_MM_FIELD_SIZE_BYTES = 1;
const size_t MAX_VARINT_SIZE = 9;
const size_t TX_MM_TAG_MAX_BYTES = MAX_VARINT_SIZE + sizeof(crypto::hash);
const size_t MERGE_MINING_TAG_RESERVED_SIZE = TX_EXTRA_FIELD_TAG_BYTES  + TX_MM_FIELD_SIZE_BYTES + TX_MM_TAG_MAX_BYTES;

struct BlockTemplate {
  cryptonote::block block;
  uint64_t height;
  cryptonote::difficulty_type difficulty;
};

bool getBlockTemplate(epee::net_utils::http::http_simple_client* client, const std::string* address, const std::string* walletAddress, size_t extraNonceSize, BlockTemplate* blockTemplate) {
  cryptonote::COMMAND_RPC_GETBLOCKTEMPLATE::request request;
  request.reserve_size = extraNonceSize;
  request.wallet_address = *walletAddress;
  cryptonote::COMMAND_RPC_GETBLOCKTEMPLATE::response response;
  bool result = epee::net_utils::invoke_http_json_rpc(*address + "/json_rpc", "getblocktemplate", request, response, *client);
  if (!result || (!response.status.empty() && response.status != CORE_RPC_STATUS_OK)) {
    return false;
  }

  std::string blockString;
  epee::string_tools::parse_hexstr_to_binbuff(response.blocktemplate_blob, blockString);
  std::istringstream stringStream(blockString);
  binary_archive<false> archive(stringStream);
  if (!serialization::serialize(archive, blockTemplate->block)) {
    return false;
  }

  blockTemplate->difficulty = response.difficulty;
  blockTemplate->height = response.height;
  return true;
}

static bool fillExtra(cryptonote::block& block1, const cryptonote::block& block2) {
  std::vector<uint8_t>& extra = block1.miner_tx.extra;
  std::string extraAsString(reinterpret_cast<const char*>(extra.data()), extra.size());

  std::string extraNonceTemplate;
  extraNonceTemplate.push_back(TX_EXTRA_NONCE);
  extraNonceTemplate.push_back(MERGE_MINING_TAG_RESERVED_SIZE);
  extraNonceTemplate.append(MERGE_MINING_TAG_RESERVED_SIZE, '\0');

  size_t extraNoncePos = extraAsString.find(extraNonceTemplate);
  if (std::string::npos == extraNoncePos) {
    return false;
  }

  cryptonote::tx_extra_merge_mining_tag tag;
  tag.depth = 0;
  if (!cryptonote::get_block_header_hash(block2, tag.merkle_root)) {
    return false;
  }

  std::vector<uint8_t> extraNonceReplacement;
  if (!cryptonote::append_mm_tag_to_extra(extraNonceReplacement, tag)) {
    return false;
  }

  if (MERGE_MINING_TAG_RESERVED_SIZE < extraNonceReplacement.size()) {
    return false;
  }

  size_t diff = extraNonceTemplate.size() - extraNonceReplacement.size();
  if (0 < diff) {
    extraNonceReplacement.push_back(TX_EXTRA_NONCE);
    extraNonceReplacement.push_back(static_cast<uint8_t>(diff - 2));
  }

  std::copy(extraNonceReplacement.begin(), extraNonceReplacement.end(), extra.begin() + extraNoncePos);
  return true;
}

static bool mergeBlocks(const cryptonote::block& block1, cryptonote::block& block2) {
  block2.timestamp = block1.timestamp;
  block2.parent_block.major_version = block1.major_version;
  block2.parent_block.minor_version = block1.minor_version;
  block2.parent_block.prev_id = block1.prev_id;
  block2.parent_block.nonce = block1.nonce;
  block2.parent_block.miner_tx = block1.miner_tx;
  block2.parent_block.number_of_transactions = block1.tx_hashes.size() + 1;
  block2.parent_block.miner_tx_branch.resize(crypto::tree_depth(block1.tx_hashes.size() + 1));
  std::vector<crypto::hash> transactionHashes;
  transactionHashes.push_back(cryptonote::get_transaction_hash(block1.miner_tx));
  std::copy(block1.tx_hashes.begin(), block1.tx_hashes.end(), std::back_inserter(transactionHashes));
  tree_branch(transactionHashes.data(), transactionHashes.size(), block2.parent_block.miner_tx_branch.data());
  block2.parent_block.blockchain_branch.clear();
  return true;
}

static bool submitBlock(epee::net_utils::http::http_simple_client& client, const std::string& address, const cryptonote::block& block) {
  cryptonote::COMMAND_RPC_SUBMITBLOCK::request request;
  request.push_back(epee::string_tools::buff_to_hex_nodelimer(t_serializable_object_to_blob(block)));
  cryptonote::COMMAND_RPC_SUBMITBLOCK::response response;
  bool result = epee::net_utils::invoke_http_json_rpc(address + "/json_rpc", "submitblock", request, response, client);
  if (!result || (!response.status.empty() && response.status != CORE_RPC_STATUS_OK)) {
    return false;
  }

  return true;
}

MergedMiner::MergedMiner() : m_blockCount(0), m_stopped(false) {
}

uint32_t MergedMiner::getBlockCount() const {
  return m_blockCount;
}

std::string MergedMiner::getMessage() {
  std::string result;
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_messages.empty()) {
    result = m_messages.front();
    m_messages.pop();
  }

  return result;
}

bool MergedMiner::mine(std::string address1, std::string wallet1, std::string address2, std::string wallet2, size_t threads) {
  epee::net_utils::http::http_simple_client httpClient1;
  epee::net_utils::http::http_simple_client httpClient2;
  Miner miner;
  BlockTemplate blockTemplate1;
  BlockTemplate blockTemplate2;
  cryptonote::block block1;
  cryptonote::block block2;
  cryptonote::difficulty_type difficulty1;
  cryptonote::difficulty_type difficulty2;
  std::future<bool> request1;
  std::future<bool> request2;
  std::future<bool> mining;
  crypto::hash hash;
  std::chrono::steady_clock::time_point time1;

  uint64_t prefix1;
  cryptonote::account_public_address walletAddress1;
  if (!get_account_address_from_str(prefix1, walletAddress1, wallet1)) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_messages.push("Failed to parse donor wallet address");
    return false;
  }

  uint64_t prefix2;
  cryptonote::account_public_address walletAddress2;
  if (!address2.empty() && !get_account_address_from_str(prefix2, walletAddress2, wallet2)) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_messages.push("Failed to parse acceptor wallet address");
    return false;
  }

  m_blockCount = 0;
  for (;;) {
    miner.getHashCount();

    if (m_stopped) {
      if (mining.valid()) {
        miner.stop();
        mining.wait();
      }

      return true;
    }

    request1 = std::async(std::launch::async, getBlockTemplate, &httpClient1, &address1, &wallet1, MERGE_MINING_TAG_RESERVED_SIZE, &blockTemplate1);
    if (!address2.empty()) {
      request2 = std::async(std::launch::async, getBlockTemplate, &httpClient2, &address2, &wallet2, MERGE_MINING_TAG_RESERVED_SIZE, &blockTemplate2);
    }

    if (!request1.get()) {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_messages.push("Failed to get donor block");
      continue;
    }

    if (!address2.empty()) {
      if (!request2.get()) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_messages.push("Failed to get acceptor block");
        continue;
      }

      if (blockTemplate2.block.major_version != BLOCK_MAJOR_VERSION_2) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_messages.push("Unsupported block version received from acceptor network, merged mining is not possible");
        if (mining.valid()) {
          miner.stop();
          mining.wait();
        }

        return false;
      }
    }

    if (mining.valid()) {
      miner.stop();
      if (mining.get()) {
        if (cryptonote::check_hash(hash, difficulty1)) {
          if (submitBlock(httpClient1, address1, block1)) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_messages.push("Submitted donor block");
          } else {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_messages.push("Failed to submit donor block");
          }
        }

        if (!address2.empty() && cryptonote::check_hash(hash, difficulty2)) {
          if (!mergeBlocks(block1, block2)) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_messages.push("Internal error");
            return false;
          }

          if (submitBlock(httpClient2, address2, block2)) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_messages.push("Submitted acceptor block");
          } else {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_messages.push("Failed to submit acceptor block");
          }
        }
      }

      std::chrono::steady_clock::time_point time2 = std::chrono::steady_clock::now();
      std::ostringstream stream;
      stream << "Hashrate: " << miner.getHashCount() / std::chrono::duration_cast<std::chrono::duration<double>>(time2 - time1).count();
      std::lock_guard<std::mutex> lock(m_mutex);
      m_messages.push(stream.str());
      time1 = time2;

      miner.start();
    } else {
      time1 = std::chrono::steady_clock::now();
    }

    block1 = blockTemplate1.block;
    difficulty1 = blockTemplate1.difficulty;
    cryptonote::difficulty_type difficulty = difficulty1;
    if (!address2.empty()) {
      block2 = blockTemplate2.block;
      difficulty2 = blockTemplate2.difficulty;
      difficulty = std::min(difficulty, difficulty2);
      if (!fillExtra(block1, block2)) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_messages.push("Internal error");
        return false;
      }
    }

    mining = std::async(std::launch::async, &Miner::findNonce, &miner, &block1, blockTemplate1.height, difficulty, threads, &hash);
    for (size_t i = 0; i < 50; ++i) {
      if (m_stopped || mining.wait_for(std::chrono::milliseconds(100)) == std::future_status::ready) {
        break;
      }
    }
  }
}

void MergedMiner::start() {
  m_stopped = false;
}

void MergedMiner::stop() {
  m_stopped = true;
}
