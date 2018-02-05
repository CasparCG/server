const pino = require('pino')
const config = require('./config')
const PouchDB = require('pouchdb-node')
const scanner = require('./scanner')
const app = require('./app')

const logger = pino({
  ...config.logger,
  serializers: {
    err: pino.stdSerializers.err
  }
})
const db = new PouchDB('media')

scanner({ logger, db, config })
app({ logger, db, config }).listen(config.http.port)
