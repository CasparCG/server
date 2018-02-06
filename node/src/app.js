const express = require('express')
const pinoHttp = require('pino-http')
const util = require('util')
const recursiveReadDir = require('recursive-readdir')
const { getId } = require('./util')

const recursiveReadDirAsync = util.promisify(recursiveReadDir)

module.exports = function ({ db, config, logger }) {
  const app = express()

  app.use(pinoHttp({ logger }))

  app.get('/cls', async (req, res, next) => {
    try {
      const { rows } = await db.allDocs({ include_docs: true })

      let str = '200 CLS OK\r\n'
      str += rows
        .map(row => row.doc.cinf || '')
        .reduce((acc, cinf) => acc + cinf, '')
      str += '\r\n'
      res.send(str)
    } catch (err) {
      next(err)
    }
  })

  app.get('/tls', async (req, res, next) => {
    try {
      let str = '200 TLS OK\r\n'
      for (const templatePath of await recursiveReadDirAsync(config.paths.template)) {
        str += `${getId(config.paths.template, templatePath)}\r\n`
      }
      str += '\r\n'
      res.send(str)
    } catch (err) {
      next(err)
    }
  })

  app.get('/fls', async (req, res, next) => {
    try {
      let str = '200 FLS OK\r\n'
      for (const fontPath of await recursiveReadDirAsync(config.paths.fonts)) {
        str += `${getId(config.paths.fonts, fontPath)}\r\n`
      }
      str += '\r\n'
      res.send(str)
    } catch (err) {
      next(err)
    }
  })

  app.get('/cinf/:id', async (req, res, next) => {
    try {
      let str = '200 CINF OK\r\n'
      const { cinf } = await db.get(req.params.id.toUpperCase())
      str += cinf
      res.send(str)
    } catch (err) {
      next(err)
    }
  })

  app.get('/thumbnail/generate', async (req, res, next) => {
    try {
      // TODO Force generate?
      res.send('202 THUMBNAIL GENERATE_ALL OK\r\n')
    } catch (err) {
      next(err)
    }
  })

  app.get('/thumbnail/generate/:id', async (req, res, next) => {
    try {
      // TODO Wait for scanner?
      res.send('202 THUMBNAIL GENERATE OK\r\n')
    } catch (err) {
      next(err)
    }
  })

  app.get('/thumbnail', async (req, res, next) => {
    try {
      const { rows } = await db.allDocs({ include_docs: true })

      let str = '200 THUMBNAIL LIST OK\r\n'
      str += rows
        .map(row => row.doc.tinf || '')
        .reduce((acc, tinf) => acc + tinf, '')
      str += '\r\n'
      res.send(str)
    } catch (err) {
      next(err)
    }
  })

  app.get('/thumbnail/:id', async (req, res, next) => {
    try {
      const { _attachments } = await db.get(req.params.id.toUpperCase(), { attachments: true })

      let str = '201 THUMBNAIL RETRIEVE OK\r\n'
      str += _attachments['thumb.png'].data
      str += '\r\n'
      res.send(str)
    } catch (err) {
      next(err)
    }
  })

  app.use((err, req, res, next) => {
    req.log.error({ err })
    next(err)
  })

  return app
}
