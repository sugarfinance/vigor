alias cleos="docker exec -it nodeos cleos --url http://127.0.0.1:8888 --wallet-url http://keosd:8901"

cleos wallet unlock --password PW5KKZ5MG8iKr5uKKFm9yvvRtVzZ31syTHoDkg3Lk9N9uLWAVRBNo
# priv 5J7gFHsbnU7EpQ4RGrdhAoMnvXnJvWgfiRtvTdMCcVfT6DGkNrK

# set up system contracts (eosio.bios and eosio.token)
docker exec -it nodeos  bash -c "cd /eosio.cdt/eosio.contracts/contracts/ &&  eosio-cpp -I eosio.token/include/ -abigen -o eosio.token/src/eosio.token.wasm eosio.token/src/eosio.token.cpp"
cleos create account eosio eosio.token EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos set contract eosio.token /eosio.cdt/eosio.contracts/contracts/eosio.token/src eosio.token.wasm eosio.token.abi -p eosio.token@active
cleos push action eosio.token create '[ "eosio", "1000000000.0000 EOS"]' -p eosio.token@active

# testuser rename to 12
cleos create account eosio testuser EOS6K42yrrMETmx2rXFJeKtaGrQAwgCDBYUVY7PGCVfhWFykqvhVR EOS6K42yrrMETmx2rXFJeKtaGrQAwgCDBYUVY7PGCVfhWFykqvhVR
cleos push action eosio.token issue '[ "testuser", "100000.0000 EOS", "m" ]' -p eosio@active
cleos get currency balance eosio.token testuser

# eosusd
docker exec -it nodeos  bash -c "cd /eosio.cdt && eosio-cpp -abigen -o contract/eosusdcom.wasm contract/eosusdcom.cpp"
cleos create account eosio eosusdcom111 EOS6RfWmDnkuHe7BoPMSeirFgz7rC8fSSVYD5Qqe8QqvzBoVLEbMg EOS6RfWmDnkuHe7BoPMSeirFgz7rC8fSSVYD5Qqe8QqvzBoVLEbMg
cleos set account permission eosusdcom111 active '{"threshold":1,"keys":[{"key":"EOS6RfWmDnkuHe7BoPMSeirFgz7rC8fSSVYD5Qqe8QqvzBoVLEbMg","weight":1}],"accounts":[{"permission":{"actor":"eosusdcom111","permission":"eosio.code"},"weight":1}],"waits":[]}' owner -p eosusdcom111@owner
cleos set contract eosusdcom111 /eosio.cdt contract/eosusdcom.wasm contract/eosusdcom.abi -p eosusdcom111@active

cleos push action eosusdcom111 create '[ "eosusdcom111", "1000000000.0000 UZD"]' -p eosusdcom111@active
cleos push action eosusdcom111 setsupply '[ "eosusdcom111", "1000000000.0000 UZD"]' -p eosusdcom111@active

# create 2 borrowers and 2 insurers
cleos create account eosio testborrow11 EOS6K42yrrMETmx2rXFJeKtaGrQAwgCDBYUVY7PGCVfhWFykqvhVR EOS6K42yrrMETmx2rXFJeKtaGrQAwgCDBYUVY7PGCVfhWFykqvhVR
cleos create account eosio testinsure11 EOS6K42yrrMETmx2rXFJeKtaGrQAwgCDBYUVY7PGCVfhWFykqvhVR EOS6K42yrrMETmx2rXFJeKtaGrQAwgCDBYUVY7PGCVfhWFykqvhVR
cleos create account eosio testborrow12 EOS6K42yrrMETmx2rXFJeKtaGrQAwgCDBYUVY7PGCVfhWFykqvhVR EOS6K42yrrMETmx2rXFJeKtaGrQAwgCDBYUVY7PGCVfhWFykqvhVR
cleos create account eosio testinsure12 EOS6K42yrrMETmx2rXFJeKtaGrQAwgCDBYUVY7PGCVfhWFykqvhVR EOS6K42yrrMETmx2rXFJeKtaGrQAwgCDBYUVY7PGCVfhWFykqvhVR
cleos push action eosio.token issue '[ "testborrow11", "100000.0000 EOS", "m" ]' -p eosio@active
cleos push action eosio.token issue '[ "testborrow12", "100000.0000 EOS", "m" ]' -p eosio@active
cleos push action eosio.token issue '[ "testinsure11", "100000.0000 EOS", "m" ]' -p eosio@active
cleos push action eosio.token issue '[ "testinsure12", "100000.0000 EOS", "m" ]' -p eosio@active

# create the VIG token
cleos create account eosio vig111111111 EOS6K42yrrMETmx2rXFJeKtaGrQAwgCDBYUVY7PGCVfhWFykqvhVR EOS6K42yrrMETmx2rXFJeKtaGrQAwgCDBYUVY7PGCVfhWFykqvhVR
cleos set contract vig111111111 /eosio.cdt/eosio.contracts/contracts/eosio.token/src eosio.token.wasm eosio.token.abi -p vig111111111@active
cleos push action vig111111111 create '[ "vig111111111", "1000000000.0000 VIG"]' -p vig111111111@active
cleos push action vig111111111 issue '[ "testborrow11", "100000.0000 VIG", "m" ]' -p vig111111111@active
cleos push action vig111111111 issue '[ "testborrow12", "100000.0000 VIG", "m" ]' -p vig111111111@active
cleos push action vig111111111 issue '[ "testinsure11", "100000.0000 VIG", "m" ]' -p vig111111111@active
cleos push action vig111111111 issue '[ "testinsure12", "100000.0000 VIG", "m" ]' -p vig111111111@active

# create dummy tokens
cleos create account eosio dummytokens1 EOS6K42yrrMETmx2rXFJeKtaGrQAwgCDBYUVY7PGCVfhWFykqvhVR EOS6K42yrrMETmx2rXFJeKtaGrQAwgCDBYUVY7PGCVfhWFykqvhVR
cleos set contract dummytokens1 /eosio.cdt/eosio.contracts/contracts/eosio.token/src eosio.token.wasm eosio.token.abi -p dummytokens1@active

cleos push action dummytokens1 create '[ "dummytokens1", "1000000000.0000 IQ"]' -p dummytokens1@active
cleos push action dummytokens1 create '[ "dummytokens1", "1000000000.0000 UTG"]' -p dummytokens1@active
cleos push action dummytokens1 create '[ "dummytokens1", "1000000000.0000 PTI"]' -p dummytokens1@active
cleos push action dummytokens1 create '[ "dummytokens1", "1000000000.0000 OWN"]' -p dummytokens1@active

cleos push action dummytokens1 issue '[ "testborrow11", "100000.0000 IQ", "m" ]' -p dummytokens1@active
cleos push action dummytokens1 issue '[ "testborrow11", "100000.0000 UTG", "m" ]' -p dummytokens1@active
cleos push action dummytokens1 issue '[ "testborrow11", "100000.0000 PTI", "m" ]' -p dummytokens1@active
cleos push action dummytokens1 issue '[ "testborrow11", "100000.0000 OWN", "m" ]' -p dummytokens1@active

cleos push action dummytokens1 issue '[ "testborrow12", "100000.0000 IQ", "m" ]' -p dummytokens1@active
cleos push action dummytokens1 issue '[ "testborrow12", "100000.0000 UTG", "m" ]' -p dummytokens1@active
cleos push action dummytokens1 issue '[ "testborrow12", "100000.0000 PTI", "m" ]' -p dummytokens1@active
cleos push action dummytokens1 issue '[ "testborrow12", "100000.0000 OWN", "m" ]' -p dummytokens1@active

cleos push action dummytokens1 issue '[ "testinsure11", "100000.0000 IQ", "m" ]' -p dummytokens1@active
cleos push action dummytokens1 issue '[ "testinsure11", "100000.0000 UTG", "m" ]' -p dummytokens1@active
cleos push action dummytokens1 issue '[ "testinsure11", "100000.0000 PTI", "m" ]' -p dummytokens1@active
cleos push action dummytokens1 issue '[ "testinsure11", "100000.0000 OWN", "m" ]' -p dummytokens1@active

cleos push action dummytokens1 issue '[ "testinsure12", "100000.0000 IQ", "m" ]' -p dummytokens1@active
cleos push action dummytokens1 issue '[ "testinsure12", "100000.0000 UTG", "m" ]' -p dummytokens1@active
cleos push action dummytokens1 issue '[ "testinsure12", "100000.0000 PTI", "m" ]' -p dummytokens1@active
cleos push action dummytokens1 issue '[ "testinsure12", "100000.0000 OWN", "m" ]' -p dummytokens1@active

# create the oracle contract (for EOS only for now)
cleos create account eosio oracle111111 EOS6K42yrrMETmx2rXFJeKtaGrQAwgCDBYUVY7PGCVfhWFykqvhVR EOS6K42yrrMETmx2rXFJeKtaGrQAwgCDBYUVY7PGCVfhWFykqvhVR
cleos create account eosio eostitanprod EOS6K42yrrMETmx2rXFJeKtaGrQAwgCDBYUVY7PGCVfhWFykqvhVR EOS6K42yrrMETmx2rXFJeKtaGrQAwgCDBYUVY7PGCVfhWFykqvhVR
cleos create account eosio feeder111111 EOS6K42yrrMETmx2rXFJeKtaGrQAwgCDBYUVY7PGCVfhWFykqvhVR EOS6K42yrrMETmx2rXFJeKtaGrQAwgCDBYUVY7PGCVfhWFykqvhVR
cleos create account eosio feeder211111 EOS6K42yrrMETmx2rXFJeKtaGrQAwgCDBYUVY7PGCVfhWFykqvhVR EOS6K42yrrMETmx2rXFJeKtaGrQAwgCDBYUVY7PGCVfhWFykqvhVR

docker exec -it nodeos cleos -u http://api.eosnewyork.io:80 get code delphioracle --wasm -c delphioracle.wasm
docker exec -it nodeos cleos -u http://api.eosnewyork.io:80 get code delphioracle --abi delphioracle.abi
cleos set contract oracle111111 / delphioracle.wasm delphioracle.abi -p oracle111111@active

cleos push action oracle111111 setoracles '{"oracleslist":["feeder111111","feeder211111"]}' -p eostitanprod@active
cleos push action oracle111111 write '{"owner":"feeder111111", "value":63800}' -p feeder111111@active

# Install oracle updater deps
docker exec -it nodeos bash -c "cd /scripts && npm install eosjs node-fetch axios --save"

## In other terminals (docker exec -it nodeos bash)
# nodeosurl='http://127.0.0.1:8888' interval=15000 account="oracle111111" defaultPrivateKey="5J7gFHsbnU7EpQ4RGrdhAoMnvXnJvWgfiRtvTdMCcVfT6DGkNrK" feeder="feeder111111" node /scripts/updater2.js
# nodeosurl='http://127.0.0.1:8888' interval=15000 account="oracle111111" defaultPrivateKey="5J7gFHsbnU7EpQ4RGrdhAoMnvXnJvWgfiRtvTdMCcVfT6DGkNrK" feeder="feeder211111" node updater2.js


##############################################
# exposed actions for eosusdcom demo starts here

# .. see test.sh

## The demo actios work but these fails

# $ cleos push action eosusdcom111 borrow '{"usern":"testborrow11","debt":"24.0000 UZD"}' -p testborrow11@active
# Error 3050003: eosio_assert_message assertion failure
# Error Details:
# assertion failure with message: symbol does not exist

# $ cleos push action eosusdcom111 transfer '{"from":"testborrow11","to":"eosusdcom111","quantity":"1.0000 UZD","memo":"payoff debt"}' -p testborrow11@active
# Error 3050003: eosio_assert_message assertion failure
# Error Details:
# assertion failure with message: Payment too high

###