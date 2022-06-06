/**
 * Copyright (c) 2016 Eugene Lazin <4lazin@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#pragma once

/* In general this is a tree-roots collection combined with various algorithms (
 * (aggregate, join, group-aggregate). One TreeRegistery should be created
 * per database. This registery can be used to create sessions. Session instances
 * should be created per-connection for each connection to operate locally (without
 * synchronization). This code assumes that each connection works with its own
 * set of time-series. If this isn't the case - performance penalty will be introduced.
 */

// Stdlib
#include <unordered_map>
#include <mutex>
#include <tuple>

// Boost libraries
#include <boost/property_tree/ptree_fwd.hpp>


// Project
#include "akumuli_def.h"
#include "external_cursor.h"
#include "metadatastorage.h"
#include "index/seriesparser.h"
#include "storage_engine/nbtree.h"
#include "queryprocessor_framework.h"
#include "log_iface.h"

namespace Akumuli {
namespace StorageEngine {


/** Columns store.
  * Serve as a central data repository for series metadata and all individual columns.
  * Each column is addressed by the series name. Data can be written in through WriteSession
  * and read back via IStreamProcessor interface. ColumnStore can reshape data (group, merge or join
  * different columns together).
  * Columns are built from NB+tree instances.
  * Instances of this class is thread-safe.
  */
class ColumnStore : public std::enable_shared_from_this<ColumnStore> {
    std::shared_ptr<StorageEngine::BlockStore> blockstore_;
    std::unordered_map<aku_ParamId, std::shared_ptr<NBTreeExtentsList>> columns_;
    PlainSeriesMatcher global_matcher_;
    //! List of metadata to update
    std::unordered_map<aku_ParamId, std::vector<StorageEngine::LogicAddr>> rescue_points_;
    //! Mutex for table_ hashmap (shrink and resize)
    mutable std::mutex table_lock_;
    //! Syncronization for watcher thread
    std::condition_variable cvar_;

public:
    ColumnStore(std::shared_ptr<StorageEngine::BlockStore> bstore);

    // No value semantics allowed.
    ColumnStore(ColumnStore const&) = delete;
    ColumnStore(ColumnStore &&) = delete;
    ColumnStore& operator = (ColumnStore const&) = delete;

    //! Open storage or restore if needed
    std::tuple<aku_Status, std::vector<aku_ParamId>> open_or_restore(
            const std::unordered_map<aku_ParamId, std::vector<LogicAddr>> &mapping,
            bool force_init=false);

    std::unordered_map<aku_ParamId, std::vector<LogicAddr> > close();

    /**
     * @brief close specific columns
     * @param ids is a list of column ids
     * @return list of rescue points for every id
     */
    std::unordered_map<aku_ParamId, std::vector<LogicAddr> > close(const std::vector<aku_ParamId>& ids);

    /** Create new column.
      * @return completion status
      */
    aku_Status create_new_column(aku_ParamId id);

    /** Write sample to data-store.
      * @param sample to write
      * @param cache_or_null is a pointer to external cache, tree ref will be added there on success
      */
    NBTreeAppendResult write(aku_Sample const& sample, std::vector<LogicAddr> *rescue_points,
                     std::unordered_map<aku_ParamId, std::shared_ptr<NBTreeExtentsList> > *cache_or_null=nullptr);

    /**
     * @brief Write sample to data-store during crash recovery
     * @param sample to write
     * @return write status
     */
    NBTreeAppendResult recovery_write(aku_Sample const& sample, bool allow_duplicates);

    size_t _get_uncommitted_memory() const;

    //! For debug reports
    std::unordered_map<aku_ParamId, std::shared_ptr<NBTreeExtentsList>> _get_columns() {
        return columns_;
    }

    // -------------
    // New-style API
    // -------------

    template<class IterType, class Fn>
    aku_Status iterate(const std::vector<aku_ParamId>& ids,
                      std::vector<std::unique_ptr<IterType>>* dest,
                      const Fn& fn) const
    {
        for (auto id: ids) {
            std::lock_guard<std::mutex> lg(table_lock_); AKU_UNUSED(lg);
            auto it = columns_.find(id);
            if (it != columns_.end()) {
                if (!it->second->is_initialized()) {
                    it->second->force_init();
                }
                aku_Status s;
                std::unique_ptr<IterType> iter;
                std::tie(s, iter) = std::move(fn(*it->second));
                if (s != AKU_SUCCESS) {
                    return s;
                }
                dest->push_back(std::move(iter));
            } else {
                return AKU_ENOT_FOUND;
            }
        }
        return AKU_SUCCESS;
    }

    aku_Status scan(std::vector<aku_ParamId> const& ids,
                    aku_Timestamp begin,
                    aku_Timestamp end,
                    std::vector<std::unique_ptr<RealValuedOperator>>* dest) const
    {
        return iterate(ids, dest, [begin, end](const NBTreeExtentsList& elist) {
            return std::make_tuple(AKU_SUCCESS, elist.search(begin, end));
        });
    }

    aku_Status scan_events(std::vector<aku_ParamId> const& ids,
                    aku_Timestamp begin,
                    aku_Timestamp end,
                    std::vector<std::unique_ptr<BinaryDataOperator>>* dest) const
    {
        return iterate(ids, dest, [begin, end](const NBTreeExtentsList& elist) {
            return std::make_tuple(AKU_SUCCESS, elist.search_binary(begin, end));
        });
    }

    aku_Status filter_events(std::vector<aku_ParamId> const& ids,
                    aku_Timestamp begin,
                    aku_Timestamp end,
                    const std::string& expr,
                    std::vector<std::unique_ptr<BinaryDataOperator>>* dest) const
    {
        return iterate(ids, dest, [begin, end, expr](const NBTreeExtentsList& elist) {
            return std::make_tuple(AKU_SUCCESS, elist.filter_binary(begin, end, expr));
        });
    }

    aku_Status filter(std::vector<aku_ParamId> const& ids,
                      aku_Timestamp begin,
                      aku_Timestamp end,
                      const std::map<aku_ParamId, ValueFilter>& filters,
                      std::vector<std::unique_ptr<RealValuedOperator>>* dest) const
    {
        return iterate(ids, dest, [begin, end, filters, ids](const NBTreeExtentsList& elist) {
            auto flt = filters.find(elist.get_id());
            if (flt != filters.end()) {
                if (flt->second.mask != 0) {
                    return std::make_tuple(AKU_SUCCESS, elist.filter(begin, end, flt->second));
                } else {
                    return std::make_tuple(AKU_SUCCESS, elist.search(begin, end));
                }
            }
            Logger::msg(AKU_LOG_ERROR, std::string("Can't find filter for id ") + std::to_string(elist.get_id()));
            return std::make_tuple(AKU_EBAD_ARG, std::unique_ptr<RealValuedOperator>());
        });
    }

    aku_Status aggregate(std::vector<aku_ParamId> const& ids,
                         aku_Timestamp begin,
                         aku_Timestamp end,
                         std::vector<std::unique_ptr<AggregateOperator>>* dest) const
    {
        return iterate(ids, dest, [begin, end](const NBTreeExtentsList& elist) {
            return std::make_tuple(AKU_SUCCESS, elist.aggregate(begin, end));
        });
    }

    aku_Status group_aggregate(std::vector<aku_ParamId> const& ids,
                               aku_Timestamp begin,
                               aku_Timestamp end,
                               aku_Timestamp step,
                               std::vector<std::unique_ptr<AggregateOperator>>* dest) const
    {
        return iterate(ids, dest, [begin, end, step](const NBTreeExtentsList& elist) {
            return std::make_tuple(AKU_SUCCESS, elist.group_aggregate(begin, end, step));
        });
    }

    aku_Status group_aggfilter(std::vector<aku_ParamId> const& ids,
                               aku_Timestamp begin,
                               aku_Timestamp end,
                               aku_Timestamp step,
                               const std::map<aku_ParamId, AggregateFilter>& filters,
                               std::vector<std::unique_ptr<AggregateOperator>>* dest) const
    {
        return iterate(ids, dest, [begin, end, step, filters, ids](const NBTreeExtentsList& elist) {
            auto flt = filters.find(elist.get_id());
            if (flt != filters.end()) {
                if (flt->second.bitmap != 0) {
                    return std::make_tuple(AKU_SUCCESS, elist.group_aggregate_filter(begin, end, step, flt->second));
                } else {
                    return std::make_tuple(AKU_SUCCESS, elist.group_aggregate(begin, end, step));
                }
            }
            Logger::msg(AKU_LOG_ERROR, std::string("Can't find filter for id ") + std::to_string(elist.get_id()));
            return std::make_tuple(AKU_EBAD_ARG, std::unique_ptr<AggregateOperator>());
        });
    }
};


/** Dispatches incoming messages to corresponding NBTreeExtentsList instances.
  * Should be created per writer thread. Stores series matcher cache and tree
  * cache. ColumnStore can work without WriteSession.
  */
class CStoreSession : public std::enable_shared_from_this<CStoreSession>
{
    //! Link to global column store.
    std::shared_ptr<ColumnStore> cstore_;
    //! Tree cache
    std::unordered_map<aku_ParamId, std::shared_ptr<NBTreeExtentsList>> cache_;
public:
    //! C-tor. Shouldn't be called directly.
    CStoreSession(std::shared_ptr<ColumnStore> registry);

    CStoreSession(CStoreSession const&) = delete;
    CStoreSession(CStoreSession &&) = delete;
    CStoreSession& operator = (CStoreSession const&) = delete;

    //! Write sample
    NBTreeAppendResult write(const aku_Sample &sample, std::vector<LogicAddr>* rescue_points);

    /**
     * Closes the session. This method should unload all cached trees
     */
    void close();
};

}}  // namespace
