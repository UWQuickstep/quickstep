CREATE TABLE region (
  r_regionkey INT NOT NULL,
  r_name CHAR(25) NOT NULL,
  r_comment VARCHAR(152) NOT NULL
) WITH BLOCKPROPERTIES (
  TYPE split_rowstore,
  BLOCKSIZEMB 4);

CREATE TABLE nation (
  n_nationkey INT NOT NULL,
  n_name CHAR(25) NOT NULL,
  n_regionkey INT NOT NULL,
  n_comment VARCHAR(152) NOT NULL
) WITH BLOCKPROPERTIES (
  TYPE split_rowstore,
  BLOCKSIZEMB 4);

CREATE TABLE supplier (
  s_suppkey INT NOT NULL,
  s_name CHAR(25) NOT NULL,
  s_address VARCHAR(40) NOT NULL,
  s_nationkey INT NOT NULL,
  s_phone CHAR(15) NOT NULL,
  s_acctbal DECIMAL NOT NULL,
  s_comment VARCHAR(101) NOT NULL
) WITH BLOCKPROPERTIES (
  TYPE split_rowstore,
  BLOCKSIZEMB 4);

CREATE TABLE customer (
  c_custkey INT NOT NULL,
  c_name VARCHAR(25) NOT NULL,
  c_address VARCHAR(40) NOT NULL,
  c_nationkey INT NOT NULL,
  c_phone CHAR(15) NOT NULL,
  c_acctbal DECIMAL NOT NULL,
  c_mktsegment CHAR(10) NOT NULL,
  c_comment VARCHAR(117) NOT NULL
) WITH BLOCKPROPERTIES (
  TYPE split_rowstore,
  BLOCKSIZEMB 4);

CREATE TABLE part (
  p_partkey INT NOT NULL,
  p_name VARCHAR(55) NOT NULL,
  p_mfgr CHAR(25) NOT NULL,
  p_brand CHAR(10) NOT NULL,
  p_type VARCHAR(25) NOT NULL,
  p_size INT NOT NULL,
  p_container CHAR(10) NOT NULL,
  p_retailprice DECIMAL NOT NULL,
  p_comment VARCHAR(23) NOT NULL
) WITH BLOCKPROPERTIES (
  TYPE split_rowstore,
  BLOCKSIZEMB 4);

CREATE TABLE partsupp (
  ps_partkey INT NOT NULL,
  ps_suppkey INT NOT NULL,
  ps_availqty INT NOT NULL,
  ps_supplycost DECIMAL NOT NULL,
  ps_comment VARCHAR(199) NOT NULL
) WITH BLOCKPROPERTIES (
  TYPE compressed_columnstore,
  SORT ps_partkey,
  COMPRESS ALL,
  BLOCKSIZEMB 4);
--) WITH BLOCKPROPERTIES (
--  TYPE split_rowstore,
--  BLOCKSIZEMB 4);
-- CREATE INDEX partsuppSMA ON partsupp USING SMA;

CREATE TABLE orders (
  o_orderkey INT NOT NULL,
  o_custkey INT NOT NULL,
  o_orderstatus CHAR(1) NOT NULL,
  o_totalprice DECIMAL NOT NULL,
  o_orderdate DATE NOT NULL,
  o_orderpriority CHAR(15) NOT NULL,
  o_clerk CHAR(15) NOT NULL,
  o_shippriority INT NOT NULL,
  o_comment VARCHAR(79) NOT NULL
) WITH BLOCKPROPERTIES (
  TYPE compressed_columnstore,
  SORT o_orderkey,
  COMPRESS ALL,
  BLOCKSIZEMB 4);
--) WITH BLOCKPROPERTIES (
--  TYPE split_rowstore,
--  BLOCKSIZEMB 4);
-- CREATE INDEX ordersSMA ON orders USING SMA;

CREATE TABLE lineitem (
  l_orderkey INT NOT NULL,
  l_partkey INT NOT NULL,
  l_suppkey INT NOT NULL,
  l_linenumber INT NOT NULL,
  l_quantity DECIMAL NOT NULL,
  l_extendedprice DECIMAL NOT NULL,
  l_discount DECIMAL NOT NULL,
  l_tax DECIMAL NOT NULL,
  l_returnflag CHAR(1) NOT NULL,
  l_linestatus CHAR(1) NOT NULL,
  l_shipdate DATE NOT NULL,
  l_commitdate DATE NOT NULL,
  l_receiptdate DATE NOT NULL,
  l_shipinstruct CHAR(25) NOT NULL,
  l_shipmode CHAR(10) NOT NULL,
  l_comment VARCHAR(44) NOT NULL
) WITH BLOCKPROPERTIES (
  TYPE compressed_columnstore,
  SORT l_orderkey,
  COMPRESS ALL,
  BLOCKSIZEMB 4);
-- CREATE INDEX sma_index ON lineitem(l_shipdate,l_receiptdate,l_quantity) USING SMA;
