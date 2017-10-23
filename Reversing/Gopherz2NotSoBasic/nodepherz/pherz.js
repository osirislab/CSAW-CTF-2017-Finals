'use strict';

const GOPHERZ_PORT = 3000;
const LINE_SEP = '\r\n';

var events = require('events');
var fs = require('fs');
var path = require('path');
var net = require('net');
var { waterfall } = require('async');

var utils = require('./utils');

function ResponseFactory(server){

  function Response(client, headers) {
    this.type = 'P';
    this.send = () => {
      let data = Buffer.from(this.data || 'E\t404');
      let message = utils.encodeMessage(client.s, data);
      let header = utils.encodeMessage(client.s, utils.generateHeader(this.type, message));
      try {
        client.write(header);
        client.write(message);
      } catch (e){}
      client.end();
    };

    this.render_page = (template_name) => {
    };

    this.listing = (items) => {
      this.data = utils.generateListing(server, items, headers);
    };

    this.media = (type, filename, cb) => {
      this.type = type;
      utils.getFileBuffer(server, filename, (err, data) => {
        if (err) this.data = Buffer.from('E\tCould not get file');
        else this.data = data;
        cb();
      });
    }

    return this;
  }

  return Response;
}

function RequestFactory(server) {

  function Request(client, selector, headers) {
    this.params = headers.raw;
    this.selector = selector;
    this.client = client;
    this.server = server;

    return this;
  }

  return Request;
}

function Router(Request, Response) {
  events.EventEmitter.call(this);

  this.route = (client, selector, headers) => {
    this.emit('route', selector, new Request(client, selector, headers), new Response(client, headers));
  }

}

Router.prototype.__proto__ = events.EventEmitter.prototype;

function Pherz() {
  this.router = new Router(RequestFactory(this), ResponseFactory(this));
  this.router.on('route', (selector, req, res) => {
    console.log(selector, 'requested by', req.client.remoteAddress);
    waterfall(this.preprocessors);
    let postprocessing = () => {
      waterfall([
        (cb) => cb(req, res),
        ...this.postprocessors,
      ],  () => res.send());
    }

    if (this.selectors[selector]) return this.selectors[selector](req, res, postprocessing);
    else if (this.medias[selector]) return this.medias[selector](req, res, postprocessing);
    else {
      postprocessing(req, res);
    }
  });
  this.selectors = {};
  this.selector = (selector, cb) => {
    this.selectors[selector] = cb;
  };

  this.preprocessors = [];
  this.pre = (cb) => { this.preprocessors.push(cb); };

  this.postprocessors = [];
  this.post = (cb) => { this.postprocessors.push(cb); };

  this.media = (selector, type, filename) => {
    this.selectors[selector] = (req, res) => {
      res.media(type, filename, res.send);
    };
  };

  this.server = net.createServer();
  this.server.on('connection', (client) => {
    console.log('Connection from', client.remoteAddress);
    client.on('error', console.error);
    utils.diffieHellman(client, (data) => {
      let [selector, headers] = utils.parseRequest(data);
      this.router.route(client, selector, headers);
    });
  });

  this.listen = (hostname, port) => {
    hostname = hostname || this.hostname;
    port = port || this.port;
    this.server.on('error', console.error);
    this.server.listen(port, '0.0.0.0', () => {
      console.log("Server listening on", '0.0.0.0', port);
    });
  }
};

module.exports = Pherz;