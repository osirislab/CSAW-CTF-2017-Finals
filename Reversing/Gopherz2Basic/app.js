'use strict';
var fs = require('fs');
var cardnames = fs.readFileSync('./cardnames.txt').toString().split('\n');
const FLAG = fs.readFileSync('/flag.txt').toString();

var pherz = require('./nodepherz');

var app = new pherz();

// app.hostname = process.env.HOSTNAME || '0.0.0.0';
// app.port = process.env.PORT || 3000;

app.hostname = 'web.chal.csaw.io'
app.port = 433


app.selector('/', function(req, res, done) {
  console.log(req.cookies);
  res.listing([
    {
      type:'T',
      data:'I hope your client likes recursion',
    },
    {
      type: 'P',
      selector: '/first',
    },
  ]);
  done();
});

function generateSelector() {
  return '/' + cardnames[Math.floor(Math.random() * cardnames.length)];
}
function generatePage(curr, next, cookie) {
  app.selector(curr, function(req, res, done) {
    res.listing([
      {
        type: 'T',
        data: 'You\'re almost there! Keep going!',
      },
      {
        type: 'C',
        data: 'Key:' + cookie,
      },
      {
        type: 'P',
        selector: next,
      }
    ]);
    done();
  });
}

var pages = ['/first'];
var cookies = [];
for (var i = 0; i < 100; ++i) {
  var next = generateSelector();
  while(pages.includes(next)) {
    next = generateSelector();
  }
  pages.push(next);
  var cookie = Math.floor(Math.random() * 255);
  cookies.push(cookie);
  generatePage(pages[i], pages[i + 1], cookie);
}

var encodedFlag = function(cookies) {
  var encoded = Buffer.from("Go to /imnotabusive for your flag!");
  for (var i = 0; i < cookies.length; ++i) {
    encoded[i % encoded.length] = cookies[i] ^ encoded[i % encoded.length];
  }
  encoded = Buffer.from(encoded).toString('base64');
  console.log(encoded);
  return encoded
}

app.selector(pages[100], (req, res, done) => {
  res.listing([
    {
      type: 'T',
      data:'Nice! Heres your flag:',
    },
    {
      type: 'T',
      data: encodedFlag(cookies),
    },
    {
      type: 'T',
      data: `var encodedFlag = function(cookies) {
        var encoded = Buffer.from(process.env.FLAG);
        for (var i = 0; i < cookies.length; ++i) {
          encoded[i % encoded.length] = cookies[i] ^ encoded[i % encoded.length];
        }
        encoded = Buffer.from(encoded).toString('base64');
        console.log(encoded);
        return encoded
      }`,
    }
  ]);
  done();
});

app.selector('/imnotabusive', (req, res, done) => {
  res.listing([
    {
      type: 'T',
      data: FLAG,
    },
    {
      type: 'T',
      data: 'OK now that you\'ve done this one, go to this link for another one',
    },
    {
      type: 'L',
      selector: '/',
      hostname: 'web.chal.csaw.io',
      port: '4333',
    }
  ]);
  done();
});

app.listen();