var pherz = require('./nodepherz');

var app = new pherz();

app.hostname = '216.165.2.48';
app.port = process.env.PORT || 9998;

app.static_dir = './';

app.selector('/', (req, res, done) => {
  res.listing([
    {
      type: 'I',
      selector: '/client/py',
    },
  ]);
  done();
});

app.media('/client/py', 'I', 'payload.py');

app.listen();