/*!
 * \file config.h
 */
#ifndef FASTSTDB_COMMON_CONFIG_H_
#define FASTSTDB_COMMON_CONFIG_H_

#include "faststdb/common/types.h"

/**
 * configuration.
 */
typedef struct {
  //! Max size of the input-log volume
  u64 input_log_volume_size;

  //! Number of volumes to keep
  u64 input_log_volume_numb;

  //! Input log max concurrency
  u32 input_log_concurrency;

  //! Path to input log root directory
  const char* input_log_path;

} FineTuneParams;

#endif  // FASTSTDB_COMMON_CONFIG_H_
