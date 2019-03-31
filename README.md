# eosusd
Website: https://eosUSD.com  
Whitepaper: https://eosUSD.com/eosUSD.pdf

# Usage

Build your eos development image

```
docker build -t my/eos .
```

In a new terminal, start the required environment

```
docker-compose up # down or rm to clean up
```

Initialize the smart contract

```
sh bootstrap.sh
alias cleos="docker exec -it nodeos cleos --url http://127.0.0.1:8888 --wallet-url http://keosd:8901"
cleos get currency balance eosusdeosusd testuser
# 0.5000 EOSUSD


```

Congrats, here are your first Localnet eosUSD