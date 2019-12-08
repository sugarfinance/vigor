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
const delay = ms => new Promise(res => setTimeout(res, ms));

describe(`Contract ${vigorCode}`, () => {
    let vigContract, vigorContract;
    const vig = 'vig'
    const vigor = 'vigor'
    before(done => {
      (async () => {
        try {
          // deploy vig & vigor contract
          await deployer.deploy(vigorCtrt, vigor);
          await deployer.deploy(vigCtrt, vig);
          vigContract = await getTestContract(vig);
          vigorContract = await getTestContract(vigor);
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
                maximum_supply: "1000000000.0000 VIG"
            }, {
                authorization: `${vig}@active`,
                broadcast: true,
                keyProvider: [vigAccountKeys.active.privateKey],
                sign: true
            });
            // issue
            await vigContract.issue({
                to: vig,
                quantity: "1000000000.0000 VIG",
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
            done();
        }
        catch (e) {
          done(e);
        }
      })();
    });
  });