'use strict';

const TOKEN_SEP = ':';
const REQUEST_SEP = '\t';
const LINE_SEP = '\r\n';
const p = 251;
const g = 6;

const BigNumber = require('big-number');
const fs = require('fs');
const path = require('path');

function generateHeader(type, message) {
    let header = Buffer.from(type);
    let size = new Buffer(4);
    size.writeUInt32BE(message.length);
    header = Buffer.concat([header, size, Buffer.from(LINE_SEP)]);
    return header;
}

function generateListing(server, items, headers) {
  let listing = items.map( ({type, ...data}) => {
    switch(type) {
      case 'T':
      case 'E':
      case 'C':
        return [type, data.data];
      case 'L':
      case 'P':
      case 'G':
      case 'I':
      case 'S':
      case 'F':
        return [
          type,
          data.selector,
          data.hostname || server.hostname,
          data.port || server.port,
          ...encodeParams(data.parameters),
        ];
    }
  });
  return listing.map((item) => item.join(REQUEST_SEP)).join(LINE_SEP)
}

function getFileBuffer(server, filename, cb) {
  fs.readFile(
    path.resolve(server.static_dir || './', filename),
    (err, data) => {
        if (err) return cb(err);
        else return cb(null, data);
    }
  );
}

function encodeCookies(cookies){
  return cookies.map(({key, value}) =>
    [key, value].join(TOKEN_SEP)
  ).join(REQUEST_SEP);
}

function encodeParams(params){
  return params && params.map(({key, value}) =>
    [key, value].join(TOKEN_SEP)
  ) || [];
}

function parseRequest(request){
  let [selector, ...params] = request.toString().split(REQUEST_SEP);
  let header = {
    raw:{},
  };
  for (var i in params) {
    let [key, value] = params[i].split(TOKEN_SEP);
    header.raw[key] = value;
  }

  return [selector, header];
};

function decodeMessage(secret, message){
  for (var i = 0; i < message.length; ++i) {
    message[i] = message[i] ^ secret;
  }
  return message;
};

function encodeMessage(secret, message) {
  return decodeMessage(secret, message);
}

function genb() {
  return Math.floor(Math.random() * 1024);
}

function diffieHellman(client, cb) {
  client.once('data', (A) => {
    A = A[0];
    let b = genb();
    let B = Buffer.alloc(1);
    B[0] = Number(BigNumber(g).pow(b).mod(p));
    try {
      client.write(B);
    } catch (e) {}
    client.s = Number(BigNumber(A).mod(p).pow(b).mod(p));
    client.on('data', (data) => {
      return cb(decodeMessage(client.s, data));
    });
  });
}

module.exports = {
  generateHeader: generateHeader,
  generateListing: generateListing,
  getFileBuffer: getFileBuffer,
  encodeCookies: encodeCookies,
  encodeParams: encodeParams,
  parseRequest: parseRequest,
  encodeMessage: encodeMessage,
  decodeMessage: decodeMessage,
  genb: genb,
  diffieHellman: diffieHellman,
};