FROM node:8

# Create app directory and epgpub directory
RUN mkdir /src
WORKDIR /src

# Install app dependencies
ADD package.json /src/package.json
RUN npm install
RUN npm install thrift
RUN apt-get update
RUN apt-get install thrift-compiler --yes

#Bundle app source
COPY . /src
COPY ./gen-nodejs /src/gen-nodejs

EXPOSE 4000

CMD npm start
#FROM apache/thrift
