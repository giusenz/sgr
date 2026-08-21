CREATE TABLE IF NOT EXISTS chiba.metrics 
(
    BUCKET_TIME DateTime,
    SRCADDR UInt32,
    DSTADDR UInt32,
    TOTAL_DPKTS AggregateFunction(sum, UInt64),
    TOTAL_DOCTETS AggregateFunction(sum, UInt64),
    DSTPORT UInt16
) ENGINE = AggregatingMergeTree() ORDER BY (BUCKET_TIME, SRCADDR, DSTADDR, DSTPORT);