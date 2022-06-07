
[![Ubuntu testing](https://github.com/faststdb/FastSTDB/actions/workflows/linux-test.yml/badge.svg)](https://github.com/faststdb/FastSTDB/actions/workflows/linux-test.yml)

======

**FastSTDB** is a time-series and spatial-temporal database for modern hardware. 
It can be used to capture, store and process time-series or spatial-temporal data in real-time. 

Features
-------

* Column-oriented storage.
* Based on novel [LSM and B+tree hybrid datastructure](http://akumuli.org/akumuli/2017/04/29/nbplustree/) with multiversion concurrency control (no concurrency bugs, parallel writes, optimized for SSD and NVMe).
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
  * 5.4M writes/sec on DigitalOcean droplet with 8-cores 32GB of RAM (using only 6 cores)
  * 4.6M writes/sec on DigitalOcean droplet with 8-cores 32GB of RAM (6 cores with enabled WAL)
  * 16.1M writes/sec on 32-core Intel Xeon E5-2680 v2 (c3.8xlarge EC2 instance).
* Queries are executed lazily. Query results are produced as long as client reads them.
* Compression algorithm and input parsers are fuzz-tested on every code change.
* Grafana [datasource](https://github.com/akumuli/akumuli-datasource) plugin.
* Fast and compact inverted index for time-series lookup.



