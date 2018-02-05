const express = require('express')
const pino = require('pino')
const pinoHttp = require('pino-http')
const util = require('util')
const fs = require('fs')
const path = require('path')
const cp = require('child_process')
const recursiveReadDir = require('recursive-readdir')
const { Observable } = require('@reactivex/rxjs')
const config = require('./config')
const chokidar = require('chokidar')
const mkdirp = require('mkdirp-promise')
const PouchDB = require('pouchdb-node')
const os = require('os')

const statAsync = util.promisify(fs.stat)
const unlinkAsync = util.promisify(fs.unlink)
const readFileAsync = util.promisify(fs.readFile)
const recursiveReadDirAsync = util.promisify(recursiveReadDir)

const logger = pino({
  ...config.logger,
  serializers: {
    err: pino.stdSerializers.err
  }
})
const db = new PouchDB('media')
const app = express()

app.use(pinoHttp({ logger }))

function getId (fileDir, filePath) {
  return path
    .relative(fileDir, filePath)
    .replace(/\.[^/.]+$/, '')
    .replace(/\\+/g, '/')
    .toUpperCase()
}

if (config.thumbnails.chokidar) {
  const watcher = chokidar
    .watch(config.paths.media, {
      alwaysStat: true,
      awaitWriteFinish: {
        stabilityThreshold: 2000,
        pollInterval: 100
      },
      ...config.thumbnails.chokidar
    })
    .on('error', err => logger.error({ err }))

  Observable
    .merge(
      Observable.fromEvent(watcher, 'add'),
      Observable.fromEvent(watcher, 'change'),
      Observable.fromEvent(watcher, 'unlink')
    )
    .concatMap(async ([ mediaPath, mediaStat ]) => {
      const mediaId = getId(mediaPath)
      try {
        await scanFile(mediaPath, mediaId, mediaStat)
      } catch (err) {
        if (err.code === 'ENOENT') {
          await db.delete(mediaId)
        } else {
          logger.error({ err })
        }
      }
    })
    .subscribe()
}

async function scanFile (mediaPath, mediaId, mediaStat) {
  const doc = await db
    .get(mediaId)
    .catch(() => ({
      _id: mediaId,
      mediaPath: mediaPath
    }))

  if (doc.mediaSize !== mediaStat.size || doc.mediaTime !== mediaStat.mtime.getTime()) {
    return
  }

  doc.mediaSize = mediaStat.size
  doc.mediaTime = mediaStat.mtime.getTime()

  await Promise.all([
    generateInfo(doc).catch(err => {
      logger.error({ err })
    }),
    generateThumb(doc).catch(err => {
      logger.error({ err })
    })
  ])

  await db.put(doc)
}

async function generateThumb (doc) {
  const tmpPath = path.join(os.tmpdir(), Math.random().toString(16))

  const args = [
    // TODO (perf) Low priority process?
    config.paths.ffmpeg,
    '-hide_banner',
    '-i', `"${doc.mediaPath}"`,
    '-frames:v 1',
    `-vf thumbnail,scale=${config.thumbnails.width}:${config.thumbnails.height}`,
    '-f png',
    '-threads 1',
    tmpPath
  ]

  await mkdirp(path.dirname(tmpPath))
  await new Promise((resolve, reject) => {
    cp.exec(args.join(' '), (err, stdout, stderr) => err ? reject(err) : resolve())
  })

  const thumbStat = await statAsync(tmpPath)
  doc.thumbSize = thumbStat.size
  doc.thumbTime = thumbStat.mtime.toISOString()
  doc.tinf = [
    `"${getId(config.paths.media, doc.mediaPath)}"`,
    // TODO (fix) Binary or base64 size?
    doc.thumbSize
  ].join(' ') + '\r\n'

  await db.putAttachment(doc._id, 'thumb.png', await readFileAsync(tmpPath), 'image/png')
  await unlinkAsync(tmpPath)
}

async function generateInfo (doc) {
  doc.cinf = await new Promise((resolve, reject) => {
    const args = [
      // TODO (perf) Low priority process?
      config.paths.ffprobe,
      '-hide_banner',
      '-i', `"${doc.mediaPath}"`,
      '-show_format',
      '-show_streams',
      '-print_format', 'json'
    ]
    cp.exec(args.join(' '), (err, stdout, stderr) => {
      if (err) {
        return reject(err)
      }

      const json = JSON.parse(stdout)
      if (!json.format || !json.streams || !json.streams[0]) {
        return reject(new Error('not media'))
      }

      const timeBase = json.streams[0].time_base
      const duration = json.streams[0].duration_ts

      let type = 'AUDIO'
      if (duration <= 1) {
        type = 'STILL'
      } else if (json.streams[0].pix_fmt) {
        type = 'MOVIE'
      }

      resolve([
        `"${getId(config.paths.media, doc.mediaPath)}"`,
        type,
        doc.mediaSize,
        duration,
        timeBase
      ].join(' '))
    })
  })
}

app.get('/cls', async (req, res, next) => {
  try {
    const { rows } = await db.allDocs({ include_docs: true })

    let str = '200 CLS OK\r\n'
    str += rows
      .map(row => row.doc.cinf || '')
      .reduce((acc, { cinf }) => acc + cinf, '')
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
    str += '\r\n\r\n'
    res.send(str)
  } catch (err) {
    next(err)
  }
})

app.get('/thumbnail/generate', async (req, res, next) => {
  try {
    res.send('202 THUMBNAIL GENERATE_ALL OK\r\n')
  } catch (err) {
    next(err)
  }
})

app.get('/thumbnail/generate/:id', async (req, res, next) => {
  try {
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
      .reduce((acc, str) => acc + str, '')
    str += '\r\n'
    res.send(str)
  } catch (err) {
    next(err)
  }
})

app.get('/thumbnail/:id', async (req, res, next) => {
  try {
    let str = '201 THUMBNAIL RETRIEVE OK\r\n'
    str += await db.getAttachment(req.params.id.toUpperCase(), 'thumb.png')
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

app.listen(config.http.port)
