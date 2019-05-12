#!/bin/bash
# Usage: ./test_microauctions.sh

#=================================================================================#
# SETUP
# 
pkill nodeos
rm -rf ~/.local/share/eosio/nodeos/data
nodeos -e -p eosio --http-validate-host=false --delete-all-blocks --contracts-console --plugin eosio::chain_api_plugin --plugin eosio::history_api_plugin --plugin eosio::producer_plugin --plugin eosio::http_plugin --max-transaction-time=10000

CYAN='\033[1;36m'
NC='\033[0m'

# CHANGE PATH
EOSIO_CONTRACTS_ROOT=/home/ab/contracts1.6.0/eosio.contracts/contracts
CONTRACT_ROOT=/home/ab/contracts1.6.0/microauctions/contracts
CONTRACT="microauctions"
CONTRACT_WASM="$CONTRACT.wasm"
CONTRACT_ABI="$CONTRACT.abi"
CONTRACT_CPP="$CONTRACT.cpp"

OWNER_KEY="EOS6TnW2MQbZwXHWDHAYQazmdc3Sc1KGv4M9TSgsKZJSo43Uxs2Bx"
OWNER_ACCT="5J3TQGkkiRQBKcg8Gg2a7Kk5a2QAQXsyGrkCnnq4krSSJSUkW12"

#cleos wallet create --to-console
cleos wallet unlock -n default --password PW5KDyCJVL3ypUGia4yf5TatcCQ4UjyrDQ296Dh2pe8ZjrLVDPh91
cleos wallet import -n default --private-key $OWNER_ACCT

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

# Bootstrap new system contracts
echo -e "${CYAN}-----------------------SYSTEM CONTRACTS-----------------------${NC}"
#eosio-cpp -I $EOSIO_CONTRACTS_ROOT/eosio.msig/include -o $EOSIO_CONTRACTS_ROOT/eosio.msig/eosio.msig.wasm $EOSIO_CONTRACTS_ROOT/eosio.msig/src/eosio.msig.cpp --abigen
#eosio-cpp -I $EOSIO_CONTRACTS_ROOT/eosio.system/include -I $EOSIO_CONTRACTS_ROOT/eosio.token/include -o $EOSIO_CONTRACTS_ROOT/eosio.system/eosio.system.wasm $EOSIO_CONTRACTS_ROOT/eosio.system/src/eosio.system.cpp --abigen
#eosio-cpp -I $EOSIO_CONTRACTS_ROOT/eosio.wrap/include -o $EOSIO_CONTRACTS_ROOT/eosio.wrap/eosio.wrap.wasm $EOSIO_CONTRACTS_ROOT/eosio.wrap/src/eosio.wrap.cpp --abigen
cleos set contract eosio.token $EOSIO_CONTRACTS_ROOT/eosio.token/
cleos set contract eosio.msig $EOSIO_CONTRACTS_ROOT/eosio.msig/
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
cleos push action eosio init '[0, "4,EOS"]' -p eosio@active
#cleos push action eosio init '[0, "4,SYS"]' -p eosio@active

# Deploy eosio.wrap
echo -e "${CYAN}-----------------------EOSIO WRAP-----------------------${NC}"
cleos wallet import -n default --private-key 5J3JRDhf4JNhzzjEZAsQEgtVuqvsPPdZv4Tm6SjMRx1ZqToaray
cleos system newaccount eosio eosio.wrap EOS7LpGN1Qz5AbCJmsHzhG7sWEGd9mwhTXWmrYXqxhTknY2fvHQ1A --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos push action eosio setpriv '["eosio.wrap", 1]' -p eosio@active
cleos set contract eosio.wrap $EOSIO_CONTRACTS_ROOT/eosio.wrap/

#=================================================================================#
# create the genesistrack account, set the contract, create UZD stablecoins

cleos system newaccount eosio genesistrack $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer
cleos set account permission genesistrack active '{"threshold":1,"keys":[{"key":"EOS6TnW2MQbZwXHWDHAYQazmdc3Sc1KGv4M9TSgsKZJSo43Uxs2Bx","weight":1}],"accounts":[{"permission":{"actor":"genesistrack","permission":"eosio.code"},"weight":1}],"waits":[]}' -p genesistrack@active
eosio-cpp -I $CONTRACT_ROOT -o "$CONTRACT_ROOT/$CONTRACT_WASM" "$CONTRACT_ROOT/$CONTRACT_CPP" --abigen
cleos set contract genesistrack $CONTRACT_ROOT $CONTRACT_WASM $CONTRACT_ABI -p genesistrack@active

cleos system newaccount eosio testbuy11111 $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer
cleos system newaccount eosio testbuy11112 $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer
cleos system newaccount eosio testsavings1 $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer

cleos push action eosio.token transfer '[ "eosio", "testbuy11111", "1000000.0000 EOS", "m" ]' -p eosio
cleos push action eosio.token transfer '[ "eosio", "testbuy11112", "1000000.0000 EOS", "m" ]' -p eosio

#=================================================================================#
# create the VIG token
cleos system newaccount eosio vig111111111 $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer
cleos set contract vig111111111 $EOSIO_CONTRACTS_ROOT/eosio.token/ -p vig111111111@active
cleos push action vig111111111 create '[ "vig111111111", "1000000000.0000 VIG"]' -p vig111111111@active
cleos push action vig111111111 issue '[ "vig111111111", "1000000000.0000 VIG", "m"]' -p vig111111111@active
#=================================================================================#

#=================================================================================#
# start an auction

#start_ts; //starting time
#payouts_per_payin;  // how many outbound transfers to trigger on each inbound
#payouts_delay_sec;  // defferred transaction delay in seconds
#payout_cycles_per_user; // how many cycles aggregated per transfer for a user
 
 echo '{"setting":
     {"whitelist": "genesistrack",
                    "tokens_account": "genesistrack",
                    "savings_account": "testsavings1",
                    "cycles": 3,
                    "seconds_per_cycle": 60,
                    "start_ts": '$(($(date +%s%N)/1000))',
                    "quota_per_cycle": {
                            "quantity": "100.0000 VIG",
                            "contract": "vig111111111",
                    },
                    "accepted_token": {
                            "quantity": "0.1000 EOS",
                            "contract": "eosio.token",
                    },
                    "payouts_per_payin": 0,
                    "payouts_delay_sec": 5,
                    "payout_cycles_per_user": 10}
                    }'> "$CONTRACT_ROOT/settings.json"

cleos push action genesistrack init "$CONTRACT_ROOT/settings.json" -p genesistrack
cleos push action vig111111111 transfer '[ "vig111111111", "genesistrack", "100000000.0000 VIG", "seed transfer" ]' -p vig111111111@active

#buy from 0th cycle
cleos push action eosio.token transfer '[ "testbuy11111", "genesistrack", "100.0000 EOS", "" ]' -p testbuy11111@active
cleos push action eosio.token transfer '[ "testbuy11112", "genesistrack", "200.0000 EOS", "" ]' -p testbuy11112@active
cleos push action genesistrack claim '[ "testbuy11111"]' -p testbuy11111@active
cleos push action genesistrack claim '[ "testbuy11112"]' -p testbuy11111@active

#pause 60 seconds
#buy from 1st cycle
cleos push action eosio.token transfer '[ "testbuy11111", "genesistrack", "100.0000 EOS", "" ]' -p testbuy11111@active
cleos push action eosio.token transfer '[ "testbuy11112", "genesistrack", "200.0000 EOS", "" ]' -p testbuy11112@active
cleos push action genesistrack claim '[ "testbuy11111"]' -p testbuy11111@active
cleos push action genesistrack claim '[ "testbuy11112"]' -p testbuy11111@active

#check balances of all users, and sale proceeds were sent to savings
cleos get table genesistrack genesistrack payment
cleos get table genesistrack genesistrack cycle
cleos get table vig111111111 genesistrack accounts
cleos get table vig111111111 vig111111111 accounts
cleos get table vig111111111 testbuy11111 accounts
cleos get table vig111111111 testbuy11112 accounts
cleos get table eosio.token testsavings1 accounts