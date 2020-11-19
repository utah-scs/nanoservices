const express = require('express');
const port = process.env.SERVER_PORT || 4000;
const app = express();

app.all('/microbenchmark/func', function (req, res) {
    let ms = req.header('MS')
    let endtime = new Date().getTime() + Number(ms);
    while (new Date().getTime() < endtime) {
    }
    res.send("OK")
})

app.listen(port, () => {
console.info(`Server started on port ${port}`);
});
