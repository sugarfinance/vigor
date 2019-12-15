import 'mocha';
require('babel-core/register');
require('babel-polyfill');
const { assert } = require('chai'); // Using Assert style
const { getCreateKeys } = require('../extensions/helpers/key-utils');
const { getNetwork, getCreateAccount, getEos, getTestAccountName } = require('../extensions/tools/eos/utils');
const getDefaultArgs = require('../extensions/helpers/getDefaultArgs');
const { getLocalDSPEos, getTestContract } = require('../extensions/tools/eos/utils');

const artifacts = require('../extensions/tools/eos/artifacts');
const deployer = require('../extensions/tools/eos/deployer');
const { genAllocateDAPPTokens, readVRAMData } = require('../extensions/tools/eos/dapp-services');
const { loadModels } = require('../extensions/tools/models');

const fetch = require('node-fetch');
const eosjs2 = require('eosjs');
const { JsonRpc, Api, Serialize } = eosjs2;
const { getUrl } = require('../extensions/tools/eos/utils');
const url = getUrl(getDefaultArgs());
const rpc = new JsonRpc(url, { fetch });

// setup vigor & vig contracts
const vigorCode = 'vigor';
const vigorCtrt = artifacts.require(`./${vigorCode}/`);
const vigCode = 'eosio.token';
const vigCtrt = artifacts.require(`./${vigCode}/`);
const dummyCode = 'eosio.token';
const dummyCtrt = artifacts.require(`./${dummyCode}/`);
const oracleCode = 'oracle';
const oracleCtrt = artifacts.require(`./${oracleCode}/`);
const delay = ms => new Promise(res => setTimeout(res, ms));
// oracle urls
const urleos = "https://min-api.cryptocompare.com/data/price?fsym=EOS&tsyms=USD&e=coinbase";
const urliq = "https://api.newdex.io/v1/price?symbol=everipediaiq-iq-eos";
const urlvig = "https://api.newdex.io/v1/price?symbol=vig111111111-vig-eos";

describe(`Contract ${vigorCode}`, () => {
    let vigContract, vigorContract, dummyContract, oracleContract;
    const vig = 'vig'
    const vigor = 'vigor'
    const dummy = 'dummy'
    const oracle = 'oracle'
    before(done => {
      (async () => {
        try {
          // deploy vig & vigor contract
          await deployer.deploy(vigorCtrt, vigor);
          await deployer.deploy(vigCtrt, vig);
          await deployer.deploy(dummyCtrt, dummy);
          await deployer.deploy(oracleCtrt, oracle);
          vigContract = await getTestContract(vig);
          vigorContract = await getTestContract(vigor);
          dummyContract = await getTestContract(dummy);
          oracleContract = await getTestContract(oracle);
          done();
        }
        catch (e) {
          done(e);
        }
      })();
    });
  
    it('init test', done => {
      (async () => {
        try {
            // setup test accounts
            // vig
            let testbrw11111 = "testbrw11111"
            await getCreateAccount(testbrw11111)
            let testbrw11112 = "testbrw11112"
            await getCreateAccount(testbrw11112)
            let testins11111 = "testins11111"
            await getCreateAccount(testins11111)
            let testins11112 = "testins11112"
            await getCreateAccount(testins11112)
            let finalreserve = "finalreserve"
            await getCreateAccount(finalreserve)
            let reinvestment = "reinvestment"
            await getCreateAccount(reinvestment)
            let testbrw21111 = "testbrw21111"
            await getCreateAccount(testbrw21111)
            let testbrw21112 = "testbrw21112"
            await getCreateAccount(testbrw21112)
            let testins21111 = "testins21111"
            await getCreateAccount(testins21111)
            let testins21112 = "testins21112"
            await getCreateAccount(testins21112)
            // oracle
            let feeder111111 = "feeder111111"
            await getCreateAccount(feeder111111)
            let feeder111112 = "feeder111112"
            await getCreateAccount(feeder111112)
            let feeder111113 = "feeder111113"
            await getCreateAccount(feeder111113)
            let datapreproc2 = "datapreproc2"
            await getCreateAccount(datapreproc2)
            // vigor
            // get vigor keys
            var vigorAccountKeys = await getCreateAccount(vigor);
            // create
            await vigorContract.create({
                issuer: vigor,
                maximum_supply: "1000000000.0000 VIGOR"
            }, {
                authorization: `${vigor}@active`,
                broadcast: true,
                keyProvider: [vigorAccountKeys.active.privateKey],
                sign: true
            });
            // set supply
            await vigorContract.setsupply({
                issuer: vigor,
                maximum_supply: "1000000000.0000 VIGOR"
            }, {
                authorization: `${vigor}@active`,
                broadcast: true,
                keyProvider: [vigorAccountKeys.active.privateKey],
                sign: true
            });

            // vig
            // get vig keys
            var vigAccountKeys = await getCreateAccount(vig);
            // create
            await vigContract.create({
                issuer: vig,
                maximum_supply: "100000000000.0000 VIG"
            }, {
                authorization: `${vig}@active`,
                broadcast: true,
                keyProvider: [vigAccountKeys.active.privateKey],
                sign: true
            });
            // issue
            await vigContract.issue({
                to: vig,
                quantity: "100000000000.0000 VIG",
                memo: ""
            }, {
                authorization: `${vig}@active`,
                broadcast: true,
                keyProvider: [vigAccountKeys.active.privateKey],
                sign: true
            });
            // transfer
            await vigContract.transfer({
                from: vig,
                to: testbrw11111,
                quantity: "1000000.0000 VIG",
                memo: ""
            }, {
                authorization: `${vig}@active`,
                broadcast: true,
                keyProvider: [vigAccountKeys.active.privateKey],
                sign: true
            });
            await vigContract.transfer({
                from: vig,
                to: testbrw11112,
                quantity: "1000000.0000 VIG",
                memo: ""
            }, {
                authorization: `${vig}@active`,
                broadcast: true,
                keyProvider: [vigAccountKeys.active.privateKey],
                sign: true
            });
            await vigContract.transfer({
                from: vig,
                to: testins11111,
                quantity: "1000000.0000 VIG",
                memo: ""
            }, {
                authorization: `${vig}@active`,
                broadcast: true,
                keyProvider: [vigAccountKeys.active.privateKey],
                sign: true
            });
            await vigContract.transfer({
                from: vig,
                to: testins11112,
                quantity: "1000000.0000 VIG",
                memo: ""
            }, {
                authorization: `${vig}@active`,
                broadcast: true,
                keyProvider: [vigAccountKeys.active.privateKey],
                sign: true
            });
            await vigContract.transfer({
                from: vig,
                to: finalreserve,
                quantity: "1000000.0000 VIG",
                memo: ""
            }, {
                authorization: `${vig}@active`,
                broadcast: true,
                keyProvider: [vigAccountKeys.active.privateKey],
                sign: true
            });
            await vigContract.transfer({
                from: vig,
                to: reinvestment,
                quantity: "1000000.0000 VIG",
                memo: ""
            }, {
                authorization: `${vig}@active`,
                broadcast: true,
                keyProvider: [vigAccountKeys.active.privateKey],
                sign: true
            });
            await vigContract.transfer({
                from: vig,
                to: testbrw21111,
                quantity: "1000000.0000 VIG",
                memo: ""
            }, {
                authorization: `${vig}@active`,
                broadcast: true,
                keyProvider: [vigAccountKeys.active.privateKey],
                sign: true
            });
            await vigContract.transfer({
                from: vig,
                to: testbrw21112,
                quantity: "1000000.0000 VIG",
                memo: ""
            }, {
                authorization: `${vig}@active`,
                broadcast: true,
                keyProvider: [vigAccountKeys.active.privateKey],
                sign: true
            });
            await vigContract.transfer({
                from: vig,
                to: testins21111,
                quantity: "1000000.0000 VIG",
                memo: ""
            }, {
                authorization: `${vig}@active`,
                broadcast: true,
                keyProvider: [vigAccountKeys.active.privateKey],
                sign: true
            });
            await vigContract.transfer({
                from: vig,
                to: testins21112,
                quantity: "1000000.0000 VIG",
                memo: ""
            }, {
                authorization: `${vig}@active`,
                broadcast: true,
                keyProvider: [vigAccountKeys.active.privateKey],
                sign: true
            });

            // setup dummycontract
            // get dummy keys
            var dummyAccountKeys = await getCreateAccount(dummy);
            // create
            await dummyContract.create({
                issuer: dummy,
                maximum_supply: "100000000000.0000 IQ"
            }, {
                authorization: `${dummy}@active`,
                broadcast: true,
                keyProvider: [dummyAccountKeys.active.privateKey],
                sign: true
            });
            // issue
            await dummyContract.issue({
                to: dummy,
                quantity: "100000000000.0000 IQ",
                memo: "m"
            }, {
                authorization: `${dummy}@active`,
                broadcast: true,
                keyProvider: [dummyAccountKeys.active.privateKey],
                sign: true
            });
            // transfer
            await dummyContract.transfer({
                from: dummy,
                to: testbrw11111,
                quantity: "1000000.0000 IQ",
                memo: "m"
            }, {
                authorization: `${dummy}@active`,
                broadcast: true,
                keyProvider: [dummyAccountKeys.active.privateKey],
                sign: true
            });
            await dummyContract.transfer({
                from: dummy,
                to: testbrw11112,
                quantity: "1000000.0000 IQ",
                memo: "m"
            }, {
                authorization: `${dummy}@active`,
                broadcast: true,
                keyProvider: [dummyAccountKeys.active.privateKey],
                sign: true
            });
            await dummyContract.transfer({
                from: dummy,
                to: testins11111,
                quantity: "1000000.0000 IQ",
                memo: "m"
            }, {
                authorization: `${dummy}@active`,
                broadcast: true,
                keyProvider: [dummyAccountKeys.active.privateKey],
                sign: true
            });
            await dummyContract.transfer({
                from: dummy,
                to: testins11112,
                quantity: "1000000.0000 IQ",
                memo: "m"
            }, {
                authorization: `${dummy}@active`,
                broadcast: true,
                keyProvider: [dummyAccountKeys.active.privateKey],
                sign: true
            });
            await dummyContract.transfer({
                from: dummy,
                to: finalreserve,
                quantity: "1000000.0000 IQ",
                memo: "m"
            }, {
                authorization: `${dummy}@active`,
                broadcast: true,
                keyProvider: [dummyAccountKeys.active.privateKey],
                sign: true
            });
            await dummyContract.transfer({
                from: dummy,
                to: reinvestment,
                quantity: "1000000.0000 IQ",
                memo: "m"
            }, {
                authorization: `${dummy}@active`,
                broadcast: true,
                keyProvider: [dummyAccountKeys.active.privateKey],
                sign: true
            });
            await dummyContract.transfer({
                from: dummy,
                to: testbrw21111,
                quantity: "1000000.0000 IQ",
                memo: "m"
            }, {
                authorization: `${dummy}@active`,
                broadcast: true,
                keyProvider: [dummyAccountKeys.active.privateKey],
                sign: true
            });
            await dummyContract.transfer({
                from: dummy,
                to: testbrw21112,
                quantity: "1000000.0000 IQ",
                memo: "m"
            }, {
                authorization: `${dummy}@active`,
                broadcast: true,
                keyProvider: [dummyAccountKeys.active.privateKey],
                sign: true
            });
            await dummyContract.transfer({
                from: dummy,
                to: testins21111,
                quantity: "1000000.0000 IQ",
                memo: "m"
            }, {
                authorization: `${dummy}@active`,
                broadcast: true,
                keyProvider: [dummyAccountKeys.active.privateKey],
                sign: true
            });
            await dummyContract.transfer({
                from: dummy,
                to: testins21112,
                quantity: "1000000.0000 IQ",
                memo: "m"
            }, {
                authorization: `${dummy}@active`,
                broadcast: true,
                keyProvider: [dummyAccountKeys.active.privateKey],
                sign: true
            });

            // setup oracle
            // get oracle keys
            var oracleAccountKeys = await getCreateAccount(oracle);
            var feeder111111AccountKeys = await getCreateAccount(feeder111111);
            var feeder111112AccountKeys = await getCreateAccount(feeder111112);
            var feeder111113AccountKeys = await getCreateAccount(feeder111113);
            await oracleContract.configure({}, 
            {
                authorization: `${oracle}@active`,
                broadcast: true,
                keyProvider: [oracleAccountKeys.active.privateKey],
                sign: true
            });
            let oracleFetch = await fetch(urleos);
            oracleFetch = await oracleFetch.json();
            await oracleContract.write({
                owner: feeder111111,
                quotes:[{value: parseInt(Math.round(oracleFetch.USD * 1000000)), pair: "eosusd"}]
            }, {
                authorization: `${feeder111111}@active`,
                broadcast: true,
                keyProvider: [feeder111111AccountKeys.active.privateKey],
                sign: true
            });
            await oracleContract.write({
                owner: feeder111112,
                quotes:[{value: parseInt(Math.round(oracleFetch.USD * 1000000)), pair: "eosusd"}]
            }, {
                authorization: `${feeder111112}@active`,
                broadcast: true,
                keyProvider: [feeder111112AccountKeys.active.privateKey],
                sign: true
            });
            await oracleContract.write({
                owner: feeder111113,
                quotes:[{value: parseInt(Math.round(oracleFetch.USD * 1000000)), pair: "eosusd"}]
            }, {
                authorization: `${feeder111113}@active`,
                broadcast: true,
                keyProvider: [feeder111113AccountKeys.active.privateKey],
                sign: true
            });
            oracleFetch = await fetch(urliq);
            oracleFetch = await oracleFetch.json();
            await oracleContract.write({
                owner: feeder111111,
                quotes:[{value: parseInt(Math.round(oracleFetch.data.price * 1000000)), pair: "iqeos"}]
            }, {
                authorization: `${feeder111111}@active`,
                broadcast: true,
                keyProvider: [feeder111111AccountKeys.active.privateKey],
                sign: true
            });
            await oracleContract.write({
                owner: feeder111112,
                quotes:[{value: parseInt(Math.round(oracleFetch.data.price * 1000000)), pair: "iqeos"}]
            }, {
                authorization: `${feeder111112}@active`,
                broadcast: true,
                keyProvider: [feeder111112AccountKeys.active.privateKey],
                sign: true
            });
            await oracleContract.write({
                owner: feeder111113,
                quotes:[{value: parseInt(Math.round(oracleFetch.data.price * 1000000)), pair: "iqeos"}]
            }, {
                authorization: `${feeder111113}@active`,
                broadcast: true,
                keyProvider: [feeder111113AccountKeys.active.privateKey],
                sign: true
            });
            oracleFetch = await fetch(urlvig);
            oracleFetch = await oracleFetch.json();
            await oracleContract.write({
                owner: feeder111111,
                quotes:[{value: parseInt(Math.round(oracleFetch.data.price * 1000000)), pair: "vigeos"}]
            }, {
                authorization: `${feeder111111}@active`,
                broadcast: true,
                keyProvider: [feeder111111AccountKeys.active.privateKey],
                sign: true
            });
            await oracleContract.write({
                owner: feeder111112,
                quotes:[{value: parseInt(Math.round(oracleFetch.data.price * 1000000)), pair: "vigeos"}]
            }, {
                authorization: `${feeder111112}@active`,
                broadcast: true,
                keyProvider: [feeder111112AccountKeys.active.privateKey],
                sign: true
            });
            await oracleContract.write({
                owner: feeder111113,
                quotes:[{value: parseInt(Math.round(oracleFetch.data.price * 1000000)), pair: "vigeos"}]
            }, {
                authorization: `${feeder111113}@active`,
                broadcast: true,
                keyProvider: [feeder111113AccountKeys.active.privateKey],
                sign: true
            });
            let datapoints = await rpc.get_table_rows({
                json: true,
                code: oracle,
                scope: 'eosusd',
                table: 'datapoints'
            });
            console.log(datapoints);
            datapoints = await rpc.get_table_rows({
                json: true,
                code: oracle,
                scope: 'eosusd',
                table: 'datapoints',
                limit: 1
            });
            console.log(datapoints);
            datapoints = await rpc.get_table_rows({
                json: true,
                code: oracle,
                scope: 'iqeos',
                table: 'datapoints',
                limit: 1
            });
            console.log(datapoints)
            datapoints = await rpc.get_table_rows({
                json: true,
                code: oracle,
                scope: 'vigeos',
                table: 'datapoints',
                limit: 1
            });
            console.log(datapoints)
            let stats = await rpc.get_table_rows({
                json: true,
                code: oracle,
                scope: oracle,
                table: 'stats'
            });
            console.log(stats);
            let pairs = await rpc.get_table_rows({
                json: true,
                code: oracle,
                scope: oracle,
                table: 'pairs'
            });
            console.log(pairs);
            done();
        }
        catch (e) {
          done(e);
        }
      })();
    });
  });