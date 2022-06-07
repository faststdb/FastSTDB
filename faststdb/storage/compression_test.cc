/*!
 * \file compression_test.cc
 */
#include "faststdb/storage/compression.h"

#include <string.h>

#include <cstring>
#include <iostream>
#include <vector>
#include <random>

#include "gtest/gtest.h"
#include "faststdb/common/logging.h"

namespace faststdb {
namespace storage {

static const u64 EXPECTED[] = {
  0ul, 1ul, 10ul,
  67ul, 127ul, 128ul,
  1024ul, 10000ul,
  100000ul, 420000000ul,
  420000001ul
};

static const size_t EXPECTED_SIZE = sizeof(EXPECTED) / sizeof(u64);

template<class TStreamWriter>
void test_stream_write(TStreamWriter& writer) {
  // Encode
  for (auto i = 0u; i < EXPECTED_SIZE; i++) {
    if (!writer.put(EXPECTED[i])) {
      LOG(FATAL) << "Buffer is too small";
    }
  }
  writer.commit();

  const size_t USED_SIZE = writer.size();
  EXPECT_LT(USED_SIZE, sizeof(EXPECTED));
  EXPECT_GT(USED_SIZE, EXPECTED_SIZE);
}

template<class TVal, class TStreamWriter, class TStreamReader>
void test_stream_chunked_op(TStreamWriter& writer,
                            TStreamReader& reader, size_t nsteps,
                            bool sort_input = false,
                            bool fixed_step = false) {
  std::vector<TVal> input;
  const size_t step_size = 16;
  const size_t input_size = step_size * nsteps;
  TVal value = 100000;

  // Generate
  if (!fixed_step) {
    input.push_back(0);
    for (u32 i = 0; i < (input_size-1); i++) {
      int delta = rand() % 1000 - 500;
      value += TVal(delta);
      input.push_back(value);
    }
  } else {
    for (u32 i = 0; i < nsteps; i++) {
      int delta = rand() % 1000;  // all positive
      for (u32 j = 0; j < step_size; j++) {
        value += TVal(delta);
        input.push_back(value);
      }
    }
  }

  if (sort_input) {
    std::sort(input.begin(), input.end());
  }

  // Encode
  for (auto offset = 0u; offset < input_size; offset += step_size) {
    auto success = writer.tput(input.data() + offset, step_size);
    EXPECT_TRUE(success);
  }

  // Decode and compare results
  std::vector<TVal> results;
  for (auto offset = 0ul; offset < input_size; offset++) {
    auto next = reader.next();
    results.push_back(next);
  }

  EXPECT_TRUE(0 == std::memcmp(input.data(), results.data(), input_size * sizeof(TVal)));
}

template<class TStreamReader>
void test_stream_read(TStreamReader& reader) {
  // Read it back
  u64 actual[EXPECTED_SIZE];
  for (auto i = 0u; i < EXPECTED_SIZE; i++) {
    actual[i] = reader.next();
  }
  EXPECT_TRUE(0 == std::memcmp(EXPECTED, actual, EXPECTED_SIZE * sizeof(u64)));
}

//! Base128StreamReader::next is a template, so we need to specialize this function.
template<>
void test_stream_read(Base128StreamReader& reader) {
  // Read it back
  u64 actual[EXPECTED_SIZE];
  for (auto i = 0u; i < EXPECTED_SIZE; i++) {
    actual[i] = reader.template next<u64>();
  }
  EXPECT_TRUE(0 == std::memcmp(EXPECTED, actual, EXPECTED_SIZE * sizeof(u64)));
}

//! VByteStreamReader::next is a template, so we need to specialize this function.
template<>
void test_stream_read(VByteStreamReader& reader) {
  // Read it back
  u64 actual[EXPECTED_SIZE];
  for (auto i = 0u; i < EXPECTED_SIZE; i++) {
    actual[i] = reader.template next<u64>();
  }
  EXPECT_TRUE(0 == std::memcmp(EXPECTED, actual, EXPECTED_SIZE * sizeof(u64)));
}


TEST(Compression, Test_base128) {
  std::vector<unsigned char> data;
  data.resize(1000);

  Base128StreamWriter writer(data.data(), data.data() + data.size());
  test_stream_write(writer);

  Base128StreamReader reader(data.data(), data.data() + data.size());
  test_stream_read(reader);
}

TEST(Compression, Test_vbyte) {
  std::vector<unsigned char> data;
  data.resize(1000);

  VByteStreamWriter writer(data.data(), data.data() + data.size());
  test_stream_write(writer);

  VByteStreamReader reader(data.data(), data.data() + data.size());
  test_stream_read(reader);
}

TEST(Compression, Test_chunked_delta_delta_vbyte_0) {
  std::vector<unsigned char> data;
  data.resize(4 * 1024);  // 4KB of storage

  {
    // variable step
    VByteStreamWriter wstream(data.data(), data.data() + data.size());
    DeltaDeltaStreamWriter<16, u64> delta_writer(wstream);
    VByteStreamReader rstream(data.data(), data.data() + data.size());
    DeltaDeltaStreamReader<16, u64> delta_reader(rstream);

    test_stream_chunked_op<u64>(delta_writer, delta_reader, 100, true, false);
  }
  {
    // fixed step
    VByteStreamWriter wstream(data.data(), data.data() + data.size());
    DeltaDeltaStreamWriter<16, u64> delta_writer(wstream);
    VByteStreamReader rstream(data.data(), data.data() + data.size());
    DeltaDeltaStreamReader<16, u64> delta_reader(rstream);

    test_stream_chunked_op<u64>(delta_writer, delta_reader, 100, true, true);
  }
}

//! Generate time-series from random walk
struct RandomWalk {
  std::random_device                  randdev;
  std::mt19937                        generator;
  std::normal_distribution<double>    distribution;
  double                              value;

  RandomWalk(double start, double mean, double stddev)
      : generator(randdev())
        , distribution(mean, stddev)
        , value(start)
  {
  }

  double generate() {
    value += distribution(generator);
    return value;
  }
};


void test_float_compression(double start, std::vector<double>* psrc = nullptr) {
  RandomWalk rwalk(start, 1., .11);
  int N = 10000;
  std::vector<double> samples;
  std::vector<u8> block;
  block.resize(N * 9, 0);

  // Compress
  VByteStreamWriter wstream(block.data(), block.data() + block.size());
  FcmStreamWriter<> writer(wstream);
  if (psrc == nullptr) {
    double val = rwalk.generate();
    samples.push_back(val);
  } else {
    samples = *psrc;
  }

  for (size_t ix = 0; ix < samples.size(); ix++) {
    auto val = samples.at(ix);
    writer.put(val);
  }
  writer.commit();

  // Decompress
  VByteStreamReader rstream(block.data(), block.data() + block.size());
  FcmStreamReader<> reader(rstream);
  for (size_t ix = 0; ix < samples.size(); ix++) {
    double val = reader.next();
    ASSERT_DOUBLE_EQ(samples.at(ix), val);
  }
}

TEST(Compression, Test_float_compression_0) {
  test_float_compression(0);
}

TEST(Compression, Test_float_compression_1) {
  test_float_compression(1E-100);
}

TEST(Compression, Test_float_compression_2) {
  test_float_compression(1E100);
}

TEST(Compression,Test_float_compression_3) {
    test_float_compression(-1E-100);
}

TEST(Compression, Test_float_compression_4) {
  test_float_compression(-1E100);
}

TEST(Compression, Test_float_compression_5) {
  std::vector<double> samples(998, 3.14159);
  samples.push_back(111.222);
  samples.push_back(222.333);
  test_float_compression(0, &samples);
}

void test_block_compression(double start, unsigned N = 10000, bool regullar = false) {
  RandomWalk rwalk(start, 1., .11);
  std::vector<Timestamp> timestamps;
  std::vector<double> values;
  std::vector<u8> block;
  block.resize(4096);

  if (regullar) {
    Timestamp its = static_cast<Timestamp>(rand());
    Timestamp stp = static_cast<Timestamp>(rand() % 1000);
    for (unsigned i = 0; i < N; i++) {
      values.push_back(rwalk.generate());
      its += stp;
      timestamps.push_back(its);
    }
  } else {
    Timestamp its = static_cast<Timestamp>(rand());
    for (unsigned i = 0; i < N; i++) {
      values.push_back(rwalk.generate());
      u32 skew = rand() % 100;
      its += skew;
      timestamps.push_back(its);
    }
  }

  // compress

  DataBlockWriter writer(42, block.data(), block.size());

  size_t actual_nelements = 0ull;
  bool writer_overflow = false;
  for (size_t ix = 0; ix < N; ix++) {
    common::Status status = writer.put(timestamps.at(ix), values.at(ix));
    if (status.Code() == common::Status::kOverflow) {
      // Block is full
      actual_nelements = ix;
      writer_overflow = true;
      break;
    }
    EXPECT_TRUE(status.IsOk());
  }
  if (!writer_overflow) {
    actual_nelements = N;
  }
  size_t size_used = writer.commit();

  // decompress
  DataBlockReader reader(block.data(), size_used);

  std::vector<Timestamp> out_timestamps;
  std::vector<double> out_values;

  // gen number of elements stored in block
  u32 nelem = reader.nelements();
  EXPECT_EQ(nelem, actual_nelements);
  EXPECT_NE(nelem, 0);

  EXPECT_EQ(reader.get_id(), 42);
  for (size_t ix = 0ull; ix < reader.nelements(); ix++) {
    common::Status status;
    Timestamp  ts;
    double      value;
    std::tie(status, ts, value) = reader.next();
    EXPECT_TRUE(status.IsOk());
    out_timestamps.push_back(ts);
    out_values.push_back(value);
  }

  // nelements() + 1 call should result in error
  common::Status status;
  Timestamp  ts;
  double      value;
  std::tie(status, ts, value) = reader.next();
  EXPECT_EQ(status.Code(), common::Status::kNoData);

  for (size_t i = 0; i < nelem; i++) {
    EXPECT_EQ(timestamps.at(i), out_timestamps.at(i));
    EXPECT_DOUBLE_EQ(values.at(i), out_values.at(i));
  }
}

TEST(Compression, Test_block_compression_00) {
  test_block_compression(0);
}

TEST(Compression, Test_block_compression_01) {
  test_block_compression(1E-100);
}

TEST(Compression, Test_block_compression_02) {
  test_block_compression(1E100);
}

TEST(Compression, Test_block_compression_03) {
  test_block_compression(-1E-100);
}

TEST(Compression, Test_block_compression_04) {
  test_block_compression(-1E100);
}

TEST(Compression, Test_block_compression_05) {
  test_block_compression(0, 1);
}

TEST(Compression, Test_block_compression_06) {
  test_block_compression(0, 16);
}

TEST(Compression, Test_block_compression_07) {
  test_block_compression(0, 100);
}

TEST(Compression, Test_block_compression_08) {
  test_block_compression(0, 0x100);
}

TEST(Compression, Test_block_compression_09) {
  test_block_compression(0, 0x111);
}

TEST(Compression, Test_block_compression_10) {
  test_block_compression(0, 10000, true);
}

TEST(Compression, Test_block_compression_11) {
  test_block_compression(1E-100, 10000, true);
}

TEST(Compression, Test_block_compression_12) {
  test_block_compression(1E100, 10000, true);
}

TEST(Compression, Test_block_compression_13) {
  test_block_compression(-1E-100, 10000, true);
}

TEST(Compression, Test_block_compression_14) {
  test_block_compression(-1E100, 10000, true);
}

TEST(Compression, Test_block_compression_15) {
  test_block_compression(0, 1, true);
}

TEST(Compression, Test_block_compression_16) {
  test_block_compression(0, 16, true);
}

TEST(Compression, Test_block_compression_17) {
  test_block_compression(0, 100, true);
}

TEST(Compression, Test_block_compression_18) {
  test_block_compression(0, 0x100, true);
}

TEST(Compression, Test_block_compression_19) {
  test_block_compression(0, 0x111, true);
}

struct CheckedBlock {
  enum {
    NCOMPONENTS = 4,
    COMPONENT_SIZE = FASTSTDB_BLOCK_SIZE / NCOMPONENTS,
  };

  std::vector<u8> provided_;
  u32 pos_;
  u8 pod_[1024]; //! all space is allocated here (and not checked)

  CheckedBlock(const std::vector<u8>& p)
      : provided_(p)
        , pos_(0)
  {
  }

  u8* allocate(u32 size) {
    u8* result = provided_.data() + pos_;
    pos_ += size;
    return result;
  }

  /** Add component if block is less than NCOMPONENTS in size.
   *  Return index of the component or -1 if block is full.
   */
  int add() { return 0; }

  void set_addr(u64) {}

  u64 get_addr() const { return 0; }

  int space_left() const { return static_cast<int>(FASTSTDB_BLOCK_SIZE - pos_); }

  int size() const { return FASTSTDB_BLOCK_SIZE; }

  void put(u8 val) {
    EXPECT_EQ(val, provided_.at(pos_));
    pos_++;
  }

  bool safe_put(u8 val) {
    EXPECT_EQ(val, provided_.at(pos_));
    pos_++;
    return true;
  }

  int get_write_pos() const {
    return static_cast<int>(pos_);
  }

  void set_write_pos(int pos) {
    pos_ = static_cast<u32>(pos);
  }

  template<class POD>
  void put(const POD& data) {
    const u8* it = reinterpret_cast<const u8*>(&data);
    for (u32 i = 0; i < sizeof(POD); i++) {
      put(it[i]);
    }
  }

  template<class POD>
  POD* allocate() {
    POD* result = reinterpret_cast<POD*>(pod_ + pos_);
    pos_ += sizeof(POD);
    return result;
  }

  bool is_readonly() const {
    return false;
  }

  const u8* get_data(int) const {
    return nullptr;
  }

  const u8* get_cdata(int) const {
    return nullptr;
  }

  u8* get_data(int) {
    return nullptr;
  }

  size_t get_size(int) const {
    return 1024;
  }
};

void test_block_iovec_compression(double start, unsigned N = 10000, bool regullar = false) {
  RandomWalk rwalk(start, 1., .11);
  std::vector<Timestamp> timestamps;
  std::vector<double> values;

  if (regullar) {
    Timestamp its = static_cast<Timestamp>(rand());
    Timestamp stp = static_cast<Timestamp>(rand() % 1000);
    for (unsigned i = 0; i < N; i++) {
      values.push_back(rwalk.generate());
      its += stp;
      timestamps.push_back(its);
    }
  } else {
    Timestamp its = static_cast<Timestamp>(rand());
    for (unsigned i = 0; i < N; i++) {
      values.push_back(rwalk.generate());
      u32 skew = rand() % 100;
      its += skew;
      timestamps.push_back(its);
    }
  }

  // compress using normal block
  std::vector<u8> dbdata;
  dbdata.resize(4096);
  DataBlockWriter dbwriter(42, dbdata.data(), dbdata.size());
  for (size_t ix = 0; ix < N; ix++) {
    common::Status status = dbwriter.put(timestamps.at(ix), values.at(ix));
    if (status.Code() == common::Status::kOverflow) {
      // Block is full
      break;
    }
    EXPECT_TRUE(status.IsOk());
  }

  // compress
  CheckedBlock chkblock(dbdata);
  IOVecBlockWriter<CheckedBlock> chkwriter(&chkblock);
  chkwriter.init(42);

  IOVecBlock block;
  IOVecBlockWriter<IOVecBlock> writer(&block);
  writer.init(42);

  size_t actual_nelements = 0ull;
  bool writer_overflow = false;
  for (size_t ix = 0; ix < N; ix++) {
    common::Status chkstatus = chkwriter.put(timestamps.at(ix), values.at(ix));
    common::Status status = writer.put(timestamps.at(ix), values.at(ix));
    EXPECT_EQ(chkstatus.Code(), status.Code());
    if (status.Code() == common::Status::kOverflow) {
      // Block is full
      actual_nelements = ix;
      writer_overflow = true;
      break;
    }
    EXPECT_TRUE(status.IsOk());
  }
  if (!writer_overflow) {
    actual_nelements = N;
  }
  size_t size_used = writer.commit();
  UNUSED(size_used);

  // decompress using normal procedure
  std::vector<u8> cblock;
  cblock.reserve(4096);
  for (int i = 0; i < IOVecBlock::NCOMPONENTS; i++) {
    const int sz = IOVecBlock::COMPONENT_SIZE;
    if(block.get_size(i) == 0) {
      break;
    }
    std::copy(block.get_cdata(i), block.get_cdata(i) + sz, std::back_inserter(cblock));
  }

  // This iovec block contains data in once continous region
  IOVecBlock cont_block(true);
  std::copy(cblock.begin(), cblock.end(), cont_block.get_data(0));

  DataBlockReader reader(cblock.data(), cblock.size());
  IOVecBlockReader<IOVecBlock> iovecreader(&block);
  IOVecBlockReader<IOVecBlock> iovecreader_cont(&cont_block);

  std::vector<Timestamp> out_timestamps;
  std::vector<double> out_values;

  // gen number of elements stored in block
  auto nelem = reader.nelements();
  EXPECT_EQ(nelem, actual_nelements);
  EXPECT_EQ(nelem, iovecreader.nelements());
  EXPECT_EQ(nelem, iovecreader_cont.nelements());
  EXPECT_NE(nelem, 0);

  EXPECT_EQ(reader.get_id(), 42);
  EXPECT_EQ(iovecreader.get_id(), 42);
  EXPECT_EQ(iovecreader_cont.get_id(), 42);

  for (size_t ix = 0ull; ix < reader.nelements(); ix++) {
    common::Status status;
    Timestamp  ts;
    double      value;
    std::tie(status, ts, value) = reader.next();
    EXPECT_TRUE(status.IsOk());
    out_timestamps.push_back(ts);
    out_values.push_back(value);
    Timestamp  iovects;
    double      iovecvalue;
    // Check io-vec block with four components
    std::tie(status, iovects, iovecvalue) = iovecreader.next();
    EXPECT_DOUBLE_EQ(value, iovecvalue);
    EXPECT_EQ(ts, iovects);
    // Check continous io-vec block
    std::tie(status, iovects, iovecvalue) = iovecreader_cont.next();
    EXPECT_DOUBLE_EQ(value, iovecvalue);
    EXPECT_EQ(ts, iovects);
  }
  // nelements() + 1 call should result in error
  common::Status status;
  Timestamp  ts;
  double      value;
  std::tie(status, ts, value) = reader.next();
  EXPECT_EQ(status.Code(), common::Status::kNoData);

  for (size_t i = 0; i < nelem; i++) {
    EXPECT_EQ(timestamps.at(i), out_timestamps.at(i));
    EXPECT_DOUBLE_EQ(values.at(i), out_values.at(i));
  }
}

TEST(Compression,Test_iovec_compression_00) {
  test_block_iovec_compression(0);
}

TEST(Compression,Test_iovec_compression_01) {
  test_block_iovec_compression(1E-100);
}

TEST(Compression,Test_iovec_compression_02) {
  test_block_iovec_compression(1E100);
}

TEST(Compression,Test_iovec_compression_03) {
  test_block_iovec_compression(-1E-100);
}

TEST(Compression,Test_iovec_compression_04) {
  test_block_iovec_compression(-1E100);
}

TEST(Compression,Test_iovec_compression_05) {
  test_block_iovec_compression(0, 1);
}

TEST(Compression,Test_iovec_compression_06) {
  test_block_iovec_compression(0, 16);
}

TEST(Compression,Test_iovec_compression_07) {
  test_block_iovec_compression(0, 100);
}

TEST(Compression,Test_iovec_compression_08) {
  test_block_iovec_compression(0, 0x100);
}

TEST(Compression,Test_iovec_compression_09) {
  test_block_iovec_compression(0, 0x111);
}

TEST(Compression,Test_iovec_compression_10) {
  test_block_iovec_compression(0, 10000, true);
}

TEST(Compression,Test_iovec_compression_11) {
  test_block_iovec_compression(1E-100, 10000, true);
}

TEST(Compression,Test_iovec_compression_12) {
  test_block_iovec_compression(1E100, 10000, true);
}

TEST(Compression,Test_iovec_compression_13) {
  test_block_iovec_compression(-1E-100, 10000, true);
}

TEST(Compression,Test_iovec_compression_14) {
  test_block_iovec_compression(-1E100, 10000, true);
}

TEST(Compression,Test_iovec_compression_15) {
  test_block_iovec_compression(0, 1, true);
}

TEST(Compression,Test_iovec_compression_16) {
  test_block_iovec_compression(0, 16, true);
}

TEST(Compression,Test_iovec_compression_17) {
  test_block_iovec_compression(0, 100, true);
}

TEST(Compression,Test_iovec_compression_18) {
  test_block_iovec_compression(0, 0x100, true);
}

TEST(Compression,Test_iovec_compression_19) {
  test_block_iovec_compression(0, 0x111, true);
}

}  // namespace storage
}  // namespace faststdb
