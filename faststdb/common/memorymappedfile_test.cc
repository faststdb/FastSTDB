/*!
 * \file memorymappedfile_test.cc
 */
#include "faststdb/common/memorymappedfile.h"

#include "gtest/gtest.h"

#include "faststdb/common/logging.h"

namespace faststdb {

struct Initializer {
  Initializer() {
    apr_initialize();
  }
};

Initializer initializer;

void create_tmp_file(const char* file_path, int len) {
  apr_pool_t* pool = NULL;
  apr_file_t* file = NULL;
  apr_status_t status = apr_pool_create(&pool, NULL);
  if (status == APR_SUCCESS) {
    status = apr_file_open(&file, file_path, APR_WRITE | APR_CREATE, APR_OS_DEFAULT, pool);
    if (status == APR_SUCCESS) {
      status = apr_file_trunc(file, len);
      if (status == APR_SUCCESS) {
        status = apr_file_close(file);
      }
    }
  }
  if (pool)
    apr_pool_destroy(pool);
  if (status != APR_SUCCESS) {
    LOG(FATAL) << apr_error_message(status);
  }
}

void delete_tmp_file(const char* file_path) {
  apr_pool_t* pool = NULL;
  apr_pool_create(&pool, NULL);
  apr_file_remove(file_path, pool);
  apr_pool_destroy(pool);
}

TEST(TestMemoryMmapedFile, TestMmap1) {
  const char* tmp_file = "testfile";
  delete_tmp_file(tmp_file);
  create_tmp_file(tmp_file, 100);
  MemoryMappedFile mmap(tmp_file, false);
  EXPECT_TRUE(mmap.is_bad() == false);
  EXPECT_TRUE(mmap.get_size() == 100);
  delete_tmp_file(tmp_file);
}

TEST(TestMemoryMmapedFile, TestMmap3) {
  const char* tmp_file = "testfile";
  delete_tmp_file(tmp_file);
  create_tmp_file(tmp_file, 100);
  {
    MemoryMappedFile mmap(tmp_file, false);
    EXPECT_TRUE(mmap.is_bad() == false);
    EXPECT_TRUE(mmap.get_size() == 100);
    char* begin = (char*)mmap.get_pointer();
    char* end = begin + 99;
    *begin = 42;
    *end = 24;
  }

  {
    MemoryMappedFile mmap(tmp_file, false);
    EXPECT_TRUE(mmap.is_bad() == false);
    EXPECT_TRUE(mmap.get_size() == 100);
    char* begin = (char*)mmap.get_pointer();
    char* end = begin + 99;
    EXPECT_TRUE(*begin == 42);
    EXPECT_TRUE(*end == 24);
  }

  delete_tmp_file(tmp_file);
}

TEST(TestMemoryMmapedFile, TestMmap4) {
  const char* tmp_file = "testfile";
  delete_tmp_file(tmp_file);
  create_tmp_file(tmp_file, 100);
  {
    MemoryMappedFile mmap(tmp_file, false);
    EXPECT_TRUE(mmap.is_bad() == false);
    EXPECT_TRUE(mmap.get_size() == 100);
    char* begin = (char*)mmap.get_pointer();
    char* end = begin + 99;
    *begin = 42;
    *end = 24;
  }

  {
    MemoryMappedFile mmap(tmp_file, false);
    EXPECT_TRUE(mmap.is_bad() == false);
    EXPECT_TRUE(mmap.get_size() == 100);
    mmap.remap_file_destructive();
    char* begin = (char*)mmap.get_pointer();
    char* end = begin + 99;
    EXPECT_TRUE(*begin != 42);
    EXPECT_TRUE(*end != 24);
  }

  delete_tmp_file(tmp_file);
}

TEST(TestMemoryMmapedFile, get_page_size) {
  auto page_size = get_page_size();
  EXPECT_EQ(4096, page_size);
  LOG(INFO) << "page_size=" << page_size;
}

}  // namespace faststdb
