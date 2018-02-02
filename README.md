Argentum integration/staging tree
=====================================

[![Build Status](https://travis-ci.org/argentumproject/argentum.svg?branch=master)](https://travis-ci.org/argentumproject/argentum)

http://www.argentum.io

What is Argentum?
----------------

Argentum is an experimental digital currency that enables instant payments to
anyone, anywhere in the world. Argentum uses peer-to-peer technology to operate
with no central authority: managing transactions and issuing money are carried
out collectively by the network. Argentum is the name of open source
software which enables the use of this currency.

For more information, as well as an immediately usable, binary version of
the Argentum software, see http://www.argentum.io, or read the
[original Bitcoin whitepaper](https://bitcoincore.org/bitcoin.pdf).

[Website](http://www.argentum.io)

License
-------

Argentum is a fork of Bitcoin Core and inherits the terms of the MIT license. See 
[COPYING](COPYING) for more information or see https://opensource.org/licenses/MIT.

Development Process
-------------------

The `master` branch is occasionally built and tested, but is not guaranteed to be
completely stable. [Tags](https://github.com/argentumproject/argentum/tags) are created
regularly to indicate new official, stable release versions of Argentum.

The contribution workflow is described in [CONTRIBUTING.md](CONTRIBUTING.md).

Developer IRC can be found on Freenode at ##argentum.


Building
--------
See files in the [doc](doc) directory for generic build instructions for Windows,
OSX and Unix.

## Argentum Version 4.14.2

- These are just a few of the hundreds of enhancements with this new release

- BIP112 (CheckSequenceVerify) soft fork
- BIP146 Argentum will hard fork at block 2977000 (around March 13th 2018) to implement BIP146
- Signature validation using libsecp256k1
- Direct headers announcement (BIP 130)
- Automatically use Tor hidden services
- Notifications through ZMQ
- BIP9 softfork deployment
- Linux ARM builds
- Compact Block support (BIP 152)
- Hierarchical Deterministic Key Generation (HD wallets)
- Substantial improvments to the client load time. 


## Argentum Version 3.11.2
- Update the code base to Bitcoin version 11.2
- Hard fork for coinbase maturity at Block 2,422,000. New coinbase maturity will be 100 confirmations
- Up to date Gitian build scripts and setup guide
- BIP65 Soft Fork when ~>78% of nodes have upgraded
- Minimum protocol version is 1050000

## General
- Scrypt & SHA256D
- AUXPoW (Merged Mining)

## Blocks
- Max Block size 10mb
- 45 Second block time
- 3 Argentums per block

## Currency Creation
- 64 million Argentums will be created

## Security
- Mined blocks mature after 100 confirmations

## Mining Settings
Use this to set the algorithm to SHA256D for mining (add to argentum.conf)  

algo=sha256d

## Bootstrap
https://electrum-arg.org/bootstrap.dat  
sha256 checksum 37020ed0abe02d7bf97a9dde7de0419d278f88866ff9d67419492d2be0dd32db

https://electrum-arg.org/bootstrap.zip  

November 1, 2017
Block 2,723,638