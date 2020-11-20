const http = require('http')
const express = require('express');
const thrift = require('thrift');
const port = process.env.SERVER_PORT || 8080;
const app = express();
const assert = require('assert');
const Service = require('./gen-nodejs/Service.js');

/*let server = thrift.createServer(Service, {
    ping: function (result) {
            console.log("Ping!"); 
            result(null, "Pong");
    }, 
    wait: function (ms, result) {
	    let endtime = new Date().getTime() + Number(ms);
            while (new Date().getTime() < endtime) {
            }
            result(null, "OK");
    }, 
});

server.listen(4000);
*/
app.all('/microbenchmark/func', function (req, res) {
    let ms = req.header('MS')
    let endtime = new Date().getTime() + Number(ms);
    while (new Date().getTime() < endtime) {
    }

    let connection = thrift.createConnection(`localhost`, 4000, {
        transport: thrift.TBufferedTransport,
        protocol: thrift.TbinaryProtocol });

    let client = thrift.createClient(Service, connection);

    client.wait(Number(ms), function (err, response) {
        res.send("OK");
    });

    connection.on('error', function (err) {
	console.log(err);
        res.sendStatus(500);
    });

})

app.listen(port, () => {
console.info(`Server started on port ${port}`);
});
