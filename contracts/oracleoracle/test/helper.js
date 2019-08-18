const { Api, JsonRpc, RpcError } = require('eosjs');
const fetch = require('node-fetch');
const { TextEncoder, TextDecoder } = require('util');

const chainIds = {
  mainnet: 'aca376f206b8fc25a6ed44dbdc66547c36c6c33e3a119ffbeaef943642f0e906',
  jungle: 'e70aaab8997e1dfce58fbfac80cbbb8fecec7b99cf982a9444273cbc64c41473',
  kylin: '5fff1dae8dc8e2fc4d5b23b2c7665c97f9e9d8edf2b6485a86ba311c25639191',
  local: 'cf057bbfb72640471fd910bcb67639c22df9f92470936cddc1ade0e2f2e7dc4f'
}

const {
  EOSIO_NETWORK,
  EOSIO_API_ENDPOINT,
  EOSIO_CHAIN_ID,
} = process.env;

const chainId = EOSIO_API_ENDPOINT || chainIds[EOSIO_NETWORK] || chainIds.local;

const rpc = new JsonRpc(process.env.EOSIO_API_ENDPOINT || 'http://0.0.0.0:8888', { fetch });

const signatureProvider = {
  getAvailableKeys: () => [
    '5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3',
  ],
};

const eos = new Api({
  rpc,
  chainId,
  signatureProvider,
  textEncoder: TextEncoder,
  textDecoder: TextDecoder,
});

global.eos = eos;