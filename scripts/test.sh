##############################################
#start the local testnet
#rm -rf ~/eosio-wallet/*.wallet
pkill nodeos
rm -rf ~/.local/share/eosio/nodeos/data
nodeos -e -p eosio --plugin eosio::chain_api_plugin --plugin eosio::history_api_plugin --contracts-console
##############################################

##############################################
# setup eosio system accounts for local testnet
#cleos wallet create -n testwallet --to-console
cleos wallet unlock -n testwallet --password PW5KcXcFzdU9fRrRskrT7YtuTwVmv3XM4FQXuEK2vGG7m3vTbGagK
#
#cleos wallet import -n testwallet --private-key 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
cd ~/contracts1.6.0/eosio.contracts/contracts/eosio.bios
#eosio-cpp -I include -o eosio.bios.wasm src/eosio.bios.cpp --abigen
cleos set contract eosio ~/contracts1.6.0/eosio.contracts/contracts/eosio.bios -p eosio@active
cleos create account eosio eosio.token EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cd ~/contracts1.6.0/eosio.contracts/contracts/eosio.token
#eosio-cpp -I include -o eosio.token.wasm src/eosio.token.cpp --abigen
cleos set contract eosio.token ~/contracts1.6.0/eosio.contracts/contracts/eosio.token --abi eosio.token.abi -p eosio.token@active
cleos push action eosio.token create '[ "eosio", "1000000000.0000 EOS"]' -p eosio.token@active
##############################################

##############################################
# create the vigor1111111 account, set the contract, create UVD stablecoins
#cleos create key --to-console
#cleos wallet import -n testwallet --private-key 5KNgPef5eY7EBCNHbbm2egiTXfACxSPW3YSiktMkMFVmpqJbRGT
cleos create account eosio vigor1111111 EOS6gn7ZtKFNRS2WS6oAgcf5QWfmU1CwQEnof6FrBRCTPMBbSMKCY EOS6gn7ZtKFNRS2WS6oAgcf5QWfmU1CwQEnof6FrBRCTPMBbSMKCY
cleos set account permission vigor1111111 active '{"threshold":1,"keys":[{"key":"EOS6gn7ZtKFNRS2WS6oAgcf5QWfmU1CwQEnof6FrBRCTPMBbSMKCY","weight":1}],"accounts":[{"permission":{"actor":"vigor1111111","permission":"eosio.code"},"weight":1}],"waits":[]}' owner -p vigor1111111@owner

cd ~/contracts1.6.0/vigor
eosio-cpp -I . -abigen vigor.cpp -o vigor.wasm
cleos set contract vigor1111111 ~/contracts1.6.0/vigor vigor.wasm vigor.abi -p vigor1111111@active
cleos push action vigor1111111 create '[ "vigor1111111", "1000000000.0000 UVD"]' -p vigor1111111@active
cleos push action vigor1111111 setsupply '[ "vigor1111111", "1000000000.0000 UVD"]' -p vigor1111111@active
##############################################

##############################################
# create a testuser1111
#cleos wallet create -n testwallet2 --to-console
cleos wallet unlock -n testwallet2 --password PW5HuDfHm4wznVknC6Bewug4VtJNfsFsxRH2yendtdvDZ9Lt9F5nR
#cleos create key --to-console
#cleos wallet import -n testwallet2 --private-key 5J7UbUYpJfywWRtvq94b3RBzQvKYfRcUA3NSi1ApRsh8HuKfCy3
cleos create account eosio testuser1111 EOS6RYBoZJzGFR35mdQMyb7SBUuZQuupM2ZuqGnbtFJ4wLW3fXbb8 EOS6RYBoZJzGFR35mdQMyb7SBUuZQuupM2ZuqGnbtFJ4wLW3fXbb8
#cleos set account permission testuser1111 active '{"threshold":1,"keys":[{"key":"EOS6K42yrrMETmx2rXFJeKtaGrQAwgCDBYUVY7PGCVfhWFykqvhVR","weight":1}],"accounts":[{"permission":{"actor":"vigor1111111","permission":"eosio.code"},"weight":1}],"waits":[]}' owner -p testuser1111@active
cleos push action eosio.token issue '[ "testuser1111", "100000.0000 EOS", "m" ]' -p eosio@active
cleos get currency balance eosio.token testuser1111
##############################################

##############################################
# oracle feeders for running on local testnet
cleos create account eosio oracle111111 EOS6RYBoZJzGFR35mdQMyb7SBUuZQuupM2ZuqGnbtFJ4wLW3fXbb8 EOS6RYBoZJzGFR35mdQMyb7SBUuZQuupM2ZuqGnbtFJ4wLW3fXbb8
cleos create account eosio eostitanprod EOS6RYBoZJzGFR35mdQMyb7SBUuZQuupM2ZuqGnbtFJ4wLW3fXbb8 EOS6RYBoZJzGFR35mdQMyb7SBUuZQuupM2ZuqGnbtFJ4wLW3fXbb8
cleos create account eosio feeder111111 EOS6RYBoZJzGFR35mdQMyb7SBUuZQuupM2ZuqGnbtFJ4wLW3fXbb8 EOS6RYBoZJzGFR35mdQMyb7SBUuZQuupM2ZuqGnbtFJ4wLW3fXbb8
cleos create account eosio feeder211111 EOS6RYBoZJzGFR35mdQMyb7SBUuZQuupM2ZuqGnbtFJ4wLW3fXbb8 EOS6RYBoZJzGFR35mdQMyb7SBUuZQuupM2ZuqGnbtFJ4wLW3fXbb8

cd ~/contracts1.6.0/delphioracle/contract
cleos -u http://api.eosnewyork.io:80 get code delphioracle --wasm -c delphioracle.wasm
cleos -u http://api.eosnewyork.io:80 get code delphioracle --abi delphioracle.abi
cleos set contract oracle111111 ~/contracts1.6.0/delphioracle/contract delphioracle.wasm delphioracle.abi -p oracle111111@active

cleos push action oracle111111 setoracles '{"oracleslist":["feeder111111","feeder211111"]}' -p eostitanprod@active
#cleos push action oracle111111 write '{"owner":"feeder111111", "value":63800}' -p feeder111111@active
#cleos push action oracle111111 write '{"owner":"feeder211111", "value":64800}' -p feeder211111@active

# launch two oracle feeders
#in a new shell
cd ~/contracts1.6.0/delphioracle/scripts
nodeosurl='http://127.0.0.1:8888' interval=15000 account="oracle111111" defaultPrivateKey="5J7UbUYpJfywWRtvq94b3RBzQvKYfRcUA3NSi1ApRsh8HuKfCy3" feeder="feeder111111" node updater2.js
#in another shell
cd ~/contracts1.6.0/delphioracle/scripts
nodeosurl='http://127.0.0.1:8888' interval=15000 account="oracle111111" defaultPrivateKey="5J7UbUYpJfywWRtvq94b3RBzQvKYfRcUA3NSi1ApRsh8HuKfCy3" feeder="feeder211111" node updater2.js
#cleos get table oracle111111 oracle111111 eosusd --limit 1
#cleos get table oracle111111 oracle111111 oracles
#cleos get table oracle111111 oracle111111 eosusdstats
##############################################

##############################################
# exposed actions for vigor demo starts here

#cleos push action vigor1111111 deleteuser '{"name":testuser1111}' -p testuser1111@active
cleos push action eosio.token transfer '{"from":"testuser1111","to":"vigor1111111","quantity":"6.0000 EOS","memo":"collateral"}' -p testuser1111@active
cleos push action eosio.token transfer '{"from":"testuser1111","to":"vigor1111111","quantity":"5.0000 EOS","memo":"insurance"}' -p testuser1111@active

cleos push action vigor1111111 borrow '{"usern":"testuser1111","debt":"24.0000 UVD"}' -p testuser1111@active
cleos push action vigor1111111 assetout '{"usern":"testuser1111","assetout":"1.0000 EOS","memo":"collateral"}' -p testuser1111@active
cleos push action vigor1111111 assetout '{"usern":"testuser1111","assetout":"2.0000 EOS","memo":"insurance"}' -p testuser1111@active
cleos push action vigor1111111 transfer '{"from":"testuser1111","to":"vigor1111111","quantity":"5.0000 UVD","memo":"payoff debt"}' -p testuser1111@active

# run the doupdate action to update prices from the oracle and monitor collateral levels on all loans, and pay VIG premiums from borrower to lender
# custodians may run this action from a script, or instead delphioracle may begin calling this action from the oracle itself when BP's push prices, calling require_receiver(vigor1111111)
cleos push action vigor1111111 doupdate '{}' -p vigor1111111@active

cleos get table vigor1111111 UVD stat
cleos get table vigor1111111 vigor1111111 user
cleos get table eosio.token vigor1111111 accounts
cleos get table eosio.token testuser1111 accounts
cleos get table vigor1111111 testuser1111 accounts
#cleos get currency balance vigor1111111 testuser1111
#cleos get currency balance eosio.token vigor1111111
##############################################


