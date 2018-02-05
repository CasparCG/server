const express = require('express')
const pino = require('pino')
const pinoHttp = require('pino-http')
const util = require('util')
const fs = require('fs')
const path = require('path')
const cp = require('child_process')
const recursiveReadDir = require('recursive-readdir')
const { Observable } = require('@reactivex/rxjs')
const base64 = require('base64-stream')
const config = require('./config')
const chokidar = require('chokidar')
const mkdirp = require('mkdirp-promise')

const statAsync = util.promisify(fs.stat)
const renameAsync = util.promisify(fs.rename)
const recursiveReadDirAsync = util.promisify(recursiveReadDir)

const logger = pino({
  ...config.logger,
  serializers: {
    err: pino.stdSerializers.err
  }
})

const app = express()

app.use(pinoHttp({ logger }))

if (config.thumbnails) {
  const watcher = chokidar
    .watch(config.paths.media, config.thumbnails.chokidar)
    .on('error', err => logger.error({ err }))

  Observable.merge(
    Observable.fromEvent(watcher, 'add'),
    Observable.fromEvent(watcher, 'change')
  )
  .concatMap(async filePath => {
    try {
      await generateThumbnail(filePath)
    } catch (err) {
      // TODO
    }
  })
  .subscribe()
}

async function generateThumbnail (filePath) {
  const stat = await statAsync(filePath)
  if (stat.isDirectory()) {
    return
  }

  const mediaPath = config.paths.media
  const thumbPath = `${path.join(config.paths.thumbnail, 'tmp', path.relative(mediaPath, filePath)).replace(/\.[^/.]+$/, '')}.png`
  const thumbPath2 = `${path.join(config.paths.thumbnail, path.relative(mediaPath, filePath)).replace(/\.[^/.]+$/, '')}.png`

  const args = [
    config.paths.ffmpeg,
    '-hide_banner',
    '-i', `"${filePath}"`,
    '-frames:v 1',
    `-vf thumbnail,scale=${config.thumbnails.width}:${config.thumbnails.height}`,
    '-threads 1',
    thumbPath,
    '-y'
  ]

  await mkdirp(path.dirname(thumbPath))
  await mkdirp(path.dirname(thumbPath2))
  await new Promise((resolve, reject) => {
    cp.exec(args.join(' '), (err, stdout, stderr) => err ? reject(err) : resolve())
  })
  await renameAsync(thumbPath, thumbPath2)
}

async function mediaInfo (fileDir, filePath) {
  const stat = await statAsync(filePath)
  if (stat.isDirectory()) {
    return
  }
  const { duration, timeBase, type } = await new Promise((resolve, reject) => {
    const args = [
      config.paths.ffprobe,
      '-hide_banner',
      '-i', `"${filePath}"`,
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
      if (json.format.duration < 1.0 / 25.0) {
        type = 'STILL'
      } else if (json.streams[0].pix_fmt) {
        type = 'MOVIE'
      }

      resolve({ type, duration, timeBase })
    })
  })
  return [
    `"${path.relative(fileDir, filePath).replace(/\.[^/.]+$/, '')}"`,
    type,
    stat.size,
    stat.mtime.getTime(),
    duration,
    timeBase
  ].join(' ')
}

async function findFile (fileDir, fileId) {
  let filePath
  for (let filePath2 of await recursiveReadDirAsync(fileDir)) {
    const fileId2 = path.relative(fileDir, filePath2).replace(/\.[^/.]+$/, '').toUpperCase()
    if (fileId2 === fileId) {
      filePath = filePath2
      break
    }
  }
  if (!filePath) {
    throw new Error(`File not found`)
  }
  return filePath
}

app.get('/cls', async (req, res, next) => {
  try {
    let str = '200 CLS OK\r\n'
    str += await Observable
      .from(await recursiveReadDirAsync(config.paths.media))
      .mergeMap(async filePath => {
        try {
          return `${await mediaInfo(config.paths.media, filePath)}\r\n`
        } catch (err) {
          // TODO
        }
      }, null, 8)
      .filter(x => x)
      .reduce((acc, str) => acc + str, '')
      .toPromise()
    str += '\r\n'
    res.send(str)
  } catch (err) {
    next(err)
  }
})

app.get('/tls', async (req, res, next) => {
  try {
    let str = '200 TLS OK\r\n'
    for (const filePath of await recursiveReadDirAsync(config.paths.template)) {
      if (filePath.endsWith('.ft')) {
        str += `${path.relative(config.paths.template, filePath).replace(/\.[^/.]+$/, '')}\r\n`
      }
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
    for (const filePath of await recursiveReadDirAsync(config.paths.fonts)) {
      str += `${path.relative(config.paths.fonts, filePath).replace(/\.[^/.]+$/, '')}\r\n`
    }
    str += '\r\n'
    res.send(str)
  } catch (err) {
    next(err)
  }
})

app.get('/cinf/:id', async (req, res, next) => {
  try {
    let fileId = req.params.id.toUpperCase()
    let filePath = await findFile(config.paths.media, fileId)

    let str = '200 CINF OK\r\n'
    str += await mediaInfo(config.paths.media, filePath)
    str += '\r\n\r\n'
    res.send(str)
  } catch (err) {
    next(err)
  }
})

app.get('/thumbnail/generate', async (req, res, next) => {
  try {
    const filePaths = await recursiveReadDirAsync(config.paths.media)
    await Observable
      .from(filePaths)
      .concatMap(async filePath => {
        try {
          await generateThumbnail(filePath)
        } catch (err) {
          // TODO
        }
      })
      .toPromise()
    res.send('202 THUMBNAIL GENERATE_ALL OK\r\n')
  } catch (err) {
    next(err)
  }
})

app.get('/thumbnail/generate/:id', async (req, res, next) => {
  try {
    const fileId = req.params.id.toUpperCase()
    const filePath = await findFile(config.paths.media, fileId)
    await generateThumbnail(filePath)
    res.send('202 THUMBNAIL GENERATE OK\r\n')
  } catch (err) {
    next(err)
  }
})

app.get('/thumbnail', (req, res) => {
  // TODO
  res.statusCode = 500
  res.end()
})

app.get('/thumbnail/:id', async (req, res, next) => {
  try {
    const fileId = req.params.id.toUpperCase()
    const filePath = await findFile(config.paths.thumbnail, fileId)

    let str = '201 THUMBNAIL RETRIEVE OK\r\n'
    await new Promise((resolve, reject) => {
      fs.createReadStream(filePath)
        .on('error', reject)
        .pipe(base64.encode())
        .on('error', reject)
        .on('data', data => {
          str += data
        })
        .on('end', resolve)
    })
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
