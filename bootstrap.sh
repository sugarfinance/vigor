alias cleos="docker exec -it nodeos cleos --url http://127.0.0.1:8888 --wallet-url http://keosd:8901"

cleos wallet unlock --password PW5KKZ5MG8iKr5uKKFm9yvvRtVzZ31syTHoDkg3Lk9N9uLWAVRBNo

docker exec -it nodeos  bash -c "cd /eosio.cdt/eosio.contracts/contracts/ &&  eosio-cpp -I eosio.token/include/ -abigen -o eosio.token/src/eosio.token.wasm eosio.token/src/eosio.token.cpp"
cleos create account eosio eosio.token EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos set contract eosio.token /eosio.cdt/eosio.contracts/contracts/eosio.token/src eosio.token.wasm eosio.token.abi -p eosio.token@active

# testuser
cleos create account eosio testuser EOS6K42yrrMETmx2rXFJeKtaGrQAwgCDBYUVY7PGCVfhWFykqvhVR EOS6K42yrrMETmx2rXFJeKtaGrQAwgCDBYUVY7PGCVfhWFykqvhVR
cleos push action eosio.token create '[ "eosio", "1000000000.0000 EOS"]' -p eosio.token@active
cleos push action eosio.token issue '[ "testuser", "100000.0000 EOS", "m" ]' -p eosio@active
cleos get currency balance eosio.token testuser

# eosusd
docker exec -it nodeos  bash -c "cd /eosio.cdt && eosio-cpp -abigen -o contract/eosusd.wasm contract/eosusd.cpp"
cleos create account eosio eosusdeosusd EOS6RfWmDnkuHe7BoPMSeirFgz7rC8fSSVYD5Qqe8QqvzBoVLEbMg EOS6RfWmDnkuHe7BoPMSeirFgz7rC8fSSVYD5Qqe8QqvzBoVLEbMg
cleos set account permission eosusdeosusd active '{"threshold":1,"keys":[{"key":"EOS6RfWmDnkuHe7BoPMSeirFgz7rC8fSSVYD5Qqe8QqvzBoVLEbMg","weight":1}],"accounts":[{"permission":{"actor":"eosusdeosusd","permission":"eosio.code"},"weight":1}],"waits":[]}' owner -p eosusdeosusd@owner
cleos set contract eosusdeosusd /eosio.cdt contract/eosusd.wasm contract/eosusd.abi -p eosusdeosusd@active

cleos push action eosusdeosusd create '[ "eosusdeosusd", "1000000000.0000 EOSUSD"]' -p eosusdeosusd@active
cleos push action eosusdeosusd setsupply '[ "eosusdeosusd", "1000.0000 EOSUSD"]' -p eosusdeosusd@active

cleos push action eosio.token transfer '{"from":"testuser","to":"eosusdeosusd","quantity":"6.0000 EOS","memo":"collateral"}' -p testuser@active
cleos push action eosio.token transfer '{"from":"testuser","to":"eosusdeosusd","quantity":"5.0000 EOS","memo":"insurance"}' -p testuser@active

cleos push action eosusdeosusd borrow '{"usern":"testuser","debt":"1.0000 EOSUSD"}' -p testuser@active
cleos push action eosusdeosusd assetout '{"usern":"testuser","assetout":"1.0000 EOS","memo":"collateral"}' -p testuser@active
cleos push action eosusdeosusd assetout '{"usern":"testuser","assetout":"2.0000 EOS","memo":"insurance"}' -p testuser@active
cleos push action eosusdeosusd transfer '{"from":"testuser","to":"eosusdeosusd","quantity":"0.5000 EOSUSD","memo":"payoff debt"}' -p testuser@active

cleos get currency balance eosio.token eosusdeosusd
cleos get currency balance eosusdeosusd testuser