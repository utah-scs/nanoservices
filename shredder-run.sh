#!/bin/bash
cd /mydata/nanoservices
./shredder -c 1 &
sleep 3
curl --get http://10.0.1.1:11211/user/init_db

