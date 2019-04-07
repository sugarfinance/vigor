#rm -rf ~/eosio-wallet/*.wallet
pkill nodeos
rm -rf ~/.local/share/eosio/nodeos/data
nodeos -e -p eosio --plugin eosio::chain_api_plugin --plugin eosio::history_api_plugin --contracts-console

#cleos wallet create -n testwallet --to-console
cleos wallet unlock -n testwallet --password PW5Jy9Dd7Hvkbc4ewvHeHk13wMumizbq7oXB1iw8ZXHf4rnZfKch5
#
#cleos wallet import -n testwallet --private-key 5Ht7C4j77gDQ4q1wjnsNqEBRrrknjwQerF5QYYnTFtBNEGwRsPu
#cleos set contract eosio ~/eos/build/contracts/eosio.bios -p eosio@active
cleos create account eosio eosio.token EOS8dnvEbWACTkfmcCfvdawv3W2SXAFRMgdQcVqxtTKdrRL113oFY EOS8dnvEbWACTkfmcCfvdawv3W2SXAFRMgdQcVqxtTKdrRL113oFY
#cd ~/contracts/eosio.contracts/eosio.token
#eosio-cpp -I include -o eosio.token.wasm src/eosio.token.cpp --abigen
cleos set contract eosio.token /home/ab/contracts/eosio.contracts/eosio.token --abi eosio.token.abi -p eosio.token@active
#
#cleos create key --to-console
#cleos wallet import -n testwallet --private-key 5K9sQVS3KAXe9ecBjs7PEmLXtE5mqvAVKRaytG3DN8aagWmMj4W
cleos create account eosio eosusdeosusd EOS6RfWmDnkuHe7BoPMSeirFgz7rC8fSSVYD5Qqe8QqvzBoVLEbMg EOS6RfWmDnkuHe7BoPMSeirFgz7rC8fSSVYD5Qqe8QqvzBoVLEbMg
cleos set account permission eosusdeosusd active '{"threshold":1,"keys":[{"key":"EOS6RfWmDnkuHe7BoPMSeirFgz7rC8fSSVYD5Qqe8QqvzBoVLEbMg","weight":1}],"accounts":[{"permission":{"actor":"eosusdeosusd","permission":"eosio.code"},"weight":1}],"waits":[]}' owner -p eosusdeosusd@owner

#cleos push action eosio.token create '[ "eosio", "1000000000.0000 EOSUSD"]' -p eosio.token@active
cleos push action eosio.token create '[ "eosio", "1000000000.0000 SYS"]' -p eosio.token@active
cleos push action eosio.token create '[ "eosio", "1000000000.0000 EOS"]' -p eosio.token@active

#cleos wallet create -n testwallet2 --to-console
cleos wallet unlock -n testwallet2 --password PW5JAui1zKRG1kV9VhrAP5KWDXzL7U7xyjSLCtjZxaLJNgggnDgud
#cleos create key --to-console
#cleos wallet import -n testwallet2 --private-key 5J7gFHsbnU7EpQ4RGrdhAoMnvXnJvWgfiRtvTdMCcVfT6DGkNrK
cleos create account eosio testuser EOS6K42yrrMETmx2rXFJeKtaGrQAwgCDBYUVY7PGCVfhWFykqvhVR EOS6K42yrrMETmx2rXFJeKtaGrQAwgCDBYUVY7PGCVfhWFykqvhVR
#cleos set account permission testuser active '{"threshold":1,"keys":[{"key":"EOS6K42yrrMETmx2rXFJeKtaGrQAwgCDBYUVY7PGCVfhWFykqvhVR","weight":1}],"accounts":[{"permission":{"actor":"eosusdeosusd","permission":"eosio.code"},"weight":1}],"waits":[]}' owner -p testuser@active
cleos push action eosio.token issue '[ "testuser", "100000.0000 SYS", "m" ]' -p eosio@active
cleos push action eosio.token issue '[ "testuser", "100000.0000 EOS", "m" ]' -p eosio@active
cleos get currency balance eosio.token testuser

cd ~/contracts/eosusd
eosio-cpp -abigen eosusd.cpp -o eosusd.wasm
cleos set contract eosusdeosusd ~/contracts/eosusd -p eosusdeosusd@active
cleos push action eosusdeosusd create '[ "eosusdeosusd", "1000000000.0000 EOSUSD"]' -p eosusdeosusd@active
cleos push action eosusdeosusd setsupply '[ "eosusdeosusd", "1000.0000 EOSUSD"]' -p eosusdeosusd@active

#cleos push action eosusdeosusd deleteuser '{"name":testuser}' -p testuser@active
cleos push action eosio.token transfer '{"from":"testuser","to":"eosusdeosusd","quantity":"6.0000 EOS","memo":"collateral"}' -p testuser@active
cleos push action eosio.token transfer '{"from":"testuser","to":"eosusdeosusd","quantity":"5.0000 EOS","memo":"insurance"}' -p testuser@active

cleos push action eosusdeosusd borrow '{"usern":"testuser","debt":"1.0000 EOSUSD"}' -p testuser@active
cleos push action eosusdeosusd assetout '{"usern":"testuser","assetout":"1.0000 EOS","memo":"collateral"}' -p testuser@active
cleos push action eosusdeosusd assetout '{"usern":"testuser","assetout":"2.0000 EOS","memo":"insurance"}' -p testuser@active
cleos push action eosusdeosusd transfer '{"from":"testuser","to":"eosusdeosusd","quantity":"1.0000 EOSUSD","memo":"payoff debt"}' -p testuser@active

cleos get table eosusdeosusd EOSUSD stat
cleos get table eosusdeosusd eosusdeosusd user
cleos get table eosio.token eosusdeosusd accounts
cleos get table eosio.token testuser accounts
cleos get table eosusdeosusd testuser accounts
#cleos get currency balance eosusdeosusd testuser
#cleos get currency balance eosio.token eosusdeosusd
