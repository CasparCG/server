const nconf = require('nconf')
const pkg = require('../package.json')
const fs = require('fs')
const xml2js = require('xml2js')
const path = require('path')

const config = nconf
  .argv()
  .env('__')
  .defaults({
    caspar: {
      config: '../casparcg.config'
    },
    paths: {
      template: '../template',
      media: '../media',
      font: '../font',
      thumbnail: '../font',
      ffmpeg: process.platform === 'win32'
        ? path.join(process.cwd(), './bin/win32/ffmpeg.exe')
        : 'ffmpeg',
      ffprobe: process.platform === 'win32'
        ? path.join(process.cwd(), './bin/win32/ffprobe.exe')
        : 'ffprobe'
    },
    thumbnails: {
      width: 256,
      height: -1,
      chokidar: {
        // Note: See https://www.npmjs.com/package/chokidar#api.
      }
    },
    isProduction: process.env.NODE_ENV === 'production',
    logger: {
      level: process.env.NODE_ENV === 'production' ? 'info' : 'trace',
      name: pkg.name,
      prettyPrint: process.env.NODE_ENV !== 'production'
    },
    http: {
      port: 8000
    }
  })
  .get()

if (config.caspar && config.caspar.config) {
  const parser = new xml2js.Parser()
  const data = fs.readFileSync(config.caspar.config)
  parser.parseString(data, (err, result) => {
    if (err) {
      throw err
    }
    for (const path in result.configuration.paths[0]) {
      config.paths[path.split('-')[0]] = result.configuration.paths[0][path][0]
    }
  })
}

module.exports = config
