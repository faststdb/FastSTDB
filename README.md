
[![Ubuntu testing](https://github.com/faststdb/FastSTDB/actions/workflows/linux-test.yml/badge.svg)](https://github.com/faststdb/FastSTDB/actions/workflows/linux-test.yml)

======

**FastSTDB** is a time-series and spatial-temporal database for modern hardware. 
It can be used to capture, store and process time-series or spatial-temporal data in real-time. 

Features
-------

* Column-oriented storage.
* Based on B+tree with multiversion concurrency control (no concurrency bugs, parallel writes, optimized for SSD and NVMe).
* Supports both metrics and events.
* Fast and effecient compression algorithm that outperforms 'Gorilla' time-series compression.
* Crash safety and recovery.
* Fast aggregation without pre-configured rollups or materialized views.
* Many queries can be executed without decompressing the data.
* Compressed in-memory storage for recent data.
* Can be used as a server application or embedded library.
* Simple API based on JSON and HTTP.
* Fast range scans and joins, read speed doesn't depend on database cardinality.
* Fast data ingestion:
* Queries are executed lazily. Query results are produced as long as client reads them.
* Compression algorithm and input parsers are fuzz-tested on every code change.
* Fast and compact inverted index for time-series lookup.



