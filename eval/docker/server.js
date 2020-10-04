const http = require('http')
const express = require('express');
const port = process.env.SERVER_PORT || 8080;
const app = express();

const options = {
  hostname: 'docker_service2_1',
  port: 4000,
  path: '/microbenchmark/func',
  method: 'POST',
  headers: {
    'Content-Type': 'application/json',
    'Content-Length': 0,
    'MS': 10
  }
}

app.all('/microbenchmark/func', function (req, res) {
    let ms = req.header('MS')
    let endtime = new Date().getTime() + Number(ms);
    while (new Date().getTime() < endtime) {
    }
    http.request(options, function(response) {
        res.send("OK")
      }).on('error', function(e) {
        res.sendStatus(500);
      }).end();
})

app.listen(port, () => {
console.info(`Server started on port ${port}`);
});
