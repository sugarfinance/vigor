#!/bin/bash
# Usage: ./test.sh
#=================================================================================#
# SETUP
# 
pkill nodeos
rm -rf ~/.local/share/eosio/nodeos/data
nodeos -e -p eosio --http-validate-host=false --delete-all-blocks --plugin eosio::chain_api_plugin --contracts-console --plugin eosio::http_plugin --plugin eosio::history_api_plugin --verbose-http-errors --max-transaction-time=10000
#nodeos -e -p eosio --plugin eosio::producer_plugin --plugin eosio::chain_api_plugin --plugin eosio::http_plugin --plugin eosio::state_history_plugin --access-control-allow-origin='*' --contracts-console --http-validate-host=false --trace-history --chain-state-history --verbose-http-errors --filter-on='*' --disable-replay-opts >> nodeos.log 2>&1 &



CYAN='\033[1;36m'
NC='\033[0m'

# CHANGE PATH
EOSIO_CONTRACTS_ROOT=/home/gg/contracts/eosio.contracts/contracts
CONTRACT_ROOT=/home/gg/contracts/vigor/contracts


OWNER_KEY="EOS6TnW2MQbZwXHWDHAYQazmdc3Sc1KGv4M9TSgsKZJSo43Uxs2Bx"
OWNER_ACCT="5J3TQGkkiRQBKcg8Gg2a7Kk5a2QAQXsyGrkCnnq4krSSJSUkW12"

#cleos wallet create --to-console
cleos wallet unlock -n default --password PW5HzuR3R2g77WZCsqaSMD91aC3WPFQzmeHkyzyEREbPTB3EmHeMC
#cleos wallet import -n default --private-key $OWNER_ACCT

#=================================================================================#
# BOOTSTRAP EOS CRAP

# EOSIO system-related keys
echo -e "${CYAN}-----------------------SYSTEM KEYS-----------------------${NC}"
cleos wallet import -n default --private-key 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
cleos wallet import -n default --private-key 5JgqWJYVBcRhviWZB3TU1tN9ui6bGpQgrXVtYZtTG2d3yXrDtYX
cleos wallet import -n default --private-key 5JjjgrrdwijEUU2iifKF94yKduoqfAij4SKk6X5Q3HfgHMS4Ur6
cleos wallet import -n default --private-key 5HxJN9otYmhgCKEbsii5NWhKzVj2fFXu3kzLhuS75upN5isPWNL
cleos wallet import -n default --private-key 5JNHjmgWoHiG9YuvX2qvdnmToD2UcuqavjRW5Q6uHTDtp3KG3DS
cleos wallet import -n default --private-key 5JZkaop6wjGe9YY8cbGwitSuZt8CjRmGUeNMPHuxEDpYoVAjCFZ
cleos wallet import -n default --private-key 5Hroi8WiRg3by7ap3cmnTpUoqbAbHgz3hGnGQNBYFChswPRUt26
cleos wallet import -n default --private-key 5JbMN6pH5LLRT16HBKDhtFeKZqe7BEtLBpbBk5D7xSZZqngrV8o
cleos wallet import -n default --private-key 5JUoVWoLLV3Sj7jUKmfE8Qdt7Eo7dUd4PGZ2snZ81xqgnZzGKdC
cleos wallet import -n default --private-key 5Ju1ree2memrtnq8bdbhNwuowehZwZvEujVUxDhBqmyTYRvctaF

# Create system accounts
echo -e "${CYAN}-----------------------SYSTEM ACCOUNTS-----------------------${NC}"
cleos create account eosio eosio.bpay EOS7gFoz5EB6tM2HxdV9oBjHowtFipigMVtrSZxrJV3X6Ph4jdPg3
cleos create account eosio eosio.msig EOS6QRncHGrDCPKRzPYSiWZaAw7QchdKCMLWgyjLd1s2v8tiYmb45
cleos create account eosio eosio.names EOS7ygRX6zD1sx8c55WxiQZLfoitYk2u8aHrzUxu6vfWn9a51iDJt
cleos create account eosio eosio.ram EOS5tY6zv1vXoqF36gUg5CG7GxWbajnwPtimTnq6h5iptPXwVhnLC
cleos create account eosio eosio.ramfee EOS6a7idZWj1h4PezYks61sf1RJjQJzrc8s4aUbe3YJ3xkdiXKBhF
cleos create account eosio eosio.saving EOS8ioLmKrCyy5VyZqMNdimSpPjVF2tKbT5WKhE67vbVPcsRXtj5z
cleos create account eosio eosio.stake EOS5an8bvYFHZBmiCAzAtVSiEiixbJhLY8Uy5Z7cpf3S9UoqA3bJb
#cleos create account eosio eosio.token EOS7JPVyejkbQHzE9Z4HwewNzGss11GB21NPkwTX2MQFmruYFqGXm
cleos create account eosio eosio.token EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio eosio.vpay EOS6szGbnziz224T1JGoUUFu2LynVG72f8D3UVAS25QgwawdH983U
cleos create account eosio eosio.rex EOS6szGbnziz224T1JGoUUFu2LynVG72f8D3UVAS25QgwawdH983U

# Bootstrap new system contracts
echo -e "${CYAN}-----------------------SYSTEM CONTRACTS-----------------------${NC}"
#eosio-cpp -contract=eosio.msig -I=$EOSIO_CONTRACTS_ROOT/eosio.msig/include -o=$EOSIO_CONTRACTS_ROOT/eosio.msig/eosio.msig.wasm -abigen $EOSIO_CONTRACTS_ROOT/eosio.msig/src/eosio.msig.cpp
#eosio-cpp -contract=eosio.system -I=$EOSIO_CONTRACTS_ROOT/eosio.system/include -I=$EOSIO_CONTRACTS_ROOT/eosio.token/include -o=$EOSIO_CONTRACTS_ROOT/eosio.system/eosio.system.wasm -abigen $EOSIO_CONTRACTS_ROOT/eosio.system/src/eosio.system.cpp
#eosio-cpp -contract=eosio.wrap -I=$EOSIO_CONTRACTS_ROOT/eosio.wrap/include -o=$EOSIO_CONTRACTS_ROOT/eosio.wrap/eosio.wrap.wasm -abigen $EOSIO_CONTRACTS_ROOT/eosio.wrap/src/eosio.wrap.cpp
#eosio-cpp -contract=eosio.token -I=$EOSIO_CONTRACTS_ROOT/eosio.token/include -o=$EOSIO_CONTRACTS_ROOT/eosio.token/eosio.token.wasm -abigen $EOSIO_CONTRACTS_ROOT/eosio.token/src/eosio.token.cpp
#eosio-cpp -contract=eosio.bios -I=$EOSIO_CONTRACTS_ROOT/eosio.bios/include -o=$EOSIO_CONTRACTS_ROOT/eosio.bios/eosio.bios.wasm -abigen $EOSIO_CONTRACTS_ROOT/eosio.bios/src/eosio.bios.cpp

cleos set contract eosio.token $EOSIO_CONTRACTS_ROOT/eosio.token/
cleos set contract eosio.msig $EOSIO_CONTRACTS_ROOT/eosio.msig
cleos push action eosio.token create '[ "eosio", "100000000000.0000 EOS" ]' -p eosio.token
cleos push action eosio.token create '[ "eosio", "100000000000.0000 SYS" ]' -p eosio.token
echo -e "      EOS TOKEN CREATED"
cleos push action eosio.token issue '[ "eosio", "10000000000.0000 EOS", "memo" ]' -p eosio
cleos push action eosio.token issue '[ "eosio", "10000000000.0000 SYS", "memo" ]' -p eosio
echo -e "      EOS TOKEN ISSUED"
cleos set contract eosio $EOSIO_CONTRACTS_ROOT/eosio.bios/
echo -e "      BIOS SET"
cleos set contract eosio $EOSIO_CONTRACTS_ROOT/eosio.system/
echo -e "      SYSTEM SET"
cleos push action eosio setpriv '["eosio.msig", 1]' -p eosio@active
cleos push action eosio init '["0", "4,EOS"]' -p eosio@active
#cleos push action eosio init '["0", "4,SYS"]' -p eosio@active

# Deploy eosio.wrap
echo -e "${CYAN}-----------------------EOSIO WRAP-----------------------${NC}"
cleos wallet import -n default --private-key 5J3JRDhf4JNhzzjEZAsQEgtVuqvsPPdZv4Tm6SjMRx1ZqToaray
cleos system newaccount eosio eosio.wrap EOS7LpGN1Qz5AbCJmsHzhG7sWEGd9mwhTXWmrYXqxhTknY2fvHQ1A --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos push action eosio setpriv '["eosio.wrap", 1]' -p eosio@active
cleos set contract eosio.wrap $EOSIO_CONTRACTS_ROOT/eosio.wrap/
cleos push action eosio setparams '{"params":{"max_block_net_usage": 1048576, "target_block_net_usage_pct": 1000, "max_transaction_net_usage": 524288, "base_per_transaction_net_usage": 12, "net_usage_leeway": 500, "context_free_discount_net_usage_num": 20, "context_free_discount_net_usage_den": 100, "max_block_cpu_usage": 500000, "target_block_cpu_usage_pct": 2500, "max_transaction_cpu_usage": 400000, "min_transaction_cpu_usage": 100, "max_transaction_lifetime": 3600, "deferred_trx_expiration_window": 600, "max_transaction_delay": 3888000, "max_inline_action_size": 4096, "max_inline_action_depth": 6, "max_authority_depth": 6}}' -p eosio

#=================================================================================#
# create the vigor1111112 account, set the contract, create VIGOR stablecoins

cleos system newaccount eosio vigor1111112 $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer
cleos set account permission vigor1111112 active '{"threshold":1,"keys":[{"key":"EOS6TnW2MQbZwXHWDHAYQazmdc3Sc1KGv4M9TSgsKZJSo43Uxs2Bx","weight":1}],"accounts":[{"permission":{"actor":"vigor1111112","permission":"eosio.code"},"weight":1}],"waits":[]}' -p vigor1111112@active
CONTRACT_ROOT=/home/gg/contracts/vigor/contracts/vigor/src
CONTRACT_OUT=/home/gg/contracts/vigor/contracts/vigor
CONTRACT_INCLUDE=/home/gg/contracts/vigor/contracts/vigor/include
CONTRACT_INCLUDE_BOOST=/usr/include
CONTRACT="vigor"
CONTRACT_WASM="$CONTRACT.wasm"
CONTRACT_ABI="$CONTRACT.abi"
CONTRACT_CPP="$CONTRACT.cpp"
eosio-cpp -contract=$CONTRACT -o="$CONTRACT_OUT/$CONTRACT_WASM" -I="$CONTRACT_INCLUDE" -I="$CONTRACT_INCLUDE_BOOST" -abigen "$CONTRACT_ROOT/$CONTRACT_CPP"
#eosio-cpp -contract=$CONTRACT -o="$CONTRACT_OUT/$CONTRACT_WASM" -I="$CONTRACT_INCLUDE" -I="$CONTRACT_INCLUDE_BOOST" -I="/home/gg/Var_/eigen-eigen-323c052e1731" -I="/home/gg/Var_/eigen-eigen-323c052e1731/src" -abigen "$CONTRACT_ROOT/$CONTRACT_CPP"


cleos set contract vigor1111112 $CONTRACT_OUT $CONTRACT_WASM $CONTRACT_ABI -p vigor1111112@active
cleos push action vigor1111112 create '[ "vigor1111112", "1000000000.0000 VIGOR"]' -p vigor1111112@active
cleos push action vigor1111112 setsupply '[ "vigor1111112", "1000000000.0000 VIGOR"]' -p vigor1111112@active


#cd ~/contracts/eosio.cdt/examples/hello/src
#eosio-cpp -contract=hello -o=hello.wasm -I=../include -abigen hello.cpp
#cleos set contract vigor1111112 . hello.wasm hello.abi -p vigor1111112@active
#cleos push action vigor1111112 hi '["name","asdf"]' -p vigor1111112@active
#=================================================================================#

cleos --verbose system newaccount eosio testbrw11111 $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer
cleos --verbose system newaccount eosio testbrw11112 $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer
cleos --verbose system newaccount eosio testins11111 $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer
cleos --verbose system newaccount eosio testins11112 $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer
cleos --verbose system newaccount eosio finalreserve $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer
cleos --verbose system newaccount eosio reinvestment $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer

cleos --verbose system newaccount eosio testbrw21111 $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer
cleos --verbose system newaccount eosio testbrw21112 $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer
cleos --verbose system newaccount eosio testins21111 $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer
cleos --verbose system newaccount eosio testins21112 $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer

cleos --verbose push action eosio.token transfer '[ "eosio", "testbrw11111", "1000000.0000 EOS", "m" ]' -p eosio
cleos --verbose push action eosio.token transfer '[ "eosio", "testbrw11112", "1000000.0000 EOS", "m" ]' -p eosio
cleos --verbose push action eosio.token transfer '[ "eosio", "testins11111", "1000000.0000 EOS", "m" ]' -p eosio
cleos --verbose push action eosio.token transfer '[ "eosio", "testins11112", "1000000.0000 EOS", "m" ]' -p eosio
cleos --verbose push action eosio.token transfer '[ "eosio", "finalreserve", "1000000.0000 EOS", "m" ]' -p eosio
cleos --verbose push action eosio.token transfer '[ "eosio", "reinvestment", "1000000.0000 EOS", "m" ]' -p eosio

cleos --verbose push action eosio.token transfer '[ "eosio", "testbrw21111", "1000000.0000 EOS", "m" ]' -p eosio
cleos --verbose push action eosio.token transfer '[ "eosio", "testbrw21112", "1000000.0000 EOS", "m" ]' -p eosio
cleos --verbose push action eosio.token transfer '[ "eosio", "testins21111", "1000000.0000 EOS", "m" ]' -p eosio
cleos --verbose push action eosio.token transfer '[ "eosio", "testins21112", "1000000.0000 EOS", "m" ]' -p eosio

#=================================================================================#
#cleos system newaccount eosio profile11111 $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer
#cleos set account permission profile11111 active '{"threshold":1,"keys":[{"key":"EOS6TnW2MQbZwXHWDHAYQazmdc3Sc1KGv4M9TSgsKZJSo43Uxs2Bx","weight":1}],"accounts":[{"permission":{"actor":"profile11111","permission":"eosio.code"},"weight":1}],"waits":[]}' -p profile11111@active
#CONTRACT_ROOT=/home/gg/contracts/vigor/contracts/vigor/src
#CONTRACT_OUT=/home/gg/contracts/vigor/contracts/vigor
#CONTRACT_INCLUDE=/home/gg/contracts/vigor/contracts/vigor/include
#CONTRACT="profile"
#CONTRACT_WASM="$CONTRACT.wasm"
#CONTRACT_ABI="$CONTRACT.abi"
#CONTRACT_CPP="$CONTRACT.cpp"
#eosio-cpp -contract=$CONTRACT -o="$CONTRACT_OUT/$CONTRACT_WASM" -I="$CONTRACT_INCLUDE" -abigen "$CONTRACT_ROOT/$CONTRACT_CPP"
#cleos set contract profile11111 $CONTRACT_OUT $CONTRACT_WASM $CONTRACT_ABI -p profile11111@active
#cleos push action profile11111 update '[ "testbrw11111", "nicktestbrw11111", "avattestbrw11111","webtestbrw11111", "loctestbrw11111", "metatestbrw11111",1.0]' -p testbrw11111@active
#cleos get table profile11111 profile11111 profiles --limit -1

#=================================================================================#
# create the VIG token
cleos system newaccount eosio vig111111111 $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer

cleos set contract vig111111111 $EOSIO_CONTRACTS_ROOT/eosio.token/ -p vig111111111@active
cleos push action vig111111111 create '[ "vig111111111", "100000000000.0000 VIG"]' -p vig111111111@active
cleos push action vig111111111 issue '[ "vig111111111", "10000000000.0000 VIG", "m"]' -p vig111111111@active
cleos push action vig111111111 transfer '[ "vig111111111", "testbrw11111", "1000000.0000 VIG", "m" ]' -p vig111111111@active
cleos push action vig111111111 transfer '[ "vig111111111", "testbrw11112", "1000000.0000 VIG", "m" ]' -p vig111111111@active
cleos push action vig111111111 transfer '[ "vig111111111", "testins11111", "1000000.0000 VIG", "m" ]' -p vig111111111@active
cleos push action vig111111111 transfer '[ "vig111111111", "testins11112", "1000000.0000 VIG", "m" ]' -p vig111111111@active
cleos push action vig111111111 transfer '[ "vig111111111", "finalreserve", "1000000.0000 VIG", "m" ]' -p vig111111111@active

cleos push action vig111111111 transfer '[ "vig111111111", "testbrw21111", "1000000.0000 VIG", "m" ]' -p vig111111111@active
cleos push action vig111111111 transfer '[ "vig111111111", "testbrw21112", "1000000.0000 VIG", "m" ]' -p vig111111111@active
cleos push action vig111111111 transfer '[ "vig111111111", "testins21111", "1000000.0000 VIG", "m" ]' -p vig111111111@active
cleos push action vig111111111 transfer '[ "vig111111111", "testins21112", "1000000.0000 VIG", "m" ]' -p vig111111111@active

#=================================================================================#
# create dummy tokens
cleos system newaccount eosio dummytokensx $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer
cleos set contract dummytokensx $EOSIO_CONTRACTS_ROOT/eosio.token/ -p dummytokensx@active

cleos push action dummytokensx create '[ "dummytokensx", "100000000000.000 IQ"]' -p dummytokensx@active

cleos push action dummytokensx issue '[ "dummytokensx", "10000000000.000 IQ", "m"]' -p dummytokensx@active
cleos push action dummytokensx transfer '[ "dummytokensx", "testbrw11111", "100000.000 IQ", "m" ]' -p dummytokensx@active
cleos push action dummytokensx transfer '[ "dummytokensx", "testbrw11112", "100000.000 IQ", "m" ]' -p dummytokensx@active
cleos push action dummytokensx transfer '[ "dummytokensx", "testins11111", "100000.000 IQ", "m" ]' -p dummytokensx@active
cleos push action dummytokensx transfer '[ "dummytokensx", "testins11112", "100000.000 IQ", "m" ]' -p dummytokensx@active
cleos push action dummytokensx transfer '[ "dummytokensx", "finalreserve", "100000.000 IQ", "m" ]' -p dummytokensx@active

cleos push action dummytokensx transfer '[ "dummytokensx", "testbrw21111", "100000.000 IQ", "m" ]' -p dummytokensx@active
cleos push action dummytokensx transfer '[ "dummytokensx", "testbrw21112", "100000.000 IQ", "m" ]' -p dummytokensx@active
cleos push action dummytokensx transfer '[ "dummytokensx", "testins21111", "100000.000 IQ", "m" ]' -p dummytokensx@active
cleos push action dummytokensx transfer '[ "dummytokensx", "testins21112", "100000.000 IQ", "m" ]' -p dummytokensx@active

#=================================================================================#
# create the oracle contract for local testnet
cleos system newaccount eosio oracleoracl2 $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer
#cleos system newaccount eosio eostitanprod $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer
cleos system newaccount eosio feeder111111 $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer
cleos system newaccount eosio feeder111112 $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer
cleos system newaccount eosio feeder111113 $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer
cleos system newaccount eosio datapreproc2 $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer

ORACLE_ROOT=/home/gg/contracts/vigor/contracts/oracle/src
ORACLE_OUT=/home/gg/contracts/vigor/contracts/oracle
ORACLE_INCLUDE=/home/gg/contracts/vigor/contracts/oracle/include
ORACLE="oracle"
ORACLE_WASM="$ORACLE.wasm"
ORACLE_ABI="$ORACLE.abi"
ORACLE_CPP="$ORACLE.cpp"
EOSIO_CONTRACTS_ROOT=/home/gg/contracts/eosio.contracts/contracts
#eosio-cpp -contract=$ORACLE -I=$ORACLE_INCLUDE -I=$EOSIO_CONTRACTS_ROOT/eosio.system/include -o="$ORACLE_OUT/$ORACLE_WASM" -abigen "$ORACLE_ROOT/$ORACLE_CPP" 
cleos set contract oracleoracl2 $ORACLE_OUT $ORACLE_WASM $ORACLE_ABI -p oracleoracl2@active
cleos push action oracleoracl2 configure '{}' -p oracleoracl2@active
cd /home/gg/contracts/vigor/contracts/oracle && ORACLE=feeder111111 node updater_eosusd.js
cd /home/gg/contracts/vigor/contracts/oracle && ORACLE=feeder111112 node updater_eosusd.js
cd /home/gg/contracts/vigor/contracts/oracle && ORACLE=feeder111113 node updater_eosusd.js
cd /home/gg/contracts/vigor/contracts/oracle && ORACLE=feeder111111 node updater_iqeos.js
cd /home/gg/contracts/vigor/contracts/oracle && ORACLE=feeder111112 node updater_iqeos.js
cd /home/gg/contracts/vigor/contracts/oracle && ORACLE=feeder111113 node updater_iqeos.js
cd /home/gg/contracts/vigor/contracts/oracle && ORACLE=feeder111111 node updater_vigeos.js
cd /home/gg/contracts/vigor/contracts/oracle && ORACLE=feeder111112 node updater_vigeos.js
cd /home/gg/contracts/vigor/contracts/oracle && ORACLE=feeder111113 node updater_vigeos.js

#cleos push action oracleoracl2 write '{"owner": "feeder111111","quotes": [{"value":"20000","pair":"eosusd", "base": {"sym": "4,EOS", "con": "eosio.token"}}]}' -p feeder111111@active
#cleos push action oracleoracl2 write '{"owner": "feeder111111","quotes": [{"value":"10000","pair":"eosusd"},{"value":"80000","pair":"eosbtc"}]}' -p feeder111111@active
#cleos push action oracleoracl2 write '{"owner": "feeder111112","quotes": [{"value":"20000","pair":"eosusd"},{"value":"80000","pair":"eosbtc"}]}' -p feeder111112@active
#cleos push action oracleoracl2 write '{"owner": "feeder111113","quotes": [{"value":"30000","pair":"eosusd"},{"value":"80000","pair":"eosbtc"}]}' -p feeder111113@active
#cleos push action oracleoracl2 write '{"owner": "feeder111111","quotes": [{"value":"70000","pair":"eosusd"},{"value":"70000","pair":"eosbtc"}]}' -p feeder111111@active
#cleos push action oracleoracl2 write '{"owner": "feeder111111","quotes": [{"value":"60000","pair":"eosusd"},{"value":"60000","pair":"eosbtc"}]}' -p feeder111111@active
#cleos push action oracleoracl2 write '{"owner": "feeder111111","quotes": [{"value":"50000","pair":"eosusd"},{"value":"50000","pair":"eosbtc"}]}' -p feeder111111@active
#cleos push action oracleoracl2 write '{"owner": "feeder111111","quotes": [{"value":"40000","pair":"eosusd"},{"value":"40000","pair":"eosbtc"}]}' -p feeder111111@active
#cleos push action oracleoracl2 write '{"owner": "feeder111111","quotes": [{"value":"30000","pair":"eosusd"},{"value":"30000","pair":"eosbtc"}]}' -p feeder111111@active
#cleos push action oracleoracl2 write '{"owner": "feeder111111","quotes": [{"value":"20000","pair":"eosusd"},{"value":"20000","pair":"eosbtc"}]}' -p feeder111111@active
#cleos push action oracleoracl2 write '{"owner": "feeder111111","quotes": [{"value":"10000","pair":"eosusd"},{"value":"10000","pair":"eosbtc"}]}' -p feeder111111@active
#cleos push action oracleoracl2 clear '{"pair":"eosusd"}' -p oracleoracl2@active
#cleos push action oracleoracl2 clear '{"pair":"eosbtc"}' -p oracleoracl2@active
cleos get table oracleoracl2 eosusd datapoints --limit -1
cleos get table oracleoracl2 iqeos datapoints --limit -1
cleos get table oracleoracl2 vigeos datapoints --limit -1
cleos get table oracleoracl2 oracleoracl2 stats
cleos get table oracleoracl2 oracleoracl2 pairs

CONTRACT_ROOT=/home/gg/contracts/vigor/contracts/datapreproc/src
CONTRACT_OUT=/home/gg/contracts/vigor/contracts/datapreproc
CONTRACT_INCLUDE=/home/gg/contracts/vigor/contracts/datapreproc/include
CONTRACT="datapreproc"
CONTRACT_WASM="$CONTRACT.wasm"
CONTRACT_ABI="$CONTRACT.abi"
CONTRACT_CPP="$CONTRACT.cpp"
EOSIO_CONTRACTS_ROOT=/home/gg/contracts/eosio.contracts/contracts
#eosio-cpp -contract=$CONTRACT -I=$CONTRACT_INCLUDE -I=$EOSIO_CONTRACTS_ROOT/eosio.system/include -o="$CONTRACT_OUT/$CONTRACT_WASM" -abigen "$CONTRACT_ROOT/$CONTRACT_CPP" 
cleos set contract datapreproc2 $CONTRACT_OUT $CONTRACT_WASM $CONTRACT_ABI -p datapreproc2@active
#cleos push action datapreproc2 clear '{}' -p datapreproc2@active
cleos push action datapreproc2 addpair '{"newpair":"eosusd"}' -p datapreproc2@active
cleos push action datapreproc2 addpair '{"newpair":"iqeos"}' -p datapreproc2@active
cleos push action datapreproc2 addpair '{"newpair":"vigeos"}' -p datapreproc2@active
cleos push action datapreproc2 addpair '{"newpair":"vigorusd"}' -p datapreproc2@active
#cleos push action datapreproc2 update '{}' -p feeder111111@active
#cleos push action datapreproc2 doshock '{"shockvalue":0.5}' -p feeder111111@active
cd /home/gg/contracts/vigor/contracts/oracle && CONTRACT=datapreproc2 OWNER=feeder111111 node dataupdate.js
cleos get table datapreproc2 datapreproc2 pairtoproc --limit -1
cleos get table datapreproc2 eosusd tseries
cleos get table datapreproc2 iqeos tseries
cleos get table datapreproc2 vigeos tseries
cleos get table datapreproc2 vigorusd tseries


#cleos -u http://api.eosnewyork.io:80 get code delphioracle --wasm -c delphioracle.wasm
#cleos -u http://api.eosnewyork.io:80 get code delphioracle --abi delphioracle.abi

#cleos set contract oracleoracl2 . delphioracle.wasm delphioracle.abi -p oracleoracl2@active

#cleos push action oracleoracl2 setoracles '{"oracleslist":["feeder111111"]}' -p eostitanprod@active

#=================================================================================#

###### OPTIONAL FOR LOCAL TESTNET #############
# cd ~/contracts/delphioracle/scripts
# nodeosurl='http://127.0.0.1:8888' interval=15000 account="oracleoracl2" defaultPrivateKey="5J3TQGkkiRQBKcg8Gg2a7Kk5a2QAQXsyGrkCnnq4krSSJSUkW12" feeder="feeder111111" node updater2.js
#cleos get table oracleoracl2 oracleoracl2 eosusd --limit 1
#cleos get table oracleoracl2 oracleoracl2 oracles
#cleos get table oracleoracl2 oracleoracl2 eosusdstats

# launch two oracle feeders
#in a new shell
#=================================================================================#
# exposed actions for vigor demo starts here

cleos --verbose push action eosio.token transfer '{"from":"finalreserve","to":"vigor1111112","quantity":"6.0000 EOS","memo":"insurance"}' -p finalreserve@active
cleos --verbose push action dummytokensx transfer '{"from":"finalreserve","to":"vigor1111112","quantity":"3000.000 IQ","memo":"insurance"}' -p finalreserve@active
cleos --verbose push action vig111111111 transfer '{"from":"finalreserve","to":"vigor1111112","quantity":"3000.0000 VIG","memo":"insurance"}' -p finalreserve@active

cleos --verbose push action eosio.token transfer '{"from":"reinvestment","to":"vigor1111112","quantity":"1.0000 EOS","memo":"collateral"}' -p reinvestment@active

cleos --verbose push action eosio.token transfer '{"from":"testbrw11111","to":"vigor1111112","quantity":"16.0000 EOS","memo":"collateral"}' -p testbrw11111@active
cleos --verbose push action dummytokensx transfer '{"from":"testbrw11111","to":"vigor1111112","quantity":"3000.000 IQ","memo":"collateral"}' -p testbrw11111@active
cleos --verbose push action vig111111111 transfer '{"from":"testbrw11111","to":"vigor1111112","quantity":"3000.0000 VIG","memo":"collateral"}' -p testbrw11111@active

cleos --verbose push action eosio.token transfer '{"from":"testbrw11112","to":"vigor1111112","quantity":"7.0000 EOS","memo":"collateral"}' -p testbrw11112@active
cleos --verbose push action dummytokensx transfer '{"from":"testbrw11112","to":"vigor1111112","quantity":"3000.000 IQ","memo":"collateral"}' -p testbrw11112@active
cleos --verbose push action vig111111111 transfer '{"from":"testbrw11112","to":"vigor1111112","quantity":"3000.0000 VIG","memo":"collateral"}' -p testbrw11112@active

cleos --verbose push action eosio.token transfer '{"from":"testins11111","to":"vigor1111112","quantity":"6.0000 EOS","memo":"insurance"}' -p testins11111@active
cleos --verbose push action dummytokensx transfer '{"from":"testins11111","to":"vigor1111112","quantity":"3000.000 IQ","memo":"insurance"}' -p testins11111@active
cleos --verbose push action eosio.token transfer '{"from":"testbrw11111","to":"vigor1111112","quantity":"6.0000 EOS","memo":"insurance"}' -p testbrw11111@active
cleos --verbose push action dummytokensx transfer '{"from":"testbrw11111","to":"vigor1111112","quantity":"3000.000 IQ","memo":"insurance"}' -p testbrw11111@active
cleos --verbose push action eosio.token transfer '{"from":"testins11112","to":"vigor1111112","quantity":"12.0000 EOS","memo":"insurance"}' -p testins11112@active
cleos --verbose push action dummytokensx transfer '{"from":"testins11112","to":"vigor1111112","quantity":"3000.000 IQ","memo":"insurance"}' -p testins11112@active

cleos --verbose push action vigor1111112 assetout '{"usern":"testbrw11111","assetout":"41.0001 VIGOR","memo":"borrow"}' -p testbrw11111@active
cleos --verbose push action vigor1111112 assetout '{"usern":"testbrw11112","assetout":"20.0000 VIGOR","memo":"borrow"}' -p testbrw11112@active

# withdraw VIG specified as precision 4 (internally it treat VIG as precision 10)
cleos --verbose push action vigor1111112 assetout '{"usern":"testbrw11111","assetout":"0.0001 VIG","memo":"collateral"}' -p testbrw11111@active 
#cleos --verbose push action vigor1111112 assetout '{"usern":"finalreserve","assetout":"1.0000 EOS","memo":"insurance"}' -p finalreserve@active
#cleos --verbose push action vigor1111112 assetout '{"usern":"testbrw11111","assetout":"1.0000 EOS","memo":"collateral"}' -p testbrw11111@active
#cleos --verbose push action vigor1111112 assetout '{"usern":"testbrw11112","assetout":"1.0000 EOS","memo":"collateral"}' -p testbrw11112@active
#cleos --verbose push action vigor1111112 assetout '{"usern":"testins11111","assetout":"1.0000 EOS","memo":"insurance"}' -p testins11111@active
#cleos --verbose push action vigor1111112 assetout '{"usern":"testins11112","assetout":"1.0000 EOS","memo":"insurance"}' -p testins11112@active
#cleos --verbose push action vigor1111112 assetout '{"usern":"testins21112","assetout":"11.0000 EOS","memo":"insurance"}' -p testins21112@active

cleos --verbose push action vigor1111112 transfer '{"from":"testbrw11111","to":"vigor1111112","quantity":"0.0001 VIGOR","memo":"payoff debt"}' -p testbrw11111@active
#cleos --verbose push action vigor1111112 transfer '{"from":"testbrw11112","to":"vigor1111112","quantity":"10.0000 VIGOR","memo":"payoff debt"}' -p testbrw11112@active

#cleos --verbose push action vigor1111112 doupdate '{}' -p vigor1111112@active
#sleep 5
#cleos --verbose push action vigor1111112 doupdate '' -p vigor1111112@active
#sleep 5
#cleos --verbose get table vigor1111112 vigor1111112 user

# get all the user data
cleos --verbose get table vigor1111112 vigor1111112 collateral
cleos --verbose get table vigor1111112 vigor1111112 user
cleos --verbose get table vigor1111112 vigor1111112 user -Ltestbrw11111 -Utestbrw11111
cleos --verbose get table vigor1111112 vigor1111112 user -Ltestbrw11112 -Utestbrw11112
cleos --verbose get table vigor1111112 vigor1111112 user -Ltestins11111 -Utestins11111
cleos --verbose get table vigor1111112 vigor1111112 user -Ltestins11112 -Utestins11112
cleos --verbose get table vigor1111112 vigor1111112 user -Lfinalreserve -Ufinalreserve
cleos --verbose get table vigor1111112 vigor1111112 globals

cleos --verbose push action datapreproc2 doshock '{"shockvalue":1.2}' -p feeder111111@active
cleos --verbose push action eosio.token transfer '{"from":"finalreserve","to":"vigor1111112","quantity":"0.0001 EOS","memo":"collateral"}' -p finalreserve@active

# cleos --verbose get table vigor1111112 VIGOR stat
# cleos --verbose get table eosio.token vigor1111112 accounts
# cleos --verbose get table eosio.token testbrw11111 accounts
# cleos --verbose get table vigor1111112 testbrw11111 accounts
# cleos --verbose get currency balance vigor1111112 testbrw11111
# cleos --verbose get currency balance eosio.token vigor1111112

#cleos --verbose -u https://api.kylin.alohaeos.com get table delphioracle delphioracle eosusd
#cleos --verbose -u https://api.kylin.alohaeos.com get table eostitantest eostitantest eosusd
#cleos --verbose -u https://api.kylin.alohaeos.com get table eostitantest eosusd datapoints
#cleos --verbose -u https://api.kylin.alohaeos.com get table eostitantest eosbtc datapoints
#cleos --verbose -u https://api.kylin.alohaeos.com get table eostitantest eostitantest stats
#cleos --verbose -u https://api.kylin.alohaeos.com get table eostitantest eostitantest pairs
#cleos --verbose -u https://api.kylin.alohaeos.com get table eosio eosio producers

#=================================================================================#
#lender.... borrow EOS native tokens against stablecoin collateral



cleos --verbose get table vigor1111112 testbrw11111 accounts
cleos --verbose get table vigor1111112 testbrw21111 accounts
cleos --verbose get table vigor1111112 testbrw21112 accounts
cleos --verbose get table vigor1111112 vigor1111112 accounts

cleos --verbose push action vigor1111112 transfer '{"from":"testbrw11111","to":"testbrw21111","quantity":"37.0000 VIGOR","memo":""}' -p testbrw11111@active
cleos --verbose push action vigor1111112 transfer '{"from":"testbrw11111","to":"testbrw21112","quantity":"4.0000 VIGOR","memo":""}' -p testbrw11111@active

#deposit and withdraw stablecoin collateral
cleos --verbose push action vigor1111112 transfer '{"from":"testbrw21111","to":"vigor1111112","quantity":"37.0000 VIGOR","memo":"collateral"}' -p testbrw21111@active
cleos --verbose push action vigor1111112 transfer '{"from":"testbrw21112","to":"vigor1111112","quantity":"4.0000 VIGOR","memo":"collateral"}' -p testbrw21112@active
cleos --verbose push action vigor1111112 assetout '{"usern":"testbrw21111","assetout":"1.0000 VIGOR","memo":"collateral"}' -p testbrw21111@active

#deposit and withdraw to the insurance pool
cleos --verbose push action eosio.token transfer '{"from":"testins21111","to":"vigor1111112","quantity":"2.0000 EOS","memo":"insurance"}' -p testins21111@active
cleos --verbose push action dummytokensx transfer '{"from":"testins21111","to":"vigor1111112","quantity":"3000.000 IQ","memo":"insurance"}' -p testins21111@active
cleos --verbose push action eosio.token transfer '{"from":"testbrw21111","to":"vigor1111112","quantity":"1.0000 EOS","memo":"insurance"}' -p testbrw21111@active
cleos --verbose push action dummytokensx transfer '{"from":"testbrw21111","to":"vigor1111112","quantity":"3000.000 IQ","memo":"insurance"}' -p testbrw21111@active
cleos --verbose push action eosio.token transfer '{"from":"testins21112","to":"vigor1111112","quantity":"12.0000 EOS","memo":"insurance"}' -p testins21112@active
cleos --verbose push action dummytokensx transfer '{"from":"testins21112","to":"vigor1111112","quantity":"3000.000 IQ","memo":"insurance"}' -p testins21112@active
cleos --verbose push action vigor1111112 transfer '{"from":"testbrw21111","to":"vigor1111112","quantity":"1.0000 VIGOR","memo":"insurance"}' -p testbrw21111@active

cleos --verbose push action vigor1111112 assetout '{"usern":"testins21111","assetout":"1.0000 EOS","memo":"insurance"}' -p testins21111@active
cleos --verbose push action vigor1111112 assetout '{"usern":"testins21112","assetout":"1.0000 EOS","memo":"insurance"}' -p testins21112@active
cleos --verbose push action vigor1111112 assetout '{"usern":"testbrw21111","assetout":"0.0001 VIGOR","memo":"insurance"}' -p testbrw21111@active
cleos --verbose push action vigor1111112 assetout '{"usern":"testbrw21111","assetout":"1.0000 EOS","memo":"insurance"}' -p testbrw21111@active

# borrow cryptos against stablecoin collateral
# Example: user has 50 VIGOR in l_debt as collateral against which user requests borrow 10 EOS transfered out
# locate crypto for borrow in the insurance pool
# a lending receipt is created into reinvestment account: ex: 10 EOS @ 30 VIGOR to user: l_lrtoken @ l_lrpayment to l_lrname
# 10 EOS booked into user (and global) l_collateral as a borrow, and 10 EOS subtracted from global insurance
# 30 VIGOR booked into reinvestment (and global) insurance to earn VIG, the earned VIG is paid to (and bailout recapped by) all insurance pool participants according to l_pcts

cleos --verbose push action vigor1111112 assetout '{"usern":"testbrw21111","assetout":"2.0000 EOS","memo":"borrow"}' -p testbrw21111@active
cleos --verbose push action vigor1111112 assetout '{"usern":"testbrw21111","assetout":"7.0000 EOS","memo":"borrow"}' -p testbrw21111@active
cleos --verbose push action vigor1111112 assetout '{"usern":"testbrw21111","assetout":"100.000 IQ","memo":"borrow"}' -p testbrw21111@active
cleos --verbose push action vigor1111112 assetout '{"usern":"testbrw21112","assetout":"1.3000 EOS","memo":"borrow"}' -p testbrw21112@active

#payback cryptos
# when user paysback borrowed crypto, loop through and payoff the outstanding locate receipts booked into the reinvestment account
cleos --verbose push action eosio.token transfer '{"from":"testbrw21111","to":"vigor1111112","quantity":"3.0000 EOS","memo":"payback borrowed token"}' -p testbrw21111@active
cleos --verbose push action eosio.token transfer '{"from":"testbrw21111","to":"vigor1111112","quantity":"3.0000 EOS","memo":"payback borrowed token"}' -p testbrw21111@active
cleos --verbose push action eosio.token transfer '{"from":"testbrw21111","to":"vigor1111112","quantity":"1.5000 EOS","memo":"payback borrowed token"}' -p testbrw21111@active
cleos --verbose push action dummytokensx transfer '{"from":"testbrw11111","to":"vigor1111112","quantity":"3000.000 IQ","memo":"payback borrowed token"}' -p testbrw11111@active
cleos --verbose push action dummytokensx transfer '{"from":"testbrw21111","to":"vigor1111112","quantity":"100.000 IQ","memo":"payback borrowed token"}' -p testbrw21111@active
cleos --verbose push action vig111111111 transfer '{"from":"testbrw11111","to":"vigor1111112","quantity":"3000.0000 VIG","memo":"payback borrowed token"}' -p testbrw11111@active
cleos --verbose push action eosio.token transfer '{"from":"testbrw21111","to":"vigor1111112","quantity":"1.5000 EOS","memo":"payback borrowed token"}' -p testbrw21111@active
cleos --verbose push action eosio.token transfer '{"from":"testbrw21112","to":"vigor1111112","quantity":"1.0000 EOS","memo":"payback borrowed token"}' -p testbrw21112@active


#reserve should not pay VIG for loan insurance because nobody backs the reserve except itself, reserve needs to have a self bailout function which basically re-ups the reserve collateral



# get all the user data
cleos --verbose get table vigor1111112 vigor1111112 user
cleos --verbose get table vigor1111112 vigor1111112 user -Ltestbrw21111 -Utestbrw21111
cleos --verbose get table vigor1111112 vigor1111112 user -Ltestbrw21112 -Utestbrw21112
cleos --verbose get table vigor1111112 vigor1111112 user -Ltestins21111 -Utestins21111
cleos --verbose get table vigor1111112 vigor1111112 user -Ltestins21112 -Utestins21112
cleos --verbose get table vigor1111112 vigor1111112 user -Lfinalreserve -Ufinalreserve
cleos --verbose get table vigor1111112 vigor1111112 user -Lreinvestment -Ureinvestment
cleos --verbose get table vigor1111112 vigor1111112 globals
